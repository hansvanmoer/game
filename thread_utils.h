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

/**
 * initializes a mutex and logs an error message on failure
 */
int init_named_mutex(pthread_mutex_t * mutex, const char * name);

/**
 * locks a mutex and logs an error message on failure
 */
int lock_named_mutex(pthread_mutex_t * mutex, const char * name);

/**
 * unlocks a mutex and logs and logs error message on failure
 */ 
int unlock_named_mutex(pthread_mutex_t * mutex, const char * name);

/**
 * disposes a mutex and logs an error message on failure
 */
int dispose_named_mutex(pthread_mutex_t * mutex, const char * name);

/**
 * disables thread cancellation
 */
int disable_thread_cancel();

/**
 * enables thread cancellation
 */
int enable_thread_cancel();

/**
 * to be called after creating a thread
 * sets log priority according to program settings
 */
void init_thread();

#endif
