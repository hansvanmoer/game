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

#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>

void * malloc_checked(size_t size);

struct memory_buffer_block;

struct memory_buffer{
  size_t cap;
  struct memory_buffer_block * head;
  struct memory_buffer_block * tail;
};

void init_memory_buffer(struct memory_buffer * buf, size_t cap);

void * copy_to_memory_buffer(struct memory_buffer * dest, const void * src, size_t len);

void dispose_memory_buffer(struct memory_buffer * buf);


#endif
