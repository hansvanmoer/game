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

#ifndef UNICODE_H
#define UNICODE_H

#include <stdbool.h>
#include <stddef.h>
#include <uchar.h>

#define UTF_8_ENCODING_NAME "UTF-8"

const char * get_unicode_encoding_name();

size_t unicode_strlen(const char32_t * str);

bool unicode_streq(const char32_t * first, const char32_t * second);

void unicode_strncpy(char32_t * dest, const char32_t * src, size_t len);

void unicode_strcpy(char32_t * dest, const char32_t * src);

size_t unicode_strcpy_checked(char32_t * dest, size_t len, const char32_t * src);

/**
 * TODO: remove this, only for testing purposes
 */

void str_to_unicode_str(char32_t * dest, const char * src);

size_t str_to_unicode_str_checked(char32_t * dest, size_t len, const char * src);

#endif
