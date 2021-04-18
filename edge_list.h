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

#ifndef EDGE_LIST_H
#define EDGE_LIST_H

#include "deque.h"

#include <stdbool.h>

struct vertex{
  double x;
  double y;
};

struct face;

struct half_edge{
  struct vertex * vertex;
  struct half_edge * twin;
  struct face * face;
  struct half_edge * prev;
  struct half_edge * next;
};

struct face{
  double x;
  double y;
  struct half_edge * head;
  struct half_edge * tail;
  struct face * prev;
  struct face * next;
};

struct edge_list{
  struct deque vertices;
  struct deque half_edges;
  struct deque faces;
  struct face * head;
  struct face * tail;
};

void init_edge_list(struct edge_list * el);

struct vertex * emplace_vertex(struct edge_list * el);

struct half_edge * emplace_half_edge(struct edge_list * el);

struct face * emplace_face(struct edge_list * el);

void print_edge_list(const struct edge_list * el);

void dispose_edge_list(struct edge_list * el);

void set_head_half_edge(struct face * face, struct half_edge * he);

void set_tail_half_edge(struct face * face, struct half_edge * he);

void insert_half_edge_before(struct half_edge * pos, struct half_edge * he);

void insert_half_edge_after(struct half_edge * pos, struct half_edge * he);

#endif
