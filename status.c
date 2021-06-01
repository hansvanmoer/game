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

#include "status.h"

static const char * status_msg[] = {
				    "ok",
				    "memory allocation failed",
				    "input/output error",
				    "no solution for linear system",
				    "infinite solutions for linear system",
				    "invalid image size",
				    "PNG error",
				    "invalid program argument",
				    "thread creation failed",
				    "could not join thread",
				    "could not cancel thread",
				    "could not set thread attribute",
				    "could not create mutex",
				    "could not destroy mutex",
				    "mutex lock failed",
				    "mutex unlock failed",
				    "could not create condition variable",
				    "could not signal condition variable",
				    "error waiting on condition variable",
				    "could not destroy condition variable",
				    "invalid server state",
				    "socket creation failed",
				    "signal handler registration failed",
				    "could not set signal mask",
				    "no valid sever address could be found",
				    "socket error",
				    "could not create encoder",
				    "encoding error",
				    "protocol error",
				    "invalid ipc state",
				    "ipc connection limit reached",
				    "invalid ipc recipient",
				    "ipc queue stopped",
				    "duplicate player name",
				    "maximum player count reached",
				    "invalid resource path",
				    "error parsing yaml file",
				    "syntax error",
				    "end of stream",
				    "file path too long",
				    "invalid file path",
				    "value not found",
				    "duplicate key"
};

_Thread_local enum status_code cur_status = STATUS_OK;

const char * get_status_msg(enum status_code sc){
  return status_msg[sc];
}

void set_status(enum status_code sc){
  cur_status = sc;
}

enum status_code get_status(){
  return cur_status;
}
