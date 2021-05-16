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

#include "logger.h"
#include "status.h"
#include "thread_utils.h"

#include <string.h>

int init_named_mutex(pthread_mutex_t * mutex, const char * name){
  int error;
  if((error = pthread_mutex_init(mutex, NULL))){
    LOG_ERROR("could not create %s mutex: %s", name, strerror(error));
    set_status(STATUS_CREATE_MUTEX_FAILED);
    return -1;
  }
  return 0;
}

int lock_named_mutex(pthread_mutex_t * mutex, const char * name){
  int error;
  if((error = pthread_mutex_lock(mutex))){
    LOG_ERROR("could not unlock %s mutex: %s", name, strerror(error));
    set_status(STATUS_UNLOCK_MUTEX_FAILED);
    return -1;
  }
  return 0;
}

int unlock_named_mutex(pthread_mutex_t * mutex, const char * name){
  int error;
  if((error = pthread_mutex_unlock(mutex))){
    LOG_ERROR("could not unlock %s mutex: %s", name, strerror(error));
    set_status(STATUS_UNLOCK_MUTEX_FAILED);
    return -1;
  }
  return 0;
}

int dispose_named_mutex(pthread_mutex_t * mutex, const char * name){
  int error;
  if((error = pthread_mutex_destroy(mutex))){
    LOG_ERROR("could not destroy %s mutex: %s", name, strerror(error));
    set_status(STATUS_DESTROY_MUTEX_FAILED);
    return -1;
  }
  return 0;
}

int disable_thread_cancel(){
  if(pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL)){
    LOG_ERROR("could not disable thread cancellation");
    set_status(STATUS_SET_THREAD_ATTRIBUTE_FAILED);
    return -1;
  }
  return 0;
}

int enable_thread_cancel(){
  if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)){
    LOG_ERROR("could not disable thread cancellation");
    set_status(STATUS_SET_THREAD_ATTRIBUTE_FAILED);
    return -1;
  }
  return 0;
}

