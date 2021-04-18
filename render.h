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

#ifndef RENDER_H
#define RENDER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct pixel{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct surface{
  struct pixel * pixels;
  size_t width;
  size_t height;
  size_t len;

  struct pixel color;
  struct pixel clear_color;
};

bool init_surface(struct surface * s, size_t width, size_t height);

void set_surface_color(struct surface * s, uint8_t r, uint8_t g, uint8_t b);

void set_surface_clear_color(struct surface * s, uint8_t r, uint8_t g, uint8_t b);

void clear_surface(struct surface * s);

void draw_line(struct surface * s, double xs, double ys, double xe, double ye);

void draw_point(struct surface * s, double x, double y);

bool write_surface(FILE * dest, struct surface * s);

void dispose_surface(struct surface * s);

#endif
