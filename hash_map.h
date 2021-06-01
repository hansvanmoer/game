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

#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdbool.h>
#include <stddef.h>

typedef size_t (*hash_map_hash)(const void *);

typedef bool (*hash_map_eq)(const void *, const void *);

struct ptr_hash_map_bucket;

struct ptr_hash_map{
  hash_map_hash hash;
  hash_map_eq eq;
  struct ptr_hash_map_bucket * data;
  size_t cap;
  size_t mask;
  size_t count;
};

int init_ptr_hash_map(struct ptr_hash_map * map, hash_map_hash hash, hash_map_eq eq, size_t cap);

int insert_new_into_ptr_hash_map(struct ptr_hash_map * map, void * key, void * value);

void * get_from_ptr_hash_map(const struct ptr_hash_map * map, const void * key);

void dispose_ptr_hash_map(struct ptr_hash_map * map);

struct ptr_hash_map_iter{
  struct ptr_hash_map * map;
  size_t pos;
};

void init_ptr_hash_map_iter(struct ptr_hash_map_iter * i, struct ptr_hash_map * map);

bool has_ptr_hash_map_iter(struct ptr_hash_map_iter * i);

void * get_ptr_hash_map_iter_key(struct ptr_hash_map_iter * i);

void * get_ptr_hash_map_iter_value(struct ptr_hash_map_iter * i);

void next_ptr_hash_map_iter(struct ptr_hash_map_iter * i);

/*
 * Basic utility functions for use in hash tables
 */

bool hash_map_eq_str(const void * first, const void * second);

size_t hash_map_hash_str(const void * str);

#endif
