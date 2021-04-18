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


#ifndef VORONOI_H
#define VORONOI_H

#include "deque.h"
#include "edge_list.h"

#include <stdbool.h>

enum event_type{
		EVENT_TYPE_ADD_ARC,
		EVENT_TYPE_REMOVE_ARC
};

struct add_arc_event{
  struct face * face;
};

struct remove_arc_event{
  double priority;
  double x;
  double y;
  struct node * node;
};

struct event{
  struct event * parent;
  struct event * left;
  struct event * right;
  enum event_type type;
  union{
    struct add_arc_event add_arc;
    struct remove_arc_event remove_arc;
  };
};

enum node_type{
	       NODE_TYPE_ARC,
	       NODE_TYPE_HALF_EDGE
};

struct arc_node{
  struct face * face;
  struct event * event;
};

struct half_edge_node{
  double x;
  double y;
  double dx;
  double dy;
  struct half_edge * half_edge;
};

struct node{
  struct node * parent;
  struct node * left;
  struct node * right;
  enum node_type type;
  union{
    struct arc_node arc;
    struct half_edge_node half_edge;
  };
};

struct diagram{
  struct edge_list * el;
  struct deque nodes;
  struct deque events;

  double width;
  double height;
  struct node * root_node;
  struct event * root_event;
};

bool create_voronoi_diagram(struct edge_list * el, size_t face_count, double width, double height);

#endif
