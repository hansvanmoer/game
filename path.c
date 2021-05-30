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

#include "path.h"
#include "status.h"

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <strings.h>

int append_to_path(char * dest, const char * second){
  assert(dest != NULL);
  assert(second != NULL);
  size_t len = strlen(dest);
  if(len == 0){
    strcpy(dest, second);
    return 0;
  }else{
    bool sep = dest[len - 1] != '/';
    size_t index = 0;
    //strip path separators from second
    while(second[index] == '/'){
      ++index;
    }
    size_t slen = strlen(second);

    size_t total = len + (sep ? 1 : 0) + slen - index;
    if(total > PATH_MAX){
      set_status(STATUS_PATH_TOO_LONG);
      return -1;
    }
    if(sep){
      dest[len] = '/';
      ++len;
    }
    memcpy(dest + len, second + index, slen - index);
    dest[total] = '\0';
    return 0;
  }
}

int remove_from_path(char * path){
  assert(path != NULL);

  size_t len = strlen(path);
  if(len == 0){
    set_status(STATUS_INVALID_PATH);
    return -1;
  }else{
    size_t index = len - 1;
    if(path[index] == '/'){
      --index;
    }
    if(index == 0){
      // root of absolute path or empty relative one
      set_status(STATUS_INVALID_PATH);
      return -1;
    }
    while(index != 0 && path[index] != '/'){
      --index;
    }
    if(index == 0 && path[index] == '/'){
      //root of absolute path => ensure that leading '/' is returned
      ++index;
    }
    path[index] = '\0';
    return 0;
  }
}

bool path_has_extension(const char * path, const char * ext){
  assert(path != NULL);
  assert(ext != NULL);
  size_t ext_len = strlen(ext);
  size_t path_len = strlen(path);

  if(path_len < (ext_len + 1)){
    return false;
  }

  const char * p = path + path_len - ext_len - 1;
  if(*p != '.'){
    return false;
  }
  ++p;
  return strcasecmp(path, ext);
}
