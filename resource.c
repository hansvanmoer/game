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
#include "path.h"
#include "resource.h"
#include "status.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

static int load_resources(const char * path, bool (*filter_fn)(const char *), int (load_fn)(const char *)){
  assert(path != NULL);

  LOG_DEBUG("loading resources from folder '%s'", path);
  
  DIR * dir = opendir(path);
  if(dir == NULL){
    LOG_ERROR("could not open director '%s': %s", path, strerror(errno));
    return -1;
  }

  /*
   * Note that readdir is not guaranteed to be thread safe, but it should be on most systems
   * We use readdir instead of readdir_r because the latter has its own set of issues 
   * and is deprecated on some systems
   */
  struct dirent * entry;
  errno = 0;
  while((entry = readdir(dir)) != NULL){
    if(errno == EBADF){
      LOG_ERROR("could not walk directory %s: %s", path, strerror(errno));
      closedir(dir);
      return -1;
    }

    if((*filter_fn)(entry->d_name)){
      LOG_DEBUG("loading resources from file '%s'", entry->d_name);
      if((*load_fn)(entry->d_name)){
	LOG_ERROR("an error occurred while loading resources from file '%s'", entry->d_name);
	closedir(dir);
	return -1;
      }
    } 
  }
  if(closedir(dir)){
    LOG_ERROR("could not close directory '%s': %s", path, strerror(errno));
    return -1;
  }
  LOG_DEBUG("resources loaded from directory '%s'", path);
  return 0;
}

static bool is_yaml_file(const char * path){
  return path_has_extension(path, "yaml");
}

static int load_labels(const char * path){
  return 0;
}

int init_resources(const char * resource_path, const char * language){
  assert(resource_path != NULL);
  assert(language != NULL);

  LOG_INFO("loading resources from path '%s' and langauge '%s'...", resource_path, language);
  
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
  
  LOG_INFO("resources loaded");
  return 0;
}


const char32_t * get_resource_label(const char * key){
  return NULL;
}

void dispose_resources(){}
