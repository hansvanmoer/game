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

#include "deque.h"
#include "memory.h"
#include "status.h"

#include <assert.h>
#include <stdbool.h>

#define DEQUE_DEFAULT_BLOCK_CAP 8

void init_deque(struct deque * deque, size_t elem_size, size_t block_cap){
  assert(deque != NULL);
  assert(elem_size != 0);
  
  deque->head = NULL;
  deque->tail = NULL;
  deque->elem_size = elem_size;
  deque->len = 0;
  if(block_cap == 0){
    deque->block_cap = DEQUE_DEFAULT_BLOCK_CAP;
    deque->block_len = DEQUE_DEFAULT_BLOCK_CAP;
  }else{
    deque->block_cap = block_cap;
    deque->block_len = block_cap;
  }
}

static bool ensure_cap(struct deque * deque){
  assert(deque != NULL);
  if(deque->block_len == deque->block_cap){
    struct deque_block * block = malloc_checked(sizeof(struct deque_block));
    if(block == NULL){
      return true;
    }
    
    assert(deque->block_cap != 0);
    assert(deque->elem_size != 0);
    void * data = malloc_checked(deque->elem_size * deque->block_cap);
    if(data == NULL){
      free(block);
      return true;
    }
    block->data = data;
    
    block->prev = deque->tail;
    block->next = NULL;   
    if(deque->head == NULL){
      deque->head = block;
    }else{
      deque->tail->next = block;
    }
    deque->tail = block;
    deque->block_len = 0;
  }
  return false;
}

void * emplace_onto_deque(struct deque * deque){
  assert(deque != NULL);
  
  if(ensure_cap(deque)){
    return NULL;
  }
  size_t i = deque->block_len;
  ++deque->block_len;
  ++deque->len;
  return deque->tail->data + (i * deque->elem_size); 
}

void init_deque_iter(struct deque_iter * iter, struct deque * deque){
  assert(iter != NULL);
  assert(deque != NULL);
  iter->deque = deque;
  iter->block = deque->head;
  iter->index = 0;
}

bool has_next_deque_iter(struct deque_iter * i){
  assert(i != NULL);
  return (i->block != NULL) && (i->block != i->deque->tail || i->index != i->deque->block_len);
}

void * get_deque_iter(struct deque_iter * i){
  assert(i != NULL);
  assert(i->block != NULL);
  assert(i->block != i->deque->tail || i->index < i->deque->block_len);
  return i->block->data + (i->index * i->deque->elem_size);
}

void next_deque_iter(struct deque_iter * i){
  assert(i != NULL);
  size_t next = i->index + 1;
  if(next == i->deque->block_cap){
    i->block = i->block->next;
    i->index = 0;
  }else if(i->block == i->deque->tail && next == i->deque->block_len){
    i->block = NULL;
    i->index = 0;
  }else{
    ++i->index;
  }
}

void dispose_deque(struct deque * deque){
  assert(deque != NULL);

  struct deque_block * b = deque->head;
  while(b != NULL){
    free(b->data);
    struct deque_block * n = b->next;
    free(b);
    b = n;
  }
}
