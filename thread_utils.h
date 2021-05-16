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

#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#include <pthread.h>

int init_named_mutex(pthread_mutex_t * mutex, const char * name);

int lock_named_mutex(pthread_mutex_t * mutex, const char * name);

int unlock_named_mutex(pthread_mutex_t * mutex, const char * name);

int dispose_named_mutex(pthread_mutex_t * mutex, const char * name);

int disable_thread_cancel();

int enable_thread_cancel();

#endif
