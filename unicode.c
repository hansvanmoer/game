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

#include "unicode.h"

#include <assert.h>
#include <string.h>

size_t unicode_strlen(const char32_t * str){
  assert(str != NULL);
  
  size_t len = 0;
  while(*str != 0){
    ++str;
    ++len;
  }
  return len;
}

bool unicode_streq(const char32_t * first, const char32_t * second){
  assert(first != NULL);
  assert(second != NULL);

  while(true){
    char32_t f = *first;
    char32_t s = *second;
    if(f == 0){
      return s == 0;
    }else if(f != s){
      return false;
    }
    ++first;
    ++second;
  }
}

void unicode_strncpy(char32_t * dest, const char32_t * src, size_t len){
  assert(dest != NULL);
  assert(src != NULL);
  
  memcpy(dest, src, len * sizeof(char32_t));
  dest[len] = 0;
}

void unicode_strcpy(char32_t * dest, const char32_t * src){
  assert(dest != NULL);
  assert(src != NULL);

  size_t len = unicode_strlen(src);
  unicode_strncpy(dest, src, len);
}

size_t unicode_strcpy_checked(char32_t * dest, size_t len, const char32_t * src){
  assert(dest != NULL);
  assert(src != NULL);
  
  size_t src_len = unicode_strlen(src);
  
  if(len > src_len){
    len = src_len;
  }
  unicode_strncpy(dest, src, len);
  return len;
}

void str_to_unicode_str(char32_t * dest, const char * src){
  assert(dest != NULL);
  assert(src != NULL);
  
  while(*src != '\0'){
    *dest = (char32_t)*src;
    ++dest;
    ++src;
  }
  *dest = 0;
}


size_t str_to_unicode_str_checked(char32_t * dest, size_t len, const char * src){
  assert(dest != NULL);
  assert(src != NULL);

  size_t l = 0;
  while(*src != '\0' && l != len){
    *dest = (char32_t)*src;
    ++dest;
    ++src;
    ++l;
  }
  *dest = 0;
  return l;
}
