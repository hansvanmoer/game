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

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

enum log_priority{
		  LOG_PRIORITY_DEBUG,
		  LOG_PRIORITY_INFO,
		  LOG_PRIORITY_WARNING,
		  LOG_PRIORITY_ERROR
};

int start_logger(FILE * file);

int log_msg(enum log_priority priority, const char * format, ...);

void set_min_log_priority(enum log_priority priority);

enum log_priority get_min_log_priority();

int stop_logger();

#define LOG_MSG(priority, ...) if(priority >= get_min_log_priority()) log_msg(priority, __VA_ARGS__)

#define LOG_DEBUG(...) LOG_MSG(LOG_PRIORITY_DEBUG, __VA_ARGS__)

#define LOG_INFO(...) LOG_MSG(LOG_PRIORITY_INFO, __VA_ARGS__)

#define LOG_WARNING(...) LOG_MSG(LOG_PRIORITY_WARNING, __VA_ARGS__)

#define LOG_ERROR(...) LOG_MSG(LOG_PRIORITY_ERROR, __VA_ARGS__)

#endif
