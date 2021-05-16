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

#ifndef SERVER_H
#define SERVER_H

#include "ipc.h"

int init_server();

int start_server();

int receive_server_msg(struct ipc_msg ** msg);

int send_server_msg(struct ipc_msg * msg);

struct ipc_msg * create_server_msg();

int discard_server_msg(struct ipc_msg * msg);

int stop_server();

int dispose_server();

#endif
