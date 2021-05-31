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

#define INITIAL_KEY_CAP 4096

#define INITIAL_LABEL_CAP 4096

static char * key_buf;
static size_t key_len;
static size_t key_cap;

static char32_t * label_buf;
static size_t label_len;
static size_t label_cap;

static struct deserializer deserializer;  

static int init_buffers(){
  key_buf = malloc_checked(INITIAL_KEY_CAP);
  if(key_buf == NULL){
    LOG_ERROR("could not allocate resource key buffer");
    return -1;
  }
  key_len = 0;
  key_cap = INITIAL_KEY_CAP;
  label_buf = malloc_checked(INITIAL_LABEL_CAP * sizeof(char32_t));
  if(label_buf == NULL){
    LOG_ERROR("could not allocate resource value buffer");
    free(key_buf);
    return -1;
  }
  label_len = 0;
  label_cap = INITIAL_LABEL_CAP;
  return 0;
}

static int add_key(const char * key){
  assert(key != NULL);
  size_t len = strlen(key);
  size_t nlen = key_len + len;
  if((nlen + 1) > key_cap){
    size_t ncap = key_cap * 2;
    char * nbuf = malloc_checked(ncap);
    if(nbuf == NULL){
      LOG_ERROR("could not grow resource key buffer");
      return -1;
    }
    memcpy(nbuf, key_buf, key_len);
    free(key_buf);
    key_cap = ncap;
  }
  memcpy(key_buf + key_len, key, len);
  key_len += (len + 1);
  key_buf[key_len + len] = '\0';
  return 0;
}

static int shrink_buffers(){
  if(key_cap > key_len){
    char * nbuf = malloc_checked(key_len);
    if(nbuf == NULL){
      return -1;
    }
    memcpy(nbuf, key_buf, key_len);
    free(key_buf);
    key_buf = nbuf;
    key_cap = key_len;
  }
  if(label_cap > label_len){
    char32_t * nbuf = malloc_checked(label_len * sizeof(char32_t));
    if(nbuf == NULL){
      return -1;
    }
    memcpy(nbuf, label_buf, label_len * sizeof(char32_t));
    free(label_buf);
    label_buf = nbuf;
    label_cap = label_len;
  }
  return 0;
}

static void dispose_buffers(){
  free(label_buf);
  free(key_buf);
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

static int add_label(void * state, const char * key, const char32_t * value){
  puts(key);
  while(*value != 0){
    printf("%c ", (char)(*value));
    ++value;
  }
  puts("");
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
  deserializer_expect_unicode_string_entries(&deserializer, add_label);

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

  if(shrink_buffers()){
    return -1;
  }
  
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
  return NULL;
}

void dispose_resources(){
  LOG_INFO("dispose resources");
  dispose_buffers();
}
