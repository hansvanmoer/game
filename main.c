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
#include "render.h"
#include "status.h"
#include "voronoi.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static bool draw_edge_list(struct edge_list * el){
  assert(el != NULL);

  struct surface s;
  init_surface(&s, 1000, 1000);
  set_surface_clear_color(&s, 0, 0, 0);
  clear_surface(&s);
  
  for(struct face * f = el->head; f != NULL; f = f->next){
    assert(f->head != NULL);
    set_surface_color(&s, 255, 255, 0);
    draw_point(&s, f->x, f->y);
    
    set_surface_color(&s, 255, 255, 255);
    for(struct half_edge * he = f->head; he != NULL; he = he->next){
      struct vertex * start = he->vertex;
      assert(start != NULL);
      assert(he->twin != NULL);
      struct vertex * end = he->twin->vertex;
      assert(end != NULL);

      draw_line(&s, start->x, start->y, end->x, end->y);
    }
  }

  FILE * file = fopen("output.png", "w");
  if(file == NULL){
    set_status(STATUS_IO_ERROR);
    dispose_surface(&s);
    return true;
  }

  bool result = write_surface(file, &s);
  fclose(file);
  dispose_surface(&s);
  return result;
}

static bool test_voronoi_diagram(){
  
  struct edge_list el;
  init_edge_list(&el);

  if(create_voronoi_diagram(&el, 10, 1000, 1000)){
    dispose_edge_list(&el);
    return true;
  }

  puts("result:");
  print_edge_list(&el);
  
  bool result = draw_edge_list(&el);
  
  dispose_edge_list(&el);
  return result;
}

int main(int argc, const char * args[]){
  
  if(test_voronoi_diagram()){
    printf("an error occurred: '%s'", get_status_msg(get_status()));
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
