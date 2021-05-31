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

#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <iconv.h>
#include <stddef.h>
#include <stdio.h>
#include <uchar.h>
#include <yaml.h>

enum deserializer_scalar_type{
			      DESERIALIZER_SCALAR_TYPE_UNICODE_STRING
};

struct deserializer_var_map{
  int (*begin)(void *);
  int (*end)(void *);
  enum deserializer_scalar_type value_type;
  union{
    int (*unicode_string_entry)(void *, const char *, const char32_t *);
  };
};

enum deserializer_node_type{
			    DESERIALIZER_NODE_TYPE_VAR_MAP
};

struct deserializer_node{
  struct deserializer_node * parent;
  enum deserializer_node_type type;
  union{
    struct deserializer_var_map var_map;
  };
};

struct deserializer{
  iconv_t cd;
  char * buf;
  size_t cap;
  char * key_buf;
  size_t key_cap;
  yaml_parser_t parser;
  void * state;
  int status;
  struct deserializer_node * root;
  struct deserializer_node * cur;
};

int init_deserializer(struct deserializer * d);

void deserializer_expect_map(struct deserializer * d, int (*handle_start)(void *), int (*handle_end)(void *));

void deserializer_expect_unicode_string_entries(struct deserializer * d, int (*handle)(void *, const char *, const char32_t *));

int finalize_deserializer(struct deserializer * d);

int deserialize_from_file(struct deserializer * d, void * state, FILE * file);

void dispose_deserializer(struct deserializer *d);

#endif
