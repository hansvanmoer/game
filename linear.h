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

#ifndef LINEAR_H
#define LINEAR_H

#include <stdbool.h>
#include <stdlib.h>

struct linear2{
  double coefs[6];
  double vars[2];
};

void print_linear2(const struct linear2 * sys);

void set_linear2_row(struct linear2 * sys, size_t index, double a, double b, double c);

void set_linear2_col(struct linear2 * sys, size_t index, double x, double y);

bool solve_linear2(struct linear2 * sys);

#endif
