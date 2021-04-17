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

#ifndef STATUS_H
#define STATUS_H

enum status_code{
		 STATUS_OK,
		 STATUS_MALLOC_FAILED,
		 STATUS_NO_SOLUTION,
		 STATUS_INF_SOLUTIONS
};

const char * get_status_msg(enum status_code sc);

void set_status(enum status_code sc);

enum status_code get_status();

#endif
