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

#include "hash_map.h"
#include "memory.h"
#include "status.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#define DEFAULT_HASH_MAP_MASK 0x7

#define DEFAULT_HASH_MAP_MIN_CAP 8

#define MAX_HASH_MAP_RATIO 0.7

struct ptr_hash_map_bucket{
  void * key;
  void * value;
  size_t hash;
};

void calc_cap(size_t min_cap, size_t * cap, size_t * mask){
  size_t c = DEFAULT_HASH_MAP_MIN_CAP;
  size_t m = DEFAULT_HASH_MAP_MASK;
  while(c < min_cap){
    c<<=1;
    m = (m << 1) | 0x1;
  }
  *cap = c;
  *mask = m;
}

int init_ptr_hash_map(struct ptr_hash_map * map, hash_map_hash hash, hash_map_eq eq, size_t cap){
  assert(map != NULL);
  assert(hash != NULL);
  assert(eq != NULL);

  map->hash = hash;
  map->eq = eq;
  if(cap == 0){
    map->cap = DEFAULT_HASH_MAP_MIN_CAP;
    map->mask = DEFAULT_HASH_MAP_MASK;
  }else{
    calc_cap(cap, &map->cap, &map->mask);
  }

  size_t size = sizeof(struct ptr_hash_map_bucket) * map->cap;
  map->data = malloc_checked(size);
  if(map->data == NULL){
    return -1;
  }
  memset(map->data, 0, size);
  map->count = 0;
  return 0;
}

static bool is_empty(const struct ptr_hash_map_bucket * b){
  return b->key == NULL;
}

static bool is_key(const struct ptr_hash_map * map, const struct ptr_hash_map_bucket * b, const void * key){
  return (*map->eq)(b->key, key);
}

static int grow(struct ptr_hash_map * map, size_t ncap, size_t nmask){
  assert(ncap > map->cap);
  size_t size = ncap * sizeof(struct ptr_hash_map_bucket);
  struct ptr_hash_map_bucket * ndata = malloc_checked(size);
  if(ndata == NULL){
    return -1;
  }
  memset(ndata, 0, size);

  for(size_t pos = 0; pos < map->cap; ++pos){
    struct ptr_hash_map_bucket * b = map->data + pos;
    if(!is_empty(b)){
      size_t npos = b->hash & nmask;
      while(!is_empty(ndata + npos)){
	npos = (npos + 1) & nmask;
      }
      memcpy(ndata + npos,  b, sizeof(struct ptr_hash_map_bucket));
    }
  }

  free(map->data);
  map->data = ndata;
  map->cap = ncap;
  map->mask = nmask;
  
  return 0;
}

static int ensure_cap(struct ptr_hash_map * map){
  double ratio = ((double)(map->count + 1)) / map->cap;
  if(ratio > MAX_HASH_MAP_RATIO){
    return grow(map, map->cap << 1, (map->mask << 1) | 0x1);
  }
  return 0;
}

int insert_new_into_ptr_hash_map(struct ptr_hash_map * map, void * key, void * value){
  assert(map != NULL);
  assert(key != NULL);

  if(ensure_cap(map)){
    return -1;
  }

  size_t hash = (*map->hash)(key);
  size_t pos = hash & map->mask;
  while(true){
    struct ptr_hash_map_bucket * b = map->data + pos;
    if(is_empty(b)){
      b->key = key;
      b->value = value;
      b->hash = hash;
      ++map->count;
      return 0;
    }else if(is_key(map, b, key)){
      set_status(STATUS_DUPLICATE_KEY);
      return -1;
    }else{
      pos = (pos + 1) & map->mask;
    }
  }
}

void * get_from_ptr_hash_map(const struct ptr_hash_map * map, const void * key){
  assert(map != NULL);
  assert(key != NULL);

  size_t hash = (*map->hash)(key);
  size_t pos = hash & map->mask;
  while(true){
    const struct ptr_hash_map_bucket * b = map->data + pos;
    if(is_empty(b)){
      return NULL;
    }else if(is_key(map, b, key)){
      return b->value;
    }else{
      pos = (pos + 1) & map->mask;
    }
  }
}

void dispose_ptr_hash_map(struct ptr_hash_map * map){
  assert(map != NULL);
  free(map->data);
}

static size_t get_next_filled(struct ptr_hash_map * map, size_t pos){
  while(pos < map->cap){
    struct ptr_hash_map_bucket * b = map->data + pos;
    if(is_empty(b)){
      ++pos;
    }else{
      return pos;
    }
  }
  return map->cap;
}

void init_ptr_hash_map_iter(struct ptr_hash_map_iter * i, struct ptr_hash_map * map){
  assert(i != NULL);
  assert(map != NULL);
  i->map = map;
  
  i->pos = get_next_filled(i->map, 0);
}

bool has_ptr_hash_map_iter(struct ptr_hash_map_iter * i){
  assert(i != NULL);
  return i->pos != i->map->cap;
}

void * get_ptr_hash_map_iter_key(struct ptr_hash_map_iter * i){
  assert(i != NULL);
  return i->map->data[i->pos].key;
}

void * get_ptr_hash_map_iter_value(struct ptr_hash_map_iter * i){
  assert(i != NULL);
  return i->map->data[i->pos].value;
}

void next_ptr_hash_map_iter(struct ptr_hash_map_iter * i){
  assert(i != NULL);
  i->pos = get_next_filled(i->map, i->pos + 1);
}

size_t hash_map_hash_str(const void * key){
  const char * str = (const char *)key;
  size_t hash = 5381;
  while(*str){
    hash = ((hash << 5) + hash) + *str;
    ++str;
  }
  return hash;
}

bool hash_map_eq_str(const void * first, const void * second){
  return strcmp((const char *)first, (const char *)second) == 0;
}
