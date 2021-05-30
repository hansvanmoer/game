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
#include "protocol.h"
#include "status.h"
#include "unicode.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PROTOCOL_NETWORK_ENCODING "UTF-8"

static const char * msg_headers[] = {
  "AUTHENTICATION REQUEST",
  "AUTHENTICATION RESPONSE",
  "CLOSE REQUEST",
  "CLOSE RESPONSE"
};

const char * get_protocol_msg_type_label(enum protocol_msg_type type){
  return msg_headers[(int)type];
}

static const char * get_host_encoding(){
  int x = 1;
  if(*((char *)&x) == 1){
    //little endian
    return "UTF-32LE";
  }else{
    //big endian
    return "UTF-32BE";
  }
}

int init_protocol_state(struct protocol_state * ps){
  assert(ps != NULL);

  iconv_t dec = iconv_open(get_host_encoding(), PROTOCOL_NETWORK_ENCODING);
  if(dec == (iconv_t)-1){
    LOG_ERROR("could not create encoder from %s to %s", PROTOCOL_NETWORK_ENCODING, get_host_encoding());
    set_status(STATUS_CREATE_ENCODER_ERROR);
    return -1;
  }

  iconv_t enc = iconv_open(PROTOCOL_NETWORK_ENCODING, get_host_encoding());
  if(enc == (iconv_t)-1){
    LOG_ERROR("could not create encoder from %s to %s", get_host_encoding(), PROTOCOL_NETWORK_ENCODING);
     set_status(STATUS_CREATE_ENCODER_ERROR);
     return -1;
  }

  ps->enc = enc;
  ps->dec = dec;
  return 0;
}

static int write_bytes(struct protocol_state * ps, const char * buf, size_t len){
  assert(ps != NULL);
  assert(buf != NULL);
  while(len != 0){
     ssize_t write_result = write(ps->fd, buf, len);
     if(write_result == -1){
       LOG_ERROR("error while writing byte sequence");
       set_status(STATUS_IO_ERROR);
       return -1;
     }else{
       len -= write_result;
     }
   }
   return 0;
}

static int write_delim(struct protocol_state * ps){
  return write_bytes(ps, "\n", 1);
}

static int read_string(char * buf, size_t size, struct protocol_state * ps){
  assert(buf != NULL);
  assert(ps != NULL);

  char c;
  size_t len = 0;
  while(true){
    ssize_t result = read(ps->fd, &c, 1);
    if(result == -1){
      LOG_ERROR("error reading byte sequence");
      set_status(STATUS_IO_ERROR);
      return -1;
    }else if(result == 1){
      if(c == '\n'){
	buf[len] = '\0';
	break;
      }else{
	buf[len] = c;
	++len;
      }
    }
    if(len == size){
      LOG_ERROR("error reading byte sequence: string too long");
      set_status(STATUS_PROTOCOL_ERROR);
      return -1;
    }
  }
  return 0;
}

static int write_string(struct protocol_state * ps, const char * str){
  assert(ps != NULL);
  assert(str != NULL);

  if(write_bytes(ps, str, strlen(str))){
    return -1;
  }
  if(write_delim(ps)){
    return -1;
  }
  return 0;
}

static int read_int(int * dest, struct protocol_state * ps){
  assert(ps != NULL);
  
  if(read_string(ps->in_buf, PROTOCOL_STATE_IN_BUF_LEN, ps)){
    return -1;
  }

  if(sscanf(ps->in_buf, "%d", dest) != 1){
    LOG_ERROR("error reading int");
    set_status(STATUS_PROTOCOL_ERROR);
    return -1;
  }
  return 0;
}

static int write_int(struct protocol_state * ps, int i){
  assert(ps != NULL);
  
  int result = snprintf(ps->out_buf, PROTOCOL_STATE_OUT_BUF_LEN, "%d", i);
  if(result == -1){
    LOG_ERROR("error formatting int to byte sequence");
    set_status(STATUS_IO_ERROR);
    return -1;
  }else if(result >= PROTOCOL_STATE_OUT_BUF_LEN){
    LOG_ERROR("error formatting int to byte sequence: string too long");
    set_status(STATUS_IO_ERROR);
    return -1;
  }else{
    if(write_bytes(ps, ps->out_buf, result)){
      return -1;
    }
    if(write_delim(ps)){
      return -1;
    }
    return 0;
  }
}

static int read_unicode_string(char32_t * buf, size_t size, struct protocol_state * ps){
  assert(buf != NULL);
  assert(ps != NULL);

  size_t in_left = 0;
  char * out_buf = (char *)buf;
  size_t out_left = (size - 1) * sizeof(char32_t);
  
  bool eof = false;

  while(true){

    if(!eof){
      char * in_buf = ps->in_buf + in_left;
      while(in_left != PROTOCOL_STATE_IN_BUF_LEN){
	ssize_t result = read(ps->fd, in_buf, 1);
	if(result == -1){
	  LOG_ERROR("error while reading UTF-8 sequence");
	  set_status(STATUS_IO_ERROR);
	  return -1;
	}else if(result == 1){
	  if(*in_buf == '\n'){
	    eof = true;
	    break;
	  }else{
	    ++in_left;
	    ++in_buf;
	  }
	}
      }
    }

    char * in_buf = ps->in_buf;

    size_t result = iconv(ps->dec, &in_buf, &in_left, &out_buf, &out_left);
    if(result == (size_t)-1){
      if(errno == EILSEQ){
	LOG_ERROR("invalid input byte sequence detected while reading UTF-8 string");
	set_status(STATUS_ENCODING_ERROR);
	return -1;
      }else if(errno == EINVAL){
	if(eof){
	  LOG_ERROR("error reading string: invalid byte sequence detected at end");
	  set_status(STATUS_ENCODING_ERROR);
	  return -1;
	}
	memmove(ps->in_buf, in_buf, in_left);
      }else if(errno == E2BIG){
	LOG_ERROR("error reading unicode string: string too long");
	set_status(STATUS_PROTOCOL_ERROR);
	return -1;
      }else{
	LOG_ERROR("unexpected error while encoding UTF-8 string: %s", strerror(errno));
	set_status(STATUS_ENCODING_ERROR);
	return -1;
      }
    }
    if(in_left == 0 && eof){
      break;
    }
  }
  size_t len = ((size - 1) * sizeof(char32_t) - out_left) / sizeof(char32_t);
  buf[len] = '\0';
  return 0;
}

static int write_unicode_string(struct protocol_state * ps, const char32_t * str){
  assert(ps != NULL);
  assert(str != NULL);
  char * in_buf = (char*)str;
  size_t in_left = unicode_strlen(str) * sizeof(char32_t);
  char * out_buf = ps->out_buf;
  size_t out_left = PROTOCOL_STATE_OUT_BUF_LEN;
  
  while(in_left != 0){
    size_t result = iconv(ps->enc, &in_buf, &in_left, &out_buf, &out_left);
    if(result == (size_t)-1){
      if(errno == EILSEQ){
	LOG_ERROR("invalid input byte sequence detected while reading UTF-8 string");
	set_status(STATUS_ENCODING_ERROR);
	return -1;
      }else if(errno == EINVAL){
	LOG_ERROR("incomplete input byte sequence detected while reading UTF-8 string");
	set_status(STATUS_ENCODING_ERROR);
	return -1;
      }else if(errno == E2BIG){
	if(write_bytes(ps, ps->out_buf, PROTOCOL_STATE_OUT_BUF_LEN - out_left)){
	  return -1;
	}
	out_buf = ps->out_buf;
	out_left = PROTOCOL_STATE_OUT_BUF_LEN;
      }else{
	LOG_ERROR("unexpected error while encoding UTF-8 string: %s", strerror(errno));
	set_status(STATUS_ENCODING_ERROR);
	return -1;
      }
    }else{
      	if(write_bytes(ps, ps->out_buf, PROTOCOL_STATE_OUT_BUF_LEN - out_left)){
	  return -1;
	}
    }
  }
  if(write_delim(ps)){
    return -1;
  }
  return 0;
}

static int read_msg_header(enum protocol_msg_type * type, struct protocol_state * ps){
  assert(type != NULL);
  assert(ps != NULL);

  if(read_string(ps->name_buf, PROTOCOL_STATE_NAME_BUF_LEN, ps)){
    return -1;
  }
  int result = -1;
  for(int i = 0; i < sizeof(msg_headers) / sizeof(const char *); ++i){
    if(strcmp(msg_headers[i], ps->name_buf) == 0){
      result = i;
      break;
    }
  }
  if(result == -1){
    LOG_ERROR("unknown message type: %s", ps->name_buf);
    set_status(STATUS_PROTOCOL_ERROR);
    return -1;
  }
  *type = (enum protocol_msg_type)result;
  return 0;
}

static int write_msg_header(struct protocol_state * ps, const char * name){
  return write_string(ps, name);
}

static int read_auth_req_body(struct protocol_auth_req * msg, struct protocol_state * ps){
  assert(msg != NULL);
  assert(ps != NULL);

  if(read_unicode_string(msg->name, GAME_MAX_PLAYER_NAME_LEN, ps)){
    return -1;
  }
  
  return 0;
}

static int write_auth_req_body(struct protocol_state * ps, const struct protocol_auth_req * msg){
  assert(ps != NULL);
  assert(msg != NULL);

  if(write_unicode_string(ps, msg->name)){
    return -1;
  }
  return 0;
}

static int read_auth_res_body(struct protocol_auth_res * msg, struct protocol_state * ps){
  assert(msg != NULL);
  assert(ps != NULL);
  
  if(read_int(&msg->id, ps)){
    return -1;
  }
  return read_string(msg->reason, PROTOCOL_MAX_REASON_LEN, ps);
}

static int write_auth_res_body(struct protocol_state * ps, const struct protocol_auth_res * msg){
  assert(ps != NULL);
  assert(msg != NULL);

  if(write_int(ps, msg->id)){
    return -1;
  }
  return write_string(ps, msg->reason);
}

static int read_close_req_body(struct protocol_close_req * msg, struct protocol_state * ps){
  assert(msg != NULL);
  assert(ps != NULL);
  
  return read_string(msg->reason, PROTOCOL_MAX_REASON_LEN, ps);
}

static int write_close_req_body(struct protocol_state * ps, const struct protocol_close_req * msg){
  assert(ps != NULL);
  assert(msg != NULL);

  return write_string(ps, msg->reason);
}


static int read_close_res_body(struct protocol_close_res * msg, struct protocol_state * ps){
  assert(msg != NULL);
  assert(ps != NULL);
  if(read_int(&msg->id, ps)){
    return -1;
  }
  return read_string(msg->reason, PROTOCOL_MAX_REASON_LEN, ps);
}

static int write_close_res_body(struct protocol_state * ps, const struct protocol_close_res * msg){
  assert(ps != NULL);
  assert(msg != NULL);

  if(write_int(ps, msg->id)){
    return -1;
  }
  return write_string(ps, msg->reason);
}


int read_protocol_msg(struct protocol_state * ps, struct protocol_msg * msg, int fd){
  assert(msg != NULL);
  assert(ps != NULL);

  ps->fd = fd;
  
  if(read_msg_header(&msg->type, ps)){
    return -1;
  }
  switch(msg->type){
  case PROTOCOL_MSG_TYPE_AUTH_REQ:
   return read_auth_req_body(&msg->auth_req, ps);
  case PROTOCOL_MSG_TYPE_AUTH_RES:
    return read_auth_res_body(&msg->auth_res, ps);
  case PROTOCOL_MSG_TYPE_CLOSE_REQ:
    return read_close_req_body(&msg->close_req, ps);
  case PROTOCOL_MSG_TYPE_CLOSE_RES:
    return read_close_res_body(&msg->close_res, ps);
  }
  return 0;
}

int write_protocol_msg(struct protocol_state * ps, int fd, const struct protocol_msg * msg){
  assert(ps != NULL);
  assert(msg != NULL);

  ps->fd = fd;
  
  if(write_msg_header(ps, msg_headers[(int)msg->type])){
    return -1;
  }
  switch(msg->type){
  case PROTOCOL_MSG_TYPE_AUTH_REQ:
    return write_auth_req_body(ps, &msg->auth_req);
  case PROTOCOL_MSG_TYPE_AUTH_RES:
    return write_auth_res_body(ps, &msg->auth_res);
  case PROTOCOL_MSG_TYPE_CLOSE_REQ:
    return write_close_req_body(ps, &msg->close_req);
  case PROTOCOL_MSG_TYPE_CLOSE_RES:
    return write_close_res_body(ps, &msg->close_res);
  }
  return -1;
}

void dispose_protocol_state(struct protocol_state * ps){
  assert(ps != NULL);
  iconv_close(ps->enc);
  iconv_close(ps->dec);
}


void init_protocol_auth_req(struct protocol_msg *msg, const char32_t * name){
  assert(msg != NULL);
  assert(name != NULL);

  msg->type = PROTOCOL_MSG_TYPE_AUTH_REQ;
  struct protocol_auth_req * body = &msg->auth_req;
  
  unicode_strcpy_checked(body->name, GAME_MAX_PLAYER_NAME_LEN, name);
}


void init_protocol_auth_res(struct protocol_msg * msg, int id, const char * reason){
  assert(msg != NULL);
  assert(msg != NULL);
  assert(id >= -1);
  assert(strlen(reason) <= PROTOCOL_MAX_REASON_LEN);
  
  msg->type = PROTOCOL_MSG_TYPE_AUTH_RES;
  struct protocol_auth_res * body = &msg->auth_res;

  body->id = id;
  strcpy(body->reason, reason);
}

