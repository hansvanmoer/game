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


#include "client.h"
#include "edge_list.h"
#include "logger.h"
#include "program.h"
#include "render.h"
#include "server.h"
#include "settings.h"
#include "signal_utils.h"
#include "status.h"
#include "voronoi.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static bool draw_edge_list(struct edge_list * el){
  assert(el != NULL);

  struct surface s;
  init_surface(&s, 1001, 1001);
  set_surface_clear_color(&s, 0, 0, 0);
  clear_surface(&s);
  
  for(struct face * f = el->head; f != NULL; f = f->next){
    assert(f->head != NULL);
    set_surface_color(&s, 255, 255, 0);
    fill_rect(&s, f->x - 3, f->y - 3, 7, 7);
    
    set_surface_color(&s, 255, 255, 255);
    if(f->head != NULL){
      struct half_edge * he = f->head;
      do{
	struct vertex * start = he->vertex;
	assert(start != NULL);
	assert(he->twin != NULL);
	struct vertex * end = he->twin->vertex;
	assert(end != NULL);
	
	draw_line(&s, start->x, start->y, end->x, end->y);
	he = he->next;
      }while(he != f->head);
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


int main(int argc, char * const args[]){
  
  struct program_settings settings = {false, false, LOG_PRIORITY_ERROR};
  if(load_program_settings(&settings, argc, args)){
    fprintf(stderr, "an error occurred: '%s'\n", get_status_msg(get_status()));
    return EXIT_FAILURE;
  }

  if(init_signals()){
    fputs("could not initialize signal handler", stderr);
    return EXIT_FAILURE;
  }
  
  if(start_logger(stdout)){
    fputs("unable to start logger\n", stderr);
    return EXIT_FAILURE;
  }
  
  set_min_log_priority(settings.log_priority);
  
  log_program_settings(&settings);

  if(run_program_loop(&settings)){
    LOG_ERROR("program loop terminated with errors");
  }
  
  stop_logger();
  
  return EXIT_SUCCESS;
}
