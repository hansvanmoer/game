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

#ifndef PATH_H
#define PATH_H

#include <stdbool.h>

/**
 * A set of utility functions for file paths
 * Note that all destination buffers used in these functions must be at least MAX_PATH + 1 long
 */

/**
 * Concatenates both pats
 * Any path separators at the end and beginning of paths are stripped
 * The destination buffer must be at least PATH_MAX + 1 characters long
 * This function returns -1 if the resulting path is too long
 */
int concat_paths(char * dest, const char * first, const char * second);

/**
 * Returns true if the supplied path ends with the specified extension, false otherwise
 * The extension should not include the dot ('.') character and must not be empty
 */
bool path_has_extension(const char * path, const char * ext);

#endif
