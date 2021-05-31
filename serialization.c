/*
 * This file is part of game.
 *
 * game is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *    game is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with game.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "logger.h"
#include "memory.h"
#include "serialization.h"
#include "status.h"
#include "unicode.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>

#define INITIAL_BUF_CAP 1024
#define INITIAL_KEY_BUF_CAP 256

int init_deserializer(struct deserializer * d){
  assert(d != NULL);

  d->buf = malloc_checked(INITIAL_BUF_CAP);
  if(d->buf == NULL){
    return -1;
  }
  d->cap = INITIAL_BUF_CAP;

  d->key_buf = malloc_checked(INITIAL_KEY_BUF_CAP);
  if(d->key_buf == NULL){
    free(d->buf);
    return -1;
  }
  d->key_cap = INITIAL_KEY_BUF_CAP;
  
  iconv_t cd = iconv_open(get_unicode_encoding_name(), "UTF-8");
  if(cd == (iconv_t)-1){
    set_status(STATUS_CREATE_ENCODER_ERROR);
    free(d->buf);
    free(d->key_buf);
    return -1;
  }
  d->cd = cd;
 
  
  if(yaml_parser_initialize(&d->parser) == 0){
    set_status(STATUS_YAML_ERROR);
    iconv_close(cd);
    free(d->buf);
    free(d->key_buf);
    return -1;
  }
  d->state = NULL;
  d->status = 0;
  d->root = NULL;
  d->cur = NULL;
  return 0;
}

static int grow_buffer(struct deserializer * d, char ** out_buf, size_t * out_left){
  assert(d != NULL);
  assert(out_buf != NULL);
  assert(out_left != NULL);
  
  size_t ncap = d->cap * 2;
  char * nbuf = malloc_checked(ncap);
  if(nbuf == NULL){
    return -1;
  }
  size_t len = d->cap - *out_left;
  memcpy(nbuf, d->buf, len);
  free(d->buf);
  d->buf = nbuf;
  d->cap = ncap;
  *out_buf = d->buf + len;
  *out_left = d->cap - len;
  return 0;
}

static int copy_key(struct deserializer * d, const char * key){
  assert(d != NULL);
  assert(key != NULL);

  size_t len = strlen(key);
  if(len >= d->key_cap){
    size_t ncap = d->key_cap * 2;
    while(len >= ncap){
      ncap*=2;
    }
    char * nbuf = malloc_checked(ncap);
    if(nbuf == NULL){
      return -1;
    }
    free(d->key_buf);
    d->key_buf = nbuf;
    d->key_cap = ncap;
  }
  memcpy(d->key_buf, key, len + 1);
  return 0;
}

static int convert_value(struct deserializer * d, const char * src){
  char * in_buf = (char *)src;
  size_t in_left = strlen(in_buf);
  char * out_buf = (char *)d->buf;
  size_t out_left = d->cap;

  while(in_left != 0){
    size_t result = iconv(d->cd, &in_buf, &in_left, &out_buf, &out_left);
    if(result == -1){
      if(errno == E2BIG){
	if(grow_buffer(d, &out_buf, &out_left)){
	  return -1;
	}
      }else{
	LOG_ERROR("invalid encoding found in YAML file");
	set_status(STATUS_ENCODING_ERROR);
	return -1;
      }
    }
  }
  if(out_left < sizeof(char32_t)){
    if(grow_buffer(d, &out_buf, &out_left)){
      return -1;
    }
  }
  memset(out_buf, 0, sizeof(char32_t));
  return 0;
}

static struct deserializer_node * add_deserializer_node(struct deserializer * d, enum deserializer_node_type type){
  assert(d != NULL);
  struct deserializer_node * n = malloc_checked(sizeof(struct deserializer_node));
  if(n == NULL){
    d->status = -1;
    return NULL;
  }
  n->type = type;
  n->parent = d->cur;
  if(d->root == NULL){
    d->root = n;
  }
  d->cur = n;
  return n;
}

static bool ensure_parent_type(struct deserializer * d, enum deserializer_node_type type){
  assert(d != NULL);
  if(d->status == -1 || d->cur == NULL || d->cur->type != DESERIALIZER_NODE_TYPE_VAR_MAP){
    d->status = -1;
    return false;
  }else{
    return true;
  }
}

void deserializer_expect_map(struct deserializer * d, int (*handle_start)(void *), int (*handle_end)(void *)){
  assert(d != NULL);

  struct deserializer_node * n = add_deserializer_node(d, DESERIALIZER_NODE_TYPE_VAR_MAP);
  if(n != NULL){
    n->var_map.begin=handle_start;
    n->var_map.end=handle_end;
  }
}

void deserializer_expect_unicode_string_entries(struct deserializer * d, int (*handle)(void *, const char *, const char32_t *)){
  assert(d != NULL);
  if(ensure_parent_type(d, DESERIALIZER_NODE_TYPE_VAR_MAP)){
    d->cur->var_map.value_type = DESERIALIZER_SCALAR_TYPE_UNICODE_STRING;
    d->cur->var_map.unicode_string_entry = handle;
  }
}

int finalize_deserializer(struct deserializer * d){
  assert(d != NULL);
  return d->status;
}

static int expect_yaml_event(struct deserializer * d, yaml_event_type_t type, const char * msg){
  assert(d != NULL);
  assert(msg != NULL);

  yaml_event_t e;
  
  if(yaml_parser_parse(&d->parser, &e) == 0){
     LOG_ERROR("yaml parser error before %s event", msg);
    set_status(STATUS_YAML_ERROR);
    return -1;
  }
  if(e.type != type){
    LOG_ERROR("expected %s event", msg);
    set_status(STATUS_SYNTAX_ERROR);
    yaml_event_delete(&e);
    return -1;
  }
  yaml_event_delete(&e);
  return 0;
}

static int deserialize_var_map(struct deserializer * d, struct deserializer_node * n){

  if(expect_yaml_event(d, YAML_MAPPING_START_EVENT, "map start")){
    return -1;
  }

  if(n->var_map.begin != NULL){
    int result = (*n->var_map.begin)(d->state);
    if(result != 0){
      set_status(STATUS_SYNTAX_ERROR);
      return result;
    }
  }

  yaml_event_t e;
  
  while(true){
    if(yaml_parser_parse(&d->parser, &e) == 0){
      LOG_ERROR("yaml parser error before mapping end event");
      set_status(STATUS_YAML_ERROR);
      return -1;
    }
    if(e.type == YAML_MAPPING_END_EVENT){
      if(n->var_map.begin != NULL){
	int result = (*n->var_map.end)(d->state);
	if(result != 0){
	  set_status(STATUS_SYNTAX_ERROR);
	  yaml_event_delete(&e);
	  return result;
	}
      }
      return 0;
    }else if(e.type == YAML_SCALAR_EVENT){

      if(copy_key(d, (const char *)e.data.scalar.value)){
	yaml_event_delete(&e);
	return -1;
      }

      yaml_event_delete(&e);
      
      if(yaml_parser_parse(&d->parser, &e) == 0){
	LOG_ERROR("yaml parser error before key scalar event");
	set_status(STATUS_YAML_ERROR);
	return -1;
      }

      if(e.type != YAML_SCALAR_EVENT){
	LOG_ERROR("unexpected event before value scalar event");
	set_status(STATUS_SYNTAX_ERROR);
	yaml_event_delete(&e);
	return -1;
      }

      int result;
      switch(n->var_map.value_type){
      case DESERIALIZER_SCALAR_TYPE_UNICODE_STRING:
	if(convert_value(d, (const char *)e.data.scalar.value)){
	  yaml_event_delete(&e);
	  return -1;
	}
	result = (*n->var_map.unicode_string_entry)(d->state, d->key_buf, (const char32_t *)d->buf);
	break;
      }
      yaml_event_delete(&e);
      if(result != 0){
	return result;
      }
      
    }else{
      LOG_ERROR("unexpected event in mapping");
      set_status(STATUS_SYNTAX_ERROR);
      yaml_event_delete(&e);
      return -1;
    }
 
  }
}

static int deserialize_node(struct deserializer * d, struct deserializer_node * n){
  switch(n->type){
  case DESERIALIZER_NODE_TYPE_VAR_MAP:
    return deserialize_var_map(d, n);
  default:
    LOG_ERROR("unexpected node");
    set_status(STATUS_SYNTAX_ERROR);
    return -1;
  }
}

int deserialize_from_file(struct deserializer * d, void * state, FILE * file){
  assert(d != NULL);
  assert(file != NULL);
  d->state = state;
  yaml_parser_set_input_file(&d->parser, file);

  if(expect_yaml_event(d, YAML_STREAM_START_EVENT, "stream start")){
    return -1;
  }
  if(expect_yaml_event(d, YAML_DOCUMENT_START_EVENT, "document start")){
    return -1;
  }
  
  if(deserialize_node(d, d->root)){
    return -1;
  }

  if(expect_yaml_event(d, YAML_DOCUMENT_END_EVENT, "document end")){
    return -1;
  }
  if(expect_yaml_event(d, YAML_STREAM_END_EVENT, "stream end")){
    return -1;
  }
  
  return 0;
}

static void dispose_node(struct deserializer_node * n){
  free(n);
}

void dispose_deserializer(struct deserializer *d){
  assert(d != NULL);
  if(d->root != NULL){
    dispose_node(d->root);
  }
  yaml_parser_delete(&d->parser);
  free(d->buf);
  free(d->key_buf);
  iconv_close(d->cd);
}

