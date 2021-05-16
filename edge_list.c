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
#include "linear.h"
#include "status.h"

#include <assert.h>
#include <stdio.h>

#define EDGE_LIST_BLOCK_CAP 10

#define TOLERANCE 0.001

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

struct half_edge * emplace_edge(struct edge_list * el){
  assert(el != NULL);
  struct half_edge * he = emplace_half_edge(el);
  if(he == NULL){
    return NULL;
  }
  struct half_edge * tw = emplace_half_edge(el);
  if(tw == NULL){
    return NULL;
  }
  he->twin = tw;
  tw->twin = he;
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

void connect_half_edges(struct half_edge * first, struct half_edge * second){
  assert(first != NULL);
  assert(second != NULL);
  assert(first->next == NULL);
  assert(second->prev == NULL);
  assert(first->face == second->face);

  first->next = second;
  second->prev = first;

  struct face * face = first->face;
  
  if(face != NULL){
    if(face->head == second){
      if(face->tail != first){
	face->head = first;
      }
    }
    if(face->tail == first){
      if(face->head != second){
	face->tail = second;
      }
    }
  }
}

static bool is_near(double value, double target, double tolerance){
  assert(tolerance >= 0);
  return value > (target - tolerance) && value < (target + tolerance);
}

static bool is_within_interval(double value, double interval, double tolerance){
  assert(tolerance >= 0);
  return value >= - tolerance && value <= (interval + tolerance);
}

static void fix_to_bounds(struct vertex * v, double width, double height, double tolerance){
  assert(v != NULL);
  assert(tolerance >= 0);

  if(is_near(v->x, 0, tolerance)){
    v->x = 0;
  }else if(is_near(v->x, width, tolerance)){
    v->x = width;
  }
  if(is_near(v->y, 0, tolerance)){
    v->y = 0;
  }else if(is_near(v->y, height, tolerance)){
    v->y = height;
  }
}

bool project_half_edge_on_bounds(struct edge_list * el, struct half_edge * he, double ex, double ey, double edx, double edy, double width, double height){
  assert(he != NULL);
  assert(he->twin != NULL);
  assert(he->twin->vertex == NULL);
  assert(width != 0);
  assert(height != 0);
  
  double points[] = {0, 0, width, height};
  double dirs[] = {0, 1, 1, 0};
  
  struct linear2 sys;
  
  for(int pi = 0; pi < 4; pi+=2){
    for(int di = 0; di < 4; di+=2){
      set_linear2_col(&sys, 0, edx, edy);
      set_linear2_col(&sys, 1, - dirs[di] , - dirs[di+1]);
      set_linear2_col(&sys, 2, ex - points[pi], ey - points[pi+1]);
      if(!solve_linear2(&sys)){
	/*
	 * Since this are half edges, the direction vertices are pointed away from the cells
	 * This means that we can distinguish between the two intersection solutions 
	 * by only checking the one with k > 0
	 */
	if(sys.vars[0] >= 0){
	  double x = ex + sys.vars[0] * edx;
	  double y = ey + sys.vars[0] * edy;
	  if(is_within_interval(x, width, TOLERANCE) && is_within_interval(y, height, TOLERANCE)){
	    //intersection within bounding box
	    struct vertex * vertex = emplace_vertex(el);
	    if(vertex == NULL){
	      return true;
	    }
	    vertex->x = x;
	    vertex->y = y;
	    fix_to_bounds(vertex, width, height, TOLERANCE);
	    he->twin->vertex = vertex;
	    return false;
	  }
	}
      }
    }
  }
  set_status(STATUS_NO_SOLUTION);
  return true;
}

bool close_face_with_bounds(struct edge_list * el, struct face * face, double width, double height){
  assert(el != NULL);
  assert(face != NULL);
  assert(face->head != NULL);
  
  if(face->tail == NULL){
    // no edges to close
    return false;
  }
  if(face->tail->next == face->head){
    // face already closed
    return false;
  }

  struct half_edge * he = face->tail;
  assert(he->twin != NULL);
  assert(he->twin->vertex != NULL);
  struct vertex * end = he->twin->vertex;
  
  assert(face->head->vertex != NULL);
  struct vertex * target = face->head->vertex;

  while(end != target){
    
    double nx;
    double ny;
    
    //the gap must be at the edges of the diagram
    assert(end->x == 0 || end->x == width || end->y == 0 || end->y == height);

    if(end->x == 0 && end->y != 0){
      // end lies on left bound and is not the top left corner
      nx = 0;
      // check if start lies on the left bound upward
      if(target->x == 0 && target->y < end->y){
	//close to end
	ny = target->y;
      }else{
	//add edge to top left corner
	ny = 0;
      }
    }else if(end->y == 0 && end->x != width){
      //end lies on the top bound and is not the top right corner
      ny = 0;
      if(target->y == 0 && target->x > end->x){
	//close to end
	nx = target->y;
      }else{
	// add edge to top right corner
	nx = width;
      }
    }else if(end->x == width && end->y != height){
      //end lies on the right bound and is not the bottom right corner
      nx = width;
      if(target->x == width && target->y > end->y){
	//close to end
	ny = target->y;
      }else{
	// add edge to bottom right corner
	ny = height;
      }
    }else{
      assert(end->y == height && end->x != 0);
      // end lies on the bottom bound but is not the bottom left corner
      ny = height;
      if(target->y == height && target->x < end->x){
	//close to end
	nx = target->x;
      }else{
	//add edge to bottom left corner
	nx = 0;
      }
    }

    struct vertex * next;
    struct half_edge * next_he;
    if(nx != target->x || ny != target->y){
      next = emplace_vertex(el);
      if(next == NULL){
	return true;
      }
      next->x = nx;
      next->y = ny;
    }else{
      next = target;
    }
    next_he = emplace_edge(el);
    if(next_he == NULL){
      return true;
    }
    next_he->vertex = end;
    next_he->twin->vertex = next;
    next_he->face = he->face;
    connect_half_edges(he, next_he);
    he = next_he;
    end = next;
  }
  face->tail = he;
  connect_half_edges(he, face->head);
  return false;
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
  if(he != NULL){
    do{
      print_half_edge(he);
      he = he->next;
    }while(he != face->head && he != NULL);
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

