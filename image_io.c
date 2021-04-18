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
#include "status.h"

#include <assert.h>
#include <png.h>
#include <setjmp.h>

bool write_pixels(FILE * file, struct pixel * pixels, size_t width, size_t height){
  assert(pixels != NULL);
  assert(file != NULL);
  
  if(width == 0 || height == 0){
    set_status(STATUS_INVALID_IMAGE_SIZE);
    return true;
  }
  
  png_structp png;
  png_infop info;
  png_byte ** rows;

  bool result = true;
  png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if(png != NULL){
    
    info = png_create_info_struct(png);

    if(info != NULL){
      if(!setjmp(png_jmpbuf(png))){

	png_set_IHDR(png,
		     info,
		     width,
		     height,
		     8,
		     PNG_COLOR_TYPE_RGB,
		     PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_DEFAULT,
		     PNG_FILTER_TYPE_DEFAULT
		     );

	rows = png_malloc(png, sizeof(png_byte *) * height);
	struct pixel * src = pixels;
	for(size_t y = 0; y < height; ++y){
	  png_byte * row = png_malloc(png, sizeof(png_byte) * width * 3);
	  rows[y] = row;
	  for(size_t x = 0; x < width; ++x){
	    *row = (png_byte)src->red;
	    ++row;
	    *row = (png_byte)src->green;
	    ++row;
	    *row = (png_byte)src->blue;
	    ++row;
	    ++src;
	  }
	}

	png_init_io(png, file);
	png_set_rows(png, info, rows);
	png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
	result = false;

	for(int y = 0; y < height; ++y){
	  png_free(png, rows[y]);
	}
	png_free(png, rows);
      }
    }
  }
  if(result){
    set_status(STATUS_PNG_ERROR);
  }
  png_destroy_write_struct(&png, &info);
  return result;
}
