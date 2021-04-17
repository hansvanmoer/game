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

#ifndef DEQUE_H
#define DEQUE_H

#include <stdbool.h>
#include <stdlib.h>

struct deque_block{
  struct deque_block * prev;
  struct deque_block * next;
  void * data;
};

struct deque{
  struct deque_block * head;
  struct deque_block * tail;
  size_t elem_size;
  size_t len;
  size_t block_len;
  size_t block_cap;
};

struct deque_iter{
  struct deque * deque;
  struct deque_block * block;
  size_t index;
};

void init_deque(struct deque * deque, size_t elem_size, size_t block_cap);

void * emplace_onto_deque(struct deque * deque);

void init_deque_iter(struct deque_iter * iter, struct deque * deque);

bool has_next_deque_iter(struct deque_iter * i);

void * get_deque_iter(struct deque_iter * i);

void next_deque_iter(struct deque_iter * i);

void dispose_deque(struct deque * deque);

#endif
