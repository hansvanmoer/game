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

#include <assert.h>
#include <string.h>
#include <strings.h>

bool path_has_extension(const char * path, const char * ext){
  assert(path != NULL);
  assert(ext != NULL);
  int ext_len = strlen(ext);
  int path_len = strlen(path);

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
