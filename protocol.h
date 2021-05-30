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

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "game.h"

#include <iconv.h>
#include <uchar.h>

#define DEFAULT_SERVER_HOST "::1"
#define DEFAULT_SERVER_PORT "50000"

#define PROTOCOL_STATE_OUT_BUF_LEN 1024
#define PROTOCOL_STATE_IN_BUF_LEN 1024

#define PROTOCOL_MAX_REASON_LEN 64

/**
 * Must be at least player max name len times 4 bytes
 */
#define PROTOCOL_STATE_NAME_BUF_LEN GAME_MAX_PLAYER_NAME_LEN * 4

enum protocol_msg_type{
		       PROTOCOL_MSG_TYPE_AUTH_REQ,
		       PROTOCOL_MSG_TYPE_AUTH_RES,
		       PROTOCOL_MSG_TYPE_CLOSE_REQ,
		       PROTOCOL_MSG_TYPE_CLOSE_RES
};

const char * get_protocol_msg_type_label(enum protocol_msg_type type);

struct protocol_auth_req{
  char32_t name[GAME_MAX_PLAYER_NAME_LEN + 1];
};

struct protocol_auth_res{
  int id;
  char reason[PROTOCOL_MAX_REASON_LEN + 1];
};

struct protocol_close_req{
  char reason[PROTOCOL_MAX_REASON_LEN + 1];
};

struct protocol_close_res{
  int id;
  char reason[PROTOCOL_MAX_REASON_LEN + 1];
};


struct protocol_msg{
  enum protocol_msg_type type;
  union{
    struct protocol_auth_req auth_req;
    struct protocol_auth_res auth_res;
    struct protocol_close_req close_req;
    struct protocol_close_res close_res;
  };
};

struct protocol_state{
  int fd;
  iconv_t enc;
  char out_buf[PROTOCOL_STATE_OUT_BUF_LEN];
  iconv_t dec;
  char in_buf[PROTOCOL_STATE_IN_BUF_LEN];
  char name_buf[PROTOCOL_STATE_NAME_BUF_LEN + 1];
};

int init_protocol_state(struct protocol_state * ps);

int read_protocol_msg( struct protocol_state * ps, struct protocol_msg * dest, int fd); 

int write_protocol_msg(struct protocol_state * ps, int fd, const struct protocol_msg * msg);

void dispose_protocol_state(struct protocol_state * ps);

void init_protocol_auth_req(struct protocol_msg *msg, const char32_t * name);

void init_protocol_auth_res(struct protocol_msg * msg, int id, const char * reason);

#endif
