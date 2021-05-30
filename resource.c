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
#include "resource.h"
#include "status.h"

#include <assert.h>
#include <sys/stat.h>

int init_resources(const char * resource_path, const char * language){
  assert(resource_path != NULL);
  assert(language != NULL);

  struct stat st;

  if(stat(resource_path, &st)){
    LOG_ERROR("could not determine file type of resource path '%s'", resource_path);
    set_status(STATUS_INVALID_RESOURCE_PATH);
    return -1;
  }
  if(!S_ISDIR(st.st_mode)){
    LOG_ERROR("resource path '%s' is not a directory", resource_path);
    set_status(STATUS_INVALID_RESOURCE_PATH);
    return -1;
  }

  return 0;
}

const char32_t * get_resource_label(const char * key){
  return NULL;
}

void dispose_resources(){}
