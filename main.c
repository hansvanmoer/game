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
#include "status.h"
#include "voronoi.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char * args[]){
  if(create_voronoi_diagram(10, 1000, 1000)){
    fprintf(stderr, "an error occurred: %s\n", get_status_msg(get_status()));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
