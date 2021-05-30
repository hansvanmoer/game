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
#include "unicode.h"

#include <assert.h>
#include <string.h>

#define SERVER_STATE_COUNT 1

struct server_player{
  int id;
  char32_t name[GAME_MAX_PLAYER_NAME_LEN + 1];
};

static enum server_state state;

static struct server_player players[GAME_MAX_PLAYER_COUNT];
static size_t player_count;

static int add_server_player(const char32_t * name){
  if(state ==  SERVER_STATE_WAITING_FOR_PLAYERS){
    set_status(STATUS_INVALID_SERVER_STATE);
    return -1;
  }
  
  if(player_count == GAME_MAX_PLAYER_COUNT){
    set_status(STATUS_MAX_PLAYER_COUNT_REACHED);
    return -1;
  }
  bool valid = true;
  for(int i = 0; i < player_count; ++i){
    if(unicode_streq(name, players[i].name) == 0){
      valid = false;
    }
  }
  if(valid){
    unicode_strcpy(players[player_count].name, name);
    int id = (int)player_count;
    players[player_count].id = id;
    ++player_count;
    return id;
  }else{
    set_status(STATUS_DUPLICATE_PLAYER_NAME);
    return -1;
  }
}

static int handle_auth_req(const struct protocol_auth_req * req){
  assert(req != NULL);
  LOG_DEBUG("server: handle authentication request");
  struct ipc_msg * msg = create_server_msg();
  if(msg == NULL){
    return -1;
  }
  
  const char * reason;
  int result = add_server_player(req->name);
  if(result > 0){
    reason = "";
  }else{
    switch(get_status()){
    case STATUS_INVALID_SERVER_STATE:
      LOG_DEBUG("server: authentication request rejected: server not waiting for players");
      reason = "server not waiting for players";
      break;
    case  STATUS_DUPLICATE_PLAYER_NAME:
      LOG_DEBUG("server: authentication rejected: duplicate player name");
      reason = "duplicate player name";
      break;
    case STATUS_MAX_PLAYER_COUNT_REACHED:
      LOG_DEBUG("server: authentication rejected: maximum number of players reached");
      reason = "maximum player count reached";
      break;
    default:
      LOG_ERROR("server: unexpected error while adding player");
      reason = "unexpected error";
      break;
    }
  }
  init_protocol_auth_res(&msg->payload, result, reason);
  return send_server_msg(msg);
}

int init_server_state(){
  memset(players, 0, sizeof(players));
  player_count = 0;
  state = SERVER_STATE_WAITING_FOR_PLAYERS;
  return 0;
}

int update_server_state(const struct ipc_msg * msg){
  LOG_DEBUG("server: message received: %s", get_protocol_msg_type_label(msg->payload.type));
  switch(msg->payload.type){
  case PROTOCOL_MSG_TYPE_AUTH_REQ:
    return handle_auth_req(&msg->payload.auth_req);
  default:
    LOG_ERROR("server: unexpected message: %s", get_protocol_msg_type_label(msg->payload.type));
    set_status(STATUS_PROTOCOL_ERROR);
    return -1;
  }
}

int dispose_server_state(){
  return 0;
}
