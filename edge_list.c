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

#include "edge_list.h"

#include <assert.h>
#include <stdio.h>

#define EDGE_LIST_BLOCK_CAP 10

void set_head_half_edge(struct face * face, struct half_edge * he){
  assert(face != NULL);
  assert(he != NULL);
  assert(he->prev == NULL);
  assert(he->next == NULL);
  
  he->face = face;
  if(face->head == NULL){
    face->head = he;
    face->tail = he;
  }else{
    he->next = face->head;
    face->head->prev = he;
    face->head = he;
  }
}


void set_tail_half_edge(struct face * face, struct half_edge * he){
  assert(face != NULL);
  assert(he != NULL);
  assert(he->prev == NULL);
  assert(he->next == NULL);
  
  he->face = face;
  if(face->tail == NULL){
    face->head = he;
    face->tail = he;
  }else{
    he->prev = face->tail;
    face->tail->next = he;
    face->tail = he;
  }
}

void insert_half_edge_before(struct half_edge * pos, struct half_edge * he){
  assert(pos != NULL);
  assert(he != NULL);
  assert(he->prev == NULL);
  assert(he->next == NULL);

  struct face * face = pos->face;
  assert(face != NULL);
  he->face = pos->face;
  if(pos->prev == NULL){
    assert(face->head == pos);
    pos->prev = he;
    he->next = pos;
    face->head = he;
  }else{
    pos->prev->next = he;
    he->prev = pos->prev;
    he->next = pos;
    pos->prev = he;
  }
}

void insert_half_edge_after(struct half_edge * pos, struct half_edge * he){
  assert(pos != NULL);
  assert(he != NULL);
  assert(he->prev == NULL);
  assert(he->next == NULL);

  struct face * face = pos->face;
  assert(face != NULL);
  he->face = face;
  if(pos->next == NULL){
    assert(face->tail == pos);
    pos->next = he;
    he->prev = pos;
    face->tail = he;
  }else{
    pos->next->prev = he;
    he->next = pos->next;
    he->prev = pos;
    pos->next = he;
  }
}

void init_edge_list(struct edge_list * el){
  assert(el != NULL);
  
  init_deque(&el->vertices, sizeof(struct vertex), EDGE_LIST_BLOCK_CAP);
  init_deque(&el->half_edges, sizeof(struct half_edge), EDGE_LIST_BLOCK_CAP);
  init_deque(&el->faces, sizeof(struct face), EDGE_LIST_BLOCK_CAP);

  el->head = NULL;
  el->tail = NULL;
}

struct vertex * emplace_vertex(struct edge_list * el){
  assert(el != NULL);
  struct vertex * v = (struct vertex *)emplace_onto_deque(&el->vertices);
  if(v != NULL){
    v->x = 0;
    v->y = 0;
  }
  return v;
}

struct half_edge * emplace_half_edge(struct edge_list * el){
  assert(el != NULL);
  struct half_edge * he = (struct half_edge *)emplace_onto_deque(&el->half_edges);
  if(he != NULL){
    he->vertex = NULL;
    he->twin = NULL;
    he->face = NULL;
    he->prev = NULL;
    he->next = NULL;
  }
  return he;
}

struct face * emplace_face(struct edge_list * el){
  assert(el != NULL);
  struct face * f = (struct face *)emplace_onto_deque(&el->faces);
  if(f != NULL){
    f->x = 0;
    f->y = 0;
    f->head = NULL;
    f->tail = NULL;
    if(el->head == NULL){
      el->head = f;
      el->tail = f;
      f->prev = NULL;
    }else{
      el->tail->next = f;
      f->prev = el->tail;
      el->tail = f;
    }
    f->next = NULL;
  }
  return f;
}

static void print_half_edge(const struct half_edge * he){
  assert(he != NULL);
  struct half_edge * twin = he->twin;
  assert(twin != NULL);
  struct vertex * v = he->vertex;
  struct vertex * tv = twin->vertex;
  if(v == NULL){
    if(tv == NULL){
      fputs("\thalf edge NONE -> NONE\n", stdout);
    }else{
      printf("\thalf edge NONE -> (%.2f, %.2f)\n", tv->x, tv->y);
    }
  }else if(tv == NULL){
    printf("\thalf edge (%.2f, %.2f) -> NONE\n", v->x, v->y);
  }else{
    printf("\thalf edge (%.2f, %.2f) -> (%.2f, %.2f)\n", v->x, v->y, tv->x, tv->y);
  }
}

static void print_face(const struct face * face){
  assert(face != NULL);

  printf("face:\n\tsite(%.2f, %.2f)\n", face->x, face->y);
  struct half_edge * he = face->head;
  while(he != NULL){
    print_half_edge(he);
    he = he->next;
  }
}

void print_edge_list(const struct edge_list * el){
  assert(el != NULL);
  for(struct face * f = el->head; f != NULL; f = f->next){
    print_face(f);
  };
}

void dispose_edge_list(struct edge_list * el){
  assert(el != NULL);
  dispose_deque(&el->faces);
  dispose_deque(&el->half_edges);
  dispose_deque(&el->vertices);
}

