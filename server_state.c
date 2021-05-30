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

#include "logger.h"
#include "protocol.h"
#include "server.h"
#include "server_state.h"
#include "status.h"

#include <string.h>

#define SERVER_STATE_COUNT 1

struct server_player{
  int id;
  char32_t name[PROTOCOL_MAX_NAME_LEN + 1];
};

static const char state_labels[] = {
  "waiting for players"
};

static enum server_state state;

static struct server_player players[PROTOCOL_MAX_PLAYER_COUNT];
static size_t player_count;

static int handle_auth_req(const struct protocol_auth_req * req){
  return 0;
}

int init_server_state(){
  memset(players, 0, sizeof(players));
  player_count = 0;
  state = SERVER_STATE_WAITING_FOR_PLAYERS;
  return 0;
}

int update_server_state(const struct ipc_msg * msg){
  LOG_DEBUG("server message received: %s", get_protocol_msg_type_label(msg->payload.type));
  switch(msg->payload.type){
  case PROTOCOL_MSG_TYPE_AUTH_REQ:
    return handle_auth_req(&msg->payload.auth_req);
  default:
    LOG_ERROR("unexpected server message: %s", get_protocol_msg_type_label(msg->payload.type));
    set_status(STATUS_PROTOCOL_ERROR);
    return -1;
  }
}

int dispose_server_state(){
  return 0;
}
