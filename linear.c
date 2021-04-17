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

#include <assert.h>
#include <stdio.h>

void print_linear2(const struct linear2 * sys){
  assert(sys != NULL);
  printf("%.4f x + %.4f y + %.4f = 0\n", sys->coefs[0], sys->coefs[1], sys->coefs[2]);
  printf("%.4f x + %.4f y + %.4f = 0\n", sys->coefs[3], sys->coefs[4], sys->coefs[5]);
  printf("x: %.4f, y: %4f\n", sys->vars[0], sys->vars[0]);
}

void set_linear2_col(struct linear2 * sys, size_t index, double x, double y){
  assert(sys != NULL);
  assert(index < 3);
  sys->coefs[index] = x;
  sys->coefs[index+3] = y;
}

void set_linear2_row(struct linear2 * sys, size_t index, double a, double b, double c){
  assert(sys != NULL);
  assert(index < 3);
  sys->coefs[index*3] = a;
  sys->coefs[index*3+1] = b;
  sys->coefs[index*3+2] = c;
}

bool solve_linear2(struct linear2 * sys){
  assert(sys != NULL);
  
  double det = sys->coefs[0] * sys->coefs[4] - sys->coefs[1] * sys->coefs[3];
  double x;
  double y;
  if(det == 0){
    double d2 = sys->coefs[0] * sys->coefs[5] - sys->coefs[3] * sys->coefs[2];
    if(d2 == 0){
      set_status(STATUS_INF_SOLUTIONS);
    }else{
      set_status(STATUS_NO_SOLUTION);
    }
    return true;
  }
  if(sys->coefs[0] == 0){
    y = - sys->coefs[2] / sys->coefs[1];
    x = - (sys->coefs[5] - sys->coefs[4] * y) / sys->coefs[3];
  }else{
    double dy = sys->coefs[2] * sys->coefs[3] - sys->coefs[0] * sys->coefs[5];
    y = dy / det;
    x = - (sys->coefs[2] + sys->coefs[1] * y) / sys->coefs[0];
  }
  sys->vars[0] = x;
  sys->vars[1] = y;
  return false;
}
