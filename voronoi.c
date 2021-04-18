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

#include "linear.h"
#include "random.h"
#include "status.h"
#include "voronoi.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#define FP_TOLERANCE 0.0001

static void init_diagram(struct diagram * diag, struct edge_list * el, double width, double height){
  assert(diag != NULL);
  assert(el != NULL);

  diag->el = el;
  
  init_deque(&diag->nodes, sizeof(struct node), 0);
  init_deque(&diag->events, sizeof(struct event), 0);

  diag->width = width;
  diag->height = height;
  diag->root_node = NULL;
  diag->root_event = NULL;
}

static void dispose_diagram(struct diagram * diag){
  assert(diag != NULL);

  dispose_deque(&diag->nodes);
  dispose_deque(&diag->events);
}

static double get_priority(struct event * event){
  assert(event != NULL);

  if(event->type == EVENT_TYPE_ADD_ARC){
    return event->add_arc.face->y;
  }else{
    return event->remove_arc.priority;
  }
}

static void insert_event(struct diagram * diag, struct event * event){
  assert(diag != NULL);
  assert(event != NULL);
  assert(event->parent == NULL);
  assert(event->left == NULL);
  assert(event->right == NULL);

  if(diag->root_event == NULL){
    diag->root_event = event;
  }else{
    double prio = get_priority(event);
    struct event * e = diag->root_event;
    while(true){
      double p = get_priority(e);
      if(prio <= p){
	if(e->left == NULL){
	  event->parent = e;
	  e->left = event;
	  break;
	}else{
	  e = e->left;
	}
      }else{
	if(e->right == NULL){
	  event->parent = e;
	  e->right = event;
	  break;
	}else{
	  e = e->right;
	}
      }
    }
  }
}

static struct event * get_min_event(struct event * event){
  assert(event != NULL);
  struct event * e = event;
  while(e->left != NULL){
    e = e->left;
  }
  return e;
}

static struct event * get_first_event(struct diagram * diag){
  assert(diag != NULL);

  struct event * e = diag->root_event;
  if(e != NULL){
    e = get_min_event(e);
  }
  return e;
}

static struct event * pop_event(struct diagram * diag){
  assert(diag != NULL);
  struct event * head = get_first_event(diag);
  if(head == NULL){
    return NULL;
  }else{
    assert(head->left == NULL);
    if(head->parent == NULL){
      diag->root_event = head->right;
      if(head->right != NULL){
	head->right->parent = NULL;
      }
    }else{
      assert(head->parent->left == head);
      head->parent->left = head->right;
      if(head->right != NULL){
	head->right->parent = head->parent;
      }
    }
    return head;
  }
}

static struct event * get_next_event(struct event * event){
  assert(event != NULL);

  if(event->right != NULL){
    return get_min_event(event->right);
  }else{
    while(event->parent != NULL && event->parent->left != event){
      event = event->parent;
    }
    return event->parent;
  }
}

static void detach_event(struct diagram * diag, struct event * event){
  assert(diag != NULL);
  assert(event != NULL);
  assert(event->type == EVENT_TYPE_REMOVE_ARC);
  assert(event->left == NULL);
  assert(event->right == NULL);

  if(event->parent == NULL){
    diag->root_event = NULL;
  }else if(event->parent->left == event){
    event->parent->left = NULL;
  }else{
    event->parent->right = NULL;
  }
}

static void replace_event(struct diagram * diag, struct event * event, struct event * child){
  assert(diag != NULL);
  assert(event != NULL);
  assert(child != NULL);
  assert(event->type == EVENT_TYPE_REMOVE_ARC);
  assert(event->left == child || event->right == child);

  if(event->parent == NULL){
    diag->root_event = child;
    child->parent = NULL;
  }else if(event->parent->left == event){
    event->parent->left = child;
    child->parent = event->parent;
  }else{
    event->parent->right = child;
    child->parent = event->parent;
  }
}

static void copy_event(struct diagram * diag, struct event * dest, struct event * src){
  assert(diag != NULL);
  assert(dest != NULL);
  assert(src != NULL);
  assert(dest->type == EVENT_TYPE_REMOVE_ARC);

  if(src->type == EVENT_TYPE_ADD_ARC){
    dest->add_arc = src->add_arc;
    dest->type = EVENT_TYPE_ADD_ARC;
  }else{
    dest->remove_arc = src->remove_arc;
  }
}

static void remove_event(struct diagram * diag, struct event * event){
  assert(diag != NULL);
  assert(event != NULL);
  if(event->left == NULL){
    if(event->right == NULL){
      detach_event(diag, event);
    }else{
      replace_event(diag, event, event->right);
    }
  }else{
    if(event->right == NULL){
      replace_event(diag, event, event->left);
    }else{
      struct event * next = get_next_event(event);
      assert(next != NULL);
      copy_event(diag, event, next);
      remove_event(diag, next);
    }
  }
}


static struct node * get_min_node(struct node * node){
  assert(node != NULL);

  while(node->left != NULL){
    node = node->left;
  }
  return node;
}


static struct node * get_max_node(struct node * node){
  assert(node != NULL);

  while(node->right != NULL){
    node = node->right;
  }
  return node;
}

static struct node * get_first_node(struct diagram * diag){
  assert(diag != NULL);
  if(diag->root_node == NULL){
    return NULL;
  }else{
    return get_min_node(diag->root_node);
  }
}

static struct node * get_prev_node(struct node * node){
  assert(node != NULL);
  if(node->left != NULL){
    return get_max_node(node->left);
  }else{
    while(node->parent != NULL && node->parent->right != node){
      node = node->parent;
    }
    return node->parent;
  }
}


static struct node * get_next_node(struct node * node){
  assert(node != NULL);
  if(node->right != NULL){
    return get_min_node(node->right);
  }else{
    while(node->parent != NULL && node->parent->left != node){
      node = node->parent;
    }
    return node->parent;
  }
}

static void replace_child(struct diagram * diag, struct node * parent, struct node * old_child, struct node * new_child){
  assert(diag != NULL);
  assert(old_child != NULL);
  assert(new_child != NULL);
  if(parent == NULL){
    diag->root_node = new_child;
  }else{
    if(parent->left == old_child){
      parent->left = new_child;
    }else{
      assert(parent->right == old_child);
      parent->right = new_child;
    }
  }
  new_child->parent = parent;
}

static bool add_faces(struct diagram * diag, size_t face_count){
  assert(diag != NULL);
  assert(face_count != 0);
  assert(diag->width > 0);
  assert(diag->height > 0);

  double pts[] = {
		  400, 400,
		  200, 600,
		  600, 650
  };
  face_count = sizeof(pts) / (2*sizeof(double));
  
  for(size_t i = 0; i < face_count; ++i){
    struct face * f = emplace_face(diag->el);
    if(f == NULL){
      return true;
    }
    //f->x = rand_double_range(0, diag->width);
    //f->y = rand_double_range(0, diag->height);
    f->x = pts[i*2];
    f->y = pts[i*2+1];

    struct event * e = emplace_onto_deque(&diag->events);
    if(e == NULL){
      return true;
    }
    e->type = EVENT_TYPE_ADD_ARC;
    e->add_arc.face = f;
    e->parent = NULL;
    e->left = NULL;
    e->right = NULL;
    insert_event(diag, e);
  }
  return false;
}

struct parabola{
  double a;
  double b;
  double c;
};

static void get_parabola(struct parabola *p, struct face * face, double ly){
  assert(p != NULL);
  assert(face != NULL);
  
  /*
   * dist²(P, S) = dist²(P, L)
   * (x - sx)² + (y - sy)² = (y - ly)²
   * x² + y² + sx² + sy² - 2*sx*x - 2*sy*y  = y² - 2*ly*y + ly²
   * x² + sx² + sy² - 2*sx*x  - ly²=  2*(sy - ly)*y
   * y = (1 / (2*(sy - ly)))*x² + (sx / (sy - ly))*x + (sx² + sy² - ly²) / (2 * (sy - ly))
   *
   * a = 1 / (2*(sy - ly))
   * b = sx / (sy - ly)
   * c = (sx² + sy² - ly²) / (2 * (sy - ly))
   */
  p->a = 1.0 / (2.0 * (face->y - ly));
  p->b = - face->x / (face->y - ly);
  p->c = (face->x*face->x + face->y*face->y - ly*ly) * p->a;
  //printf("parabola around point (%.4f, %.4f) with line y = %.4f: %.4f x² + %.4f x + %.4f\n",face->x, face->y, ly, p->a, p->b, p->c);
}


static double get_intersection_x(struct face * left, struct face * right, double ly){
  assert(left != NULL);
  assert(right != NULL);

  struct parabola p;
  get_parabola(&p, left, ly);
  double a = p.a;
  double b = p.b;
  double c = p.c;
  get_parabola(&p, right, ly);
  a-=p.a;
  b-=p.b;
  c-=p.c;

  //printf("left site (%.2f, %.2f)\n", left->x, left->y);
  //printf("right site (%.2f, %.2f)\n", right->x, right->y);
  //printf("scan line: y  = %.2f\n", ly);
  //printf("solutions for %+.4f x² %+.2f x %+.2f\n", a, b, c);

  double discq = b*b - 4.0*a*c;
  assert(discq > 0);
  double disc = sqrt(discq);
  double x1 = (- b - disc) / (2.0 * a);
  double x2 = (- b + disc) / (2.0 * a);

  if(x2 < x1){
    double tmp = x1;
    x1 = x2;
    x2 = tmp;
  }

  if(left->y < right->y){
    return x1;
  }else{
    return x2;
  }
}

static double get_y(struct face * face, double x, double ly){
  assert(face != NULL);

  struct parabola p;
  get_parabola(&p, face, ly);
  
  double y = x*x*p.a + x*p.b + p.c;
  printf("P: (%.4f, %.4f)\n", x, y);
  return y;
}

static bool check_for_remove_events(struct diagram * diag, struct node * node, double sy){
  assert(diag != NULL);
  assert(node != NULL);
  assert(node->type == NODE_TYPE_ARC);
  struct node * left = get_prev_node(node);
  if(left == NULL){
    return false;
  }
  assert(left->type == NODE_TYPE_HALF_EDGE);
  struct node * right = get_next_node(node);
  if(right == NULL){
    return false;
  }
  assert(right->type == NODE_TYPE_HALF_EDGE);

  struct linear2 sys;
  set_linear2_col(&sys, 0, left->half_edge.dx, left->half_edge.dy);
  set_linear2_col(&sys, 1, - right->half_edge.dx, - right->half_edge.dy);
  set_linear2_col(&sys, 2, left->half_edge.x - right->half_edge.x, left->half_edge.y - right->half_edge.y);
  if(solve_linear2(&sys)){
    print_linear2(&sys);
    return true;
  }

  double x = left->half_edge.x + sys.vars[0] * left->half_edge.dx;
  double y = left->half_edge.y + sys.vars[0] * left->half_edge.dy;
  printf("solution: (%.4f, %.4f) sy = %.4f\n", x, y, sy);
  double dx = node->arc.face->x - x;
  double dy = node->arc.face->y - y;
  double ey = y + sqrt(dx*dx+dy*dy);
  
  //only interested if the sides actually converge
  if(ey > sy){
    printf("arc will be removed at y = %.4f\n", ey);
    //remove previous event
    if(node->arc.event != NULL){
      remove_event(diag, node->arc.event);
    }
    
    struct event * event = emplace_onto_deque(&diag->events);
    if(event == NULL){
      return true;
    }

    event->type = EVENT_TYPE_REMOVE_ARC;
    event->remove_arc.node = node;
    event->remove_arc.x = x;
    event->remove_arc.y = y;
    event->remove_arc.priority = ey;
    event->parent = NULL;
    event->left = NULL;
    event->right = NULL;
    insert_event(diag, event);
    node->arc.event = event;
  }
  return false;
}

/*
 * Creates a pair of open ended half edges for a split arc and a new one
 * Note: this function should be called AFTER the new arc has been introduced into the scanline, not before
 * split_arc = the arc that was split, now the arc to the left of the new arc
 * new_arc = the newly introduced arc
 * left_edge_node = the edge between the split arc and the new arc
 * right_edge_node = the edge between the new arc and the clone arc
 */
static bool create_edge_after_insert_arc(struct diagram * diag, struct node * split_arc, struct node * new_arc, struct node * left_edge_node, struct node * right_edge_node){
  assert(diag != NULL);
  assert(split_arc != NULL);
  assert(split_arc->type == NODE_TYPE_ARC);
  assert(new_arc != NULL);
  assert(new_arc->type == NODE_TYPE_ARC);
  assert(left_edge_node != NULL);
  assert(left_edge_node->type == NODE_TYPE_HALF_EDGE);
  assert(right_edge_node != NULL);
  assert(right_edge_node->type == NODE_TYPE_HALF_EDGE);
  
  struct half_edge * left_he = emplace_half_edge(diag->el);
  if(left_he == NULL){
    return true;
  }
  struct half_edge * right_he = emplace_half_edge(diag->el);
  if(right_he == NULL){
    return true;
  }
  left_he->twin = right_he;

  right_he->twin = left_he;
  
  struct face * split_face = split_arc->arc.face;
  struct node * prev_node = get_prev_node(split_arc);
  if(prev_node == NULL){
    // add the left half edge at the end of the split arc's face
    set_tail_half_edge(split_face, left_he);
  }else{
    assert(prev_node->type == NODE_TYPE_HALF_EDGE);
    // add the left half edge before 
    struct half_edge * prev_he = prev_node->half_edge.half_edge;
    if(prev_he->face != split_face){
      prev_he = prev_he->twin;
    }
    assert(prev_he->face == split_face);
    insert_half_edge_before(prev_he, left_he);
  }
  // the right half edge is always the first edge of the new face
  set_head_half_edge(new_arc->arc.face, right_he);
  
  left_edge_node->half_edge.half_edge = left_he;
  right_edge_node->half_edge.half_edge = right_he;
  return false;
}

static bool split_node(struct diagram * diag, struct node * split, struct node * node){
  assert(diag != NULL);
  assert(split != NULL);
  assert(split->type == NODE_TYPE_ARC);
  assert(node != NULL);
  assert(node->type == NODE_TYPE_ARC);
  assert(node->parent == NULL);
  assert(node->left == NULL);
  assert(node->right == NULL);


  printf("create half edges between (%.4f, %.4f) and (%.4f, %.4f)\n", split->arc.face->x, split->arc.face->y, node->arc.face->x, node->arc.face->y);

  double ly = node->arc.face->y;
  double x = node->arc.face->x;
  double y = get_y(split->arc.face, x, ly);
  double mx = node->arc.face->x - split->arc.face->x;
  double my = node->arc.face->y - split->arc.face->y;
  double dx;
  double dy;
  if(mx == 0){
    dx = 1;
    dy = 0;
  }else if(my == 0){
    dx = 0;
    dy = 1;
  }else{
    dx = 1;
    dy = - mx / my;
  }
  // note that these are half edges, so the direction vectors must point to opposite sides
  // this is necessary to ensure the bounding box intersection algorithm works
  // by convention, (dx, dy) is the vector extending to the right side
  assert(dx >= 0);
  
  //printf("seeking orthogonal vector to (%.4f,%.4f) -> (%.4f, %.4f)\n", mx, my, dx, dy);
  assert(dx * mx + dy * my == 0);
  
  struct node * copy = emplace_onto_deque(&diag->nodes);
  if(copy == NULL){
    return true;
  }
  copy->type = NODE_TYPE_ARC;
  copy->arc.face = split->arc.face;
  copy->arc.event = NULL;

  copy->left = NULL;
  copy->right = NULL;
 
  struct node * le = emplace_onto_deque(&diag->nodes);
  if(le == NULL){
    return true;
  }
  le->type = NODE_TYPE_HALF_EDGE;
  le->half_edge.x = x;
  le->half_edge.y = y;
  le->half_edge.dx =  - dx; // the inverse of the direction vector
  le->half_edge.dy =  - dy;
  printf("create two half edges from (%.4f, %.4f) in direction (%.4f, %.4f)\n", x, y, dx, dy); 
  replace_child(diag, split->parent, split, le);
  le->left = split;
  split->parent = le;

  struct node * re = emplace_onto_deque(&diag->nodes);
  if(re == NULL){
    return true;
  }
  re->type = NODE_TYPE_HALF_EDGE;
  re->half_edge.x = x;
  re->half_edge.y = y;
  re->half_edge.dx = dx;
  re->half_edge.dy = dy;
  
  re->parent = le;
  le->right = re;
  re->left = node;
  node->parent = re;
  re->right = copy;
  copy->parent = re;
  
  if(check_for_remove_events(diag, split, ly)){
    return true;
  }
  if(check_for_remove_events(diag, copy, ly)){
    return true;
  }

  if(create_edge_after_insert_arc(diag, split, node, le, re)){
    return true;
  }
  
  return false;
}

static bool insert_node(struct diagram * diag, struct node * node, double x, double y){
  assert(diag != NULL);
  assert(node != NULL);
  assert(diag->root_node != NULL);
  struct node * search = diag->root_node;
  while(search->type != NODE_TYPE_ARC){
    assert(search->left != NULL);
    assert(search->right != NULL);
    struct node * left = get_max_node(search->left);
    assert(left != NULL);
    assert(left->type == NODE_TYPE_ARC);
    struct node * right = get_min_node(search->right);
    assert(right != NULL);
    assert(right->type == NODE_TYPE_ARC);
    
    double xi = get_intersection_x(left->arc.face, right->arc.face, y);
    if(x < xi){
      search = search->left;
    }else{
      search = search->right;
    }
  }

  assert(search != NULL);
  assert(search->type == NODE_TYPE_ARC);
  return split_node(diag, search, node);
}

static bool handle_add_arc_event(struct diagram * diag, struct face * face){
  assert(diag != NULL);
  assert(face != NULL);

  struct node * node = emplace_onto_deque(&diag->nodes);
  if(node == NULL){
    return true;
  }
  node->parent = NULL;
  node->left = NULL;
  node->right = NULL;
  node->type = NODE_TYPE_ARC;
  node->arc.face = face;
  node->arc.event = NULL;
  
  if(diag->root_node == NULL){
    diag->root_node = node;
    return false;
  }else{
    return insert_node(diag, node, face->x, face->y);
  }
}

static void replace_node(struct node * node, struct node * child){
  assert(node != NULL);
  assert(node->type == NODE_TYPE_HALF_EDGE);
  assert(node->parent != NULL); // can not be root because the other edge must be an ancestor
  assert(child != NULL);
  assert(child->parent == node);
  assert(node->left == child || node->right == child);
  if(node->parent->left == node){
    node->parent->left = child;
  }else{
    node->parent->right = child;
  }
  child->parent = node->parent;
}

static bool close_half_edges(struct diagram * diag, struct node * left_he_node, struct node * right_he_node, double end_x, double end_y){
  assert(diag != NULL);
  assert(left_he_node != NULL);
  assert(left_he_node->type == NODE_TYPE_HALF_EDGE);
  assert(right_he_node != NULL);
  assert(right_he_node->type == NODE_TYPE_HALF_EDGE);

  struct vertex * vertex = emplace_vertex(diag->el);
  if(vertex == NULL){
    return true;
  }
  vertex->x = end_x;
  vertex->y = end_y;

  left_he_node->half_edge.half_edge->vertex = vertex;
  right_he_node->half_edge.half_edge->vertex = vertex;
  return false;
}

/*
 * Creates a half edge pair but closes one and sets the other on the new half edge node
 */
static bool create_edge_after_remove_arc(struct diagram * diag, struct half_edge * left_he, struct half_edge * right_he, struct node * he_node){
  assert(diag != NULL);
  assert(left_he != NULL);
  assert(right_he != NULL);
  assert(he_node != NULL);
  assert(he_node->type == NODE_TYPE_HALF_EDGE);

  struct vertex * vertex = emplace_vertex(diag->el);
  if(vertex == NULL){
    return true;
  }
  struct half_edge * closed = emplace_half_edge(diag->el);
  if(closed == NULL){
    return true;
  }
  struct half_edge * open = emplace_half_edge(diag->el);
  if(open == NULL){
    return true;
  }
  vertex->x = he_node->half_edge.x;
  vertex->y = he_node->half_edge.y;  
  
  closed->vertex = vertex;
  closed->twin = open;

  open->twin = closed;

  //closed is always attached to the right arc's face, open to the left
  insert_half_edge_after(left_he, open);
  insert_half_edge_before(right_he, closed);

  he_node->half_edge.half_edge = open;
  
  return false;
}

static bool handle_remove_arc_event(struct diagram * diag, struct node * node, double x, double y, double ly){
  assert(diag != NULL);
  assert(node != NULL);
  assert(node->type == NODE_TYPE_ARC);
  
  struct node * le = get_prev_node(node);
  assert(le != NULL);
  assert(le->type == NODE_TYPE_HALF_EDGE);
  struct node * la = get_prev_node(le);
  assert(la != NULL);
  assert(la->type == NODE_TYPE_ARC);
  struct node * re = get_next_node(node);
  assert(re != NULL);
  assert(re->type == NODE_TYPE_HALF_EDGE);
  struct node * ra = get_next_node(re);
  assert(ra != NULL);
  assert(ra->type == NODE_TYPE_ARC);

  struct face * face = node->arc.face;
  struct half_edge * left_he = le->half_edge.half_edge;
  if(left_he->face == face){
    left_he = left_he->twin;
  }
  struct half_edge * right_he = re->half_edge.half_edge;
  if(right_he->face == face){
    right_he = right_he->twin;
  }

  if(close_half_edges(diag, le, re, x, y)){
    return true;
  }
  
  printf("edge intersection: (%.4f, %.4f) and (%.4f,%.4f) to (%.4f,%.4f)\n", le->half_edge.x, le->half_edge.y, re->half_edge.x, re->half_edge.y, x, y);
  printf("create new edge between sites (%.4f, %.4f) and (%.4f,%.4f)\n", la->arc.face->x, la->arc.face->y, ra->arc.face->x, ra->arc.face->y);

  //one of the edges must be the parent of the arc to be removed
  assert(node->parent == le || node->parent == re);
  struct node *parent;
  struct node * anc;
  if(node->parent == re){
    parent = re;
    anc = le;
  }else{
    parent = le;
    anc = re;
  }

  //replace parent by the sibling arc or halfedge
  if(parent->left == node){
    replace_node(parent, parent->right);
  }else{
    replace_node(parent, parent->left);
  }

  //calculate a new edge and store it on the ancestor
  struct face * lf = la->arc.face;
  struct face * rf = ra->arc.face;
  assert(lf != rf);
  anc->half_edge.x = x;
  anc->half_edge.y  = y;
  double dx = lf->x - rf->x;
  double dy = lf->y - rf->y;
  double edx;
  double edy;
  if(dx == 0){
    edx = 1;
    edy = 0;
  }else if(dy == 0){
    edx = 0;
    edy = 1;
  }else{
    edx = 1;
    edy = - dx / dy;
    /* 
     * note that this is a half edge, so we must ensure that the direction vector
     * points away from the interection, i.e. dy >= 0
     * otherwise, the bounding box intersection algorithm does not work
     */
    if(edy < 0){
      edx = - edx;
      edy = - edy;
    }
  }
  assert(edy > 0);
  anc->half_edge.dx = edx;
  anc->half_edge.dy = edy;

  if(create_edge_after_remove_arc(diag, left_he, right_he, anc)){
    return true;
  }

  if(check_for_remove_events(diag, la, ly)){
    return true;
  }
  if(check_for_remove_events(diag, ra, ly)){
    return true;
  }
  return false;
}


static void print_event(struct event * event){
  assert(event != NULL);
  
  if(event->type == EVENT_TYPE_ADD_ARC){
    struct face * face = event->add_arc.face;
    assert(face != NULL);
    printf("add arc event for site (%.2f, %.2f)\n", face->x, face->y);
  }else{
    struct node * node = event->remove_arc.node;
    assert(node != NULL);
    assert(node->type == NODE_TYPE_ARC);
    struct face * face = node->arc.face;
    printf("remove arc event for site (%.2f, %.2f)\n", face->x, face->y); 
  }
}

static void print_events(struct diagram * diag){
  assert(diag != NULL);

  puts("events:");
  struct event * e = get_first_event(diag);
  while(e != NULL){
    print_event(e);
    e = get_next_event(e);
  }
}

static bool handle_event(struct diagram * diag, struct event * event){
  assert(diag != NULL);
  assert(event != NULL);
  
  printf("handle event at y = %.4f\n", get_priority(event));
  print_event(event);
  if(event->type == EVENT_TYPE_ADD_ARC){
    return handle_add_arc_event(diag, event->add_arc.face);
  }else{
    return handle_remove_arc_event(diag, event->remove_arc.node, event->remove_arc.x, event->remove_arc.y, event->remove_arc.priority);
  }
}

/**
 * TODO: do this better
 */
static bool is_within_bounds(double bound, double value){
  double d = FP_TOLERANCE * bound;
  return value > -d && value < (bound + d);
}

static bool is_close_to_bound(double bound, double max, double value){
  double d = FP_TOLERANCE * max;
  return value > bound - d && value < bound + d;
}

static void fix_to_bounds(struct diagram * diag, struct vertex * v){
  assert(diag != NULL);
  assert(v != NULL);

  if(is_close_to_bound(0, diag->width, v->x)){
    v->x = 0;
  }else if(is_close_to_bound(diag->width, diag->width, v->x)){
    v->x = diag->width;
  }
  if(is_close_to_bound(0, diag->height, v->y)){
    v->y = 0;
  }else if(is_close_to_bound(diag->height, diag->height, v->y)){
    v->y = diag->height;
  }
}

static bool intersect_with_bound(struct diagram * diag, struct node * he_node, double bx, double by, double bdx, double bdy){
  assert(diag != NULL);
  assert(he_node != NULL);
  assert(he_node->type == NODE_TYPE_HALF_EDGE);
  
  double ex = he_node->half_edge.x;
  double ey = he_node->half_edge.y;
  double edx = he_node->half_edge.dx;
  double edy = he_node->half_edge.dy;

  struct half_edge * he = he_node->half_edge.half_edge;
  assert(he != NULL);
  assert(he->vertex == NULL);
  
  // check left vertical edge
  struct linear2 sys;
  set_linear2_col(&sys, 0, edx, edy);
  set_linear2_col(&sys, 1, - bdx , - bdy);
  set_linear2_col(&sys, 2, ex - bx, ey - by);
  if(!solve_linear2(&sys)){
    /*
     * Since this are half edges, the direction vertices are pointed away from the cells
     * This means that we can distinguish between the two intersection solutions by only checking the one with k > 0
     */
    if(sys.vars[0] >= 0){
      double x = ex + sys.vars[0] * edx;
      double y = ey + sys.vars[0] * edy;
      if(is_within_bounds(diag->width, x) && is_within_bounds(diag->height, y)){
	//intersection within bounding box
	struct vertex * vertex = emplace_vertex(diag->el);
	if(vertex == NULL){
	  return true;
	}
	vertex->x = x;
	vertex->y = y;
	fix_to_bounds(diag, vertex);
	he->vertex = vertex;
	return false;
      }else{
	set_status(STATUS_NO_SOLUTION);
      }
    }else{
      set_status(STATUS_NO_SOLUTION);
    }
  }
  return true;
}

static bool close_open_half_edge(struct diagram * diag, struct node * he_node){
  assert(diag != NULL);
  assert(he_node != NULL);
  assert(he_node->type == NODE_TYPE_HALF_EDGE);
  assert(he_node->half_edge.half_edge != NULL);
  assert(he_node->half_edge.half_edge->vertex == NULL);

  double points[] = {0, 0, diag->width, diag->height};
  double dirs[] = {0, 1, 1, 0};

  for(int pi = 0; pi < 4; pi+=2){
    for(int di = 0; di < 4; di+=2){
      if(intersect_with_bound(diag, he_node, points[pi], points[pi + 1], dirs[di] ,  dirs[di+1])){
	if(get_status() != STATUS_NO_SOLUTION && get_status() != STATUS_INF_SOLUTIONS){
	  return true;
	}
      }else{
	return false;
      }
    }
  }
  // something is wrong: no intersection with any of the bounds
  set_status(STATUS_NO_SOLUTION);
  return true;
}

static bool close_open_half_edges(struct diagram * diag){
  assert(diag != NULL);

  struct node * prev = NULL;
  struct node * node = diag->root_node;
  while(node != NULL){
    if(node->type == NODE_TYPE_ARC){
      //nothing to do
      prev = node;
      node = node->parent;
    }else{
      assert(node->left != NULL);
      assert(node->right != NULL);
      if(prev == node->right){
	if(close_open_half_edge(diag, node)){
	  return true;
	}
	prev = node;
	node = node->parent;
      }else if(prev == node->left){
        node = node->right;
      }else{
	node = node->left;
      }
    }
  }
  return false;
}

static void print_node(struct node * node){
  assert(node != NULL);
  if(node->type == NODE_TYPE_ARC){
    struct face * f = node->arc.face;
    printf("arc node (%.2f, %.2f)\n", f->x, f->y); 
  }else{
    printf("half edge node (%.2f, %.2f)\n", node->half_edge.x, node->half_edge.y);
  }
}

static void print_nodes(struct diagram * diag){
  assert(diag != NULL);
  puts("nodes:");
  struct node * node = get_first_node(diag);
  while(node != NULL){
    print_node(node);
    node = get_next_node(node);
  }
}

static void print_diagram(struct diagram * diag){
  assert(diag != NULL);
  print_events(diag);
  print_nodes(diag);
  print_edge_list(diag->el);
}

bool create_voronoi_diagram(struct edge_list * result, size_t face_count, double width, double height){
  
  struct diagram diag;
  init_diagram(&diag, result, width, height);

  if(add_faces(&diag, face_count)){
    dispose_diagram(&diag);
    return true;
  }

  print_diagram(&diag);

  struct event * event;
  while((event = pop_event(&diag)) != NULL){
    if(handle_event(&diag, event)){
      return true;
    }
  }

  if(close_open_half_edges(&diag)){
    return true;
  }
  
  print_diagram(&diag);

  dispose_diagram(&diag);
  return false;
}
