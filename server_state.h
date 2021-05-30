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

#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include "ipc.h"

enum server_state{
  SERVER_STATE_WAITING_FOR_PLAYERS
};

int init_server_state();

int update_server_state(const struct ipc_msg * msg);

int dispose_server_state();

#endif
