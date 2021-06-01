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

#include "hash_map.h"
#include "logger.h"
#include "memory.h"
#include "path.h"
#include "resource.h"
#include "serialization.h"
#include "status.h"
#include "unicode.h"

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <iconv.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_LANGUAGE_ID "en"

#define MISSING_LABEL_PLACEHOLDER U"<MISSING LABEL>"

static struct memory_buffer key_buf;
static struct memory_buffer label_buf;
static struct ptr_hash_map label_map;

static struct deserializer deserializer;

static int init_buffers(){
  init_memory_buffer(&key_buf, 0);
  init_memory_buffer(&label_buf, 0);

  if(init_ptr_hash_map(&label_map, hash_map_hash_str, hash_map_eq_str, 0)){
    LOG_ERROR("could not create label hash map");
    dispose_memory_buffer(&key_buf);
    dispose_memory_buffer(&label_buf);
    return -1;
  }
  return 0;
}

static char * add_key(const char * key){
  assert(key != NULL);
  char * result = copy_to_memory_buffer(&key_buf, key, strlen(key) + 1);
  if(result == NULL){
    LOG_ERROR("could not allocate resource key on buffer");
    return NULL;
  }
  return result;
}

static int add_label(const char * key, const char32_t * value){
  assert(key != NULL);
  assert(value != NULL);

  char * nkey = add_key(key);
  if(key == NULL){
    return -1;
  }

  char32_t * label = copy_to_memory_buffer(&label_buf, value, (unicode_strlen(value) + 1) * sizeof(char32_t));
  if(label == NULL){
    LOG_ERROR("could not allocate label on buffer");
    return -1;
  }

  if(insert_new_into_ptr_hash_map(&label_map, nkey, label)){
    if(get_status() == STATUS_DUPLICATE_KEY){
      LOG_WARNING("duplicate label key %s", nkey);
    }else{
      LOG_ERROR("could not add label to map");
      return -1;
    }
  }
  
  return 0;
}

static void dispose_buffers(){
  dispose_ptr_hash_map(&label_map);
  dispose_memory_buffer(&label_buf);
  dispose_memory_buffer(&key_buf);
}

static int load_resources(char * path, bool (*filter_fn)(const char *), int (load_fn)(const char *)){
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
    if(append_to_path(path, entry->d_name)){
      LOG_ERROR("could not create file path for name '%s'", entry->d_name);
      return -1;
    }
    if((*filter_fn)(path)){
      LOG_DEBUG("loading resources from file '%s'", path);
      if((*load_fn)(path)){
	LOG_ERROR("an error occurred while loading resources from file '%s'", path);
	closedir(dir);
	remove_from_path(path);
	return -1;
      }
    }
    remove_from_path(path);
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

static int load_label_file(const char * path){

  FILE * file = fopen(path, "r");

  int result = deserialize_from_file(&deserializer, NULL, file);

  fclose(file);
  
  return result;
}

static int find_resource_path(char * dest){
  assert(dest != NULL);
  if(getcwd(dest, PATH_MAX + 1) == NULL){
    LOG_ERROR("could not retrieve current working directory: path too long");
    set_status(STATUS_INVALID_RESOURCE_PATH);
    return -1;
  }
  if(append_to_path(dest, "resources")){
    return -1;
  }
  LOG_DEBUG("trying to find resources at path %s", dest);
  
  struct stat st;
  if(stat(dest, &st) == 0){
    if(S_ISDIR(st.st_mode)){
      return 0;
    }
  }
  
  LOG_DEBUG("resources not found at path %s", dest);
  set_status(STATUS_INVALID_RESOURCE_PATH);
  *dest = '\0';
  return -1;
}

static int handle_label(void * state, const char * key, const char32_t * value){
  if(add_label(key, value)){
    return -1;
  }
  return 0;
}

static int load_labels(char * path, const char * language){
  
  if(append_to_path(path, "labels")){
    LOG_ERROR("could not create labels resource path");
    return -1;
  }
  if(append_to_path(path, language)){
    LOG_ERROR("could not create localized resource path");
    return -1;
  }
  
  if(init_deserializer(&deserializer)){
    return -1;
  }

  deserializer_expect_map(&deserializer, NULL, NULL);
  deserializer_expect_unicode_string_entries(&deserializer, handle_label);

  if(finalize_deserializer(&deserializer)){
    dispose_deserializer(&deserializer);
    return -1;
  }
  
  if(load_resources(path, is_yaml_file, load_label_file)){
    LOG_ERROR("could not load labels");
    dispose_deserializer(&deserializer);
    return -1;
  }

  dispose_deserializer(&deserializer);
  
  remove_from_path(path);
  remove_from_path(path);

  return 0;
}

int init_resources(const char * resource_path, const char * language){

  char path[PATH_MAX + 1];
  
  if(resource_path == NULL || strlen(resource_path) == 0){
    LOG_INFO("no resource path specified: searching for resources");
    if(find_resource_path(path)){
      set_status(STATUS_INVALID_RESOURCE_PATH);
      return -1;
    }
  }else{
    if(strlen(resource_path) > PATH_MAX){
      LOG_ERROR("resource path too long");
      set_status(STATUS_INVALID_RESOURCE_PATH);
      return -1;
    }
    strcpy(path, resource_path);
  }

  if(language == NULL){
    LOG_INFO("no language specified, defaulting to '%s'", DEFAULT_LANGUAGE_ID);
    language = DEFAULT_LANGUAGE_ID;
  }

  LOG_INFO("loading resources from path '%s' and langauge '%s'...", path, language);
  
  struct stat st;

  if(stat(path, &st)){
    LOG_ERROR("could not determine file type of resource path '%s'", path);
    set_status(STATUS_INVALID_RESOURCE_PATH);
    return -1;
  }
  if(!S_ISDIR(st.st_mode)){
    LOG_ERROR("resource path '%s' is not a directory", path);
    set_status(STATUS_INVALID_RESOURCE_PATH);
    return -1;
  }

  if(init_buffers()){
    return -1;
  }
  
  if(load_labels(path, language)){
    dispose_buffers();
    return -1;
  }
  
  LOG_INFO("resources loaded");
  return 0;
}


const char32_t * get_resource_label(const char * key){
  assert(key != NULL);

  char32_t * label = get_from_ptr_hash_map(&label_map, key);
  if(label == NULL){
    return MISSING_LABEL_PLACEHOLDER;
  }else{
    return label;
  }
}

void dispose_resources(){
  LOG_INFO("dispose resources");
  dispose_buffers();
}
