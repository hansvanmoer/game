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

#include "image_io.h"
#include "memory.h"
#include "render.h"

#include <assert.h>
#include <math.h>

#include <png.h>

bool init_surface(struct surface * s, size_t width, size_t height){
  assert(s != NULL);
  size_t len = width * height;
  struct pixel * pixels = malloc_checked(sizeof(struct pixel) * len);
  if(pixels == NULL){
    return true;
  }
  s->pixels = pixels;
  s->width = width;
  s->height = height;
  s->len = len;
  struct pixel c = { 0, 0, 0};
  s->clear_color = c;
  s->color = c;
  return false;
}

void set_surface_color(struct surface * s, uint8_t r, uint8_t g, uint8_t b){
  assert(s != NULL);
  struct pixel c = {r, g, b};
  s->color = c;
}

void set_surface_clear_color(struct surface * s, uint8_t r, uint8_t g, uint8_t b){
  assert(s != NULL);
  struct pixel c = {r, g, b};
  s->clear_color = c;
}

void clear_surface(struct surface * s){
  assert(s != NULL);
  for(size_t i = 0; i < s->len; ++i){
    s->pixels[i] = s->clear_color;
  }
}

static void plot_pixel(struct surface * s, int x, int y){
  assert(s != NULL);
  if(x >= 0 && x < (int)s->width && y >= 0 && y < (int)s->height){
    s->pixels[x + y * s->width] = s->color;
  }
}

static void plot_line_low(struct surface * s, int x1, int y1, int x2, int y2){
  assert(s != NULL);
  assert(x1 < x2);
  int dx = x2 - x1;
  int dy = y2 - y1;
  int sy;
  if(dy < 0){
    sy = -1;
    dy = - dy;
  }else{
    sy = 1;
  }
  int dif = (2 * dy) - dx;
  int y = y1;
  for(int x = x1; x <= x2; ++x){
    plot_pixel(s, x, y);
    if(dif > 0){
      y += sy;
      dif+= 2*(dy - dx);
    }else{
      dif+= 2*dy;
    }
  }
}

static void plot_line_high(struct surface * s, int x1, int y1, int x2, int y2){
  assert(s != NULL);
  assert(y1 < y2);

  int dx = x2 - x1;
  int dy = y2 - y1;
  int sx;
  if(dx < 0){
    sx = -1;
    dx = - dx;
  }else{
    sx = 1;
  }
  int dif = (2 * dx) - dy;
  int x = x1;
  for(int y = y1; y <= y2; ++y){
    plot_pixel(s, x, y);
    if(dif > 0){
      x += sx;
      dif+= 2*(dx - dy);
    }else{
      dif+= 2*dx;
    }
  }
}

static void plot_line_hor(struct surface * s, int x1, int x2, int y){
  assert(s != NULL);
  assert(x1 < x2);

  for(int x = x1; x <= x2; ++x){
    plot_pixel(s, x, y);
  }
}

static void plot_line_ver(struct surface * s, int x, int y1, int y2){
  assert(s != NULL);
  assert(y1 < y2);

  for(int y = y1; y <= y2; ++y){
    plot_pixel(s, x, y);
  }
}

void draw_line(struct surface * s, double xs, double ys, double xe, double ye){
  assert(s != NULL);

  int x1 = (int)round(xs);
  int y1 = (int)round(ys);
  int x2 = (int)round(xe);
  int y2 = (int)round(ye);

  // find degenerate cases
  if(x1 == x2){
    if(y1 == y2){
      plot_pixel(s, x1, y1);
    }else{
      if(y1 > y2){
	int tmp = y1;
	y1 = y2;
	y2 = tmp;
      }
      plot_line_ver(s, x1, y1, y2);
    }
  }else if(y1 == y2){
    if(x1 > x2){
      int tmp = x1;
      x1 = x2;
      x2 = tmp;
    }
    plot_line_hor(s, x1, x2, y1);
  }else{
    // check slope
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    if(dx >= dy){
      if(x1 > x2){
	int tmp = x1;
	x1 = x2;
	x2 = tmp;
	tmp = y1;
	y1 = y2;
	y2 = tmp;
      }
      plot_line_low(s, x1, y1, x2, y2);
    }else{
      if(y1 > y2){
	int tmp = x1;
	x1 = x2;
	x2 = tmp;
	tmp = y1;
	y1 = y2;
	y2 = tmp;
      }
      plot_line_high(s, x1, y1, x2, y2);
    }
  }
}

void draw_point(struct surface * s, double x, double y){
  int px = (int)x;
  int py = (int)y;
  int r = (int)s->width;
  int t = (int)s->height;
  if(px >= 0 && px < r && py >= 0 && py < t){
    s->pixels[px + py * s->width] = s->color;
  }
}

void fill_rect(struct surface * s, double x, double y, double w, double h){
  assert(s != NULL);
  if(w < 0){
    x-= w;
    w = -w;
  }
  if(h < 0){
    y-= h;
    h = -h;
  }
  int sx = (int)x;
  int sy = (int)y;
  int ex = (int)(x+w);
  int ey = (int)(y+h);
  for(int px = sx; px < ex; ++px){
    for(int py = sy; py < ey; ++py){
      plot_pixel(s, px, py);
    }
  }
}

bool write_surface(FILE * file, struct surface * s){
  assert(s != NULL);
  assert(file != NULL);
  return write_pixels(file, s->pixels, s->width, s->height);
}

void dispose_surface(struct surface * s){
  assert(s != NULL);
  free(s->pixels);
}
