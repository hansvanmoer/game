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

#include "memory.h"
#include "status.h"

#include <assert.h>
#include <string.h>

#define DEFAULT_MEMORY_BUFFER_CAP 1024

struct memory_buffer_block{
  char * data;
  size_t left;
  size_t cap;
  struct memory_buffer_block * prev;
  struct memory_buffer_block * next;
};

void * malloc_checked(size_t amount){
  void * block = malloc(amount);
  if(block == NULL){
    set_status(STATUS_MALLOC_FAILED);
  }
  return block;
}

void init_memory_buffer(struct memory_buffer * buf, size_t cap){
  assert(buf != NULL);

  if(cap == 0){
    cap = DEFAULT_MEMORY_BUFFER_CAP;
  }

  buf->cap = cap;
  buf->head = NULL;
  buf->tail = NULL;
}

static void detach_block(struct memory_buffer * buf, struct memory_buffer_block * b){
  assert(buf != NULL);
  assert(b != NULL);

  if(buf->head == b){
    if(buf->tail == b){
      buf->head = NULL;
      buf->tail = NULL;
    }else{
      buf->head = b->next;
      buf->head->prev = NULL;
    }
  }else if(buf->tail == b){
    buf->tail = buf->tail->prev;
    buf->tail->next = NULL;
  }else{
    b->prev->next = b->next;
    b->next->prev = b->prev;
  }
  b->prev = NULL;
  b->next = NULL;
}

static void insert_block(struct memory_buffer * buf, struct memory_buffer_block * b){
  assert(buf != NULL);
  assert(b != NULL);

  struct memory_buffer_block * p = buf->head;
  while(p != NULL && p->left > b->left){
    p = p->next;
  }
  if(p == NULL){
    if(buf->head == NULL){
      buf->head = b;
      buf->tail = b;
    }else{
      buf->tail->next = b;
      b->prev = buf->tail;
      buf->tail = b;
    }
  }else{
    p->prev->next = b;
    b->prev = p->prev;
    p->prev = b;
    b->next = b;
  }
}

static struct memory_buffer_block * create_block(struct memory_buffer * buf, size_t len){
  size_t cap = buf->cap;
  if(cap < len){
    cap = len;
  }
  struct memory_buffer_block * b = malloc_checked(sizeof(struct memory_buffer_block));
  if(b == NULL){
    return NULL;
  }
  b->data = malloc_checked(cap);
  if(b->data == NULL){
    free(b);
    return NULL;
  }
  b->cap = cap;
  b->left = cap;
  b->prev = NULL;
  b->next = NULL;
  return b;
}

static struct memory_buffer_block * find_block(struct memory_buffer * buf, size_t len){
  struct memory_buffer_block * b = buf->head;
  if(b == NULL || b->left < len){
    return NULL;
  }else{
    while(b->next != NULL && b->next->left >= len){
      b = b->next;
    }
    return b;
  }
}

static char * copy_to_block(struct memory_buffer_block * dest, const char * src, size_t len){
  assert(dest != NULL);
  assert(src != NULL);
  assert(dest->left >= len);
  char * pos = dest->data + dest->cap - dest->left;
  memcpy(pos, src, len);
  dest->left-=len;
  return pos;
}

void * copy_to_memory_buffer(struct memory_buffer * dest, const void * src, size_t len){
  assert(dest != NULL);
  assert(src != NULL);
  assert(len != 0);
  
  struct memory_buffer_block * b = find_block(dest, len);
  char * pos;
  if(b == NULL){
    b = create_block(dest, len);
    if(b == NULL){
      return NULL;
    }
    pos = copy_to_block(b, src, len);
    insert_block(dest, b);
  }else{
    detach_block(dest, b);
    pos = copy_to_block(b, src, len);
    insert_block(dest, b);
  }
  return pos;
}

void dispose_memory_buffer(struct memory_buffer * buf){
  assert(buf != NULL);
  struct memory_buffer_block * b = buf->head;
  while(b != NULL){
    struct memory_buffer_block * n = b->next;
    free(b->data);
    free(b);
    b = n;
  }
}
