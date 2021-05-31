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
		 STATUS_IO_ERROR,
		 STATUS_NO_SOLUTION,
		 STATUS_INF_SOLUTIONS,
		 STATUS_INVALID_IMAGE_SIZE,
		 STATUS_PNG_ERROR,
		 STATUS_INVALID_PROGRAM_ARGUMENT,
		 STATUS_CREATE_THREAD_FAILED,
		 STATUS_JOIN_THREAD_FAILED,
		 STATUS_CANCEL_THREAD_FAILED,
		 STATUS_SET_THREAD_ATTRIBUTE_FAILED,
		 STATUS_CREATE_MUTEX_FAILED,
		 STATUS_DESTROY_MUTEX_FAILED,
		 STATUS_LOCK_MUTEX_FAILED,
		 STATUS_UNLOCK_MUTEX_FAILED,
		 STATUS_CREATE_CV_FAILED,
		 STATUS_SIGNAL_CV_FAILED,
		 STATUS_DESTROY_CV_FAILED,
		 STATUS_WAIT_CV_FAILED,
		 STATUS_INVALID_SERVER_STATE,
		 STATUS_SOCKET_CREATION_FAILED,
		 STATUS_SET_SIGNAL_HANDLER_FAILED,
		 STATUS_SET_SIGNAL_MASK_FAILED,
		 STATUS_NO_SERVER_ADDRESS,
		 STATUS_SOCKET_ERROR,
		 STATUS_CREATE_ENCODER_ERROR,
		 STATUS_ENCODING_ERROR,
		 STATUS_PROTOCOL_ERROR,
		 STATUS_INVALID_IPC_STATE,
		 STATUS_IPC_CONNECTION_LIMIT_REACHED,
		 STATUS_INVALID_IPC_RECIPIENT,
		 STATUS_IPC_QUEUE_STOPPED,
		 STATUS_DUPLICATE_PLAYER_NAME,
		 STATUS_MAX_PLAYER_COUNT_REACHED,
		 STATUS_INVALID_RESOURCE_PATH,
		 STATUS_YAML_ERROR,
		 STATUS_SYNTAX_ERROR,
		 STATUS_END_OF_STREAM,
		 STATUS_PATH_TOO_LONG,
		 STATUS_INVALID_PATH
};

const char * get_status_msg(enum status_code sc);

void set_status(enum status_code sc);

enum status_code get_status();

#endif
