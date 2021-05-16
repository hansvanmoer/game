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

#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

enum client_state{
		  CLIENT_STATE_STARTED,
		  CLIENT_STATE_UNAUTHORIZED,
		  CLIENT_STATE_AUTHORIZING,
		  CLIENT_STATE_REJECTED,
		  CLIENT_STATE_INITIALIZING,
		  CLIENT_STATE_READY,
		  CLIENT_STATE_STOPPING,
		  CLIENT_STATE_STOPPED,
		  CLIENT_STATE_ERROR
};

void init_client_state();

int update_client_state();

void dispose_client_state();

#endif
