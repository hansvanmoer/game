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

struct half_edge * emplace_edge(struct edge_list * el);

struct face * emplace_face(struct edge_list * el);

void print_edge_list(const struct edge_list * el);

void dispose_edge_list(struct edge_list * el);

void connect_half_edges(struct half_edge * first, struct half_edge * second);

bool project_half_edge_on_bounds(struct edge_list * el, struct half_edge * he, double x, double y, double dx, double dy, double width, double height);

bool close_face_with_bounds(struct edge_list * el, struct face * face, double width, double height);

#endif
