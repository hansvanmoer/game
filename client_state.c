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

#include "client.h"
#include "client_state.h"
#include "game.h"
#include "ipc.h"
#include "logger.h"
#include "unicode.h"

#include <string.h>

#define CLIENT_STATE_COUNT 9

typedef int (*client_transition_fn)();

enum client_player_state{
  CLIENT_PLAYER_STATE_UNAUTHORIZED,
  CLIENT_PLAYER_STATE_AUTHORIZING,
  CLIENT_PLAYER_STATE_REJECTED,
  CLIENT_PLAYER_STATE_AUTHORIZED
};

struct client_player{
  char32_t name[GAME_MAX_PLAYER_NAME_LEN + 1];
  int id;
  enum client_player_state state;
};

static client_transition_fn transitions[CLIENT_STATE_COUNT];

static const char * state_labels[] =  {
				       "STARTED",
				       "UNAUTHORIZED",
				       "AUTHORIZING",
				       "REJECTED",
				       "INITIALIZING",
				       "READY",
				       "STOPPING",
				       "STOPPED",
				       "ERROR"
};


static enum client_state state;

static struct client_player players[GAME_MAX_PLAYER_COUNT];
static int player_count;

static int add_player(char * name){
  if(player_count == GAME_MAX_PLAYER_COUNT){
    return -1;
  }

  struct client_player * p = &players[player_count];
  str_to_unicode_str_checked(p->name, GAME_MAX_PLAYER_NAME_LEN, name);
  p->id = -1;
  p->state = CLIENT_PLAYER_STATE_UNAUTHORIZED;
  ++player_count;
  return 0;
}

static int at_client_started(){
  //TEST CODE: add 1 player
  add_player("player1");
  state = CLIENT_STATE_UNAUTHORIZED;
  return 0;
}


static int at_client_unauthorized(){

  for(size_t i = 0; i < player_count; ++i){

    if(players[i].state == CLIENT_PLAYER_STATE_UNAUTHORIZED){
      
      struct ipc_msg * msg = create_client_msg();
      if(msg == NULL){
	return -1;
      }
      
      init_protocol_auth_req(&msg->payload, players[i].name);
      send_client_msg(msg);
      players[i].state = CLIENT_PLAYER_STATE_AUTHORIZING;
    }
  }
  
  state = CLIENT_STATE_AUTHORIZING;
  
  return 0;
}


static int at_client_authorizing(){

  struct ipc_msg * msg;
  while((msg = get_received_client_msg()) != NULL){
    if(msg->payload.type == PROTOCOL_MSG_TYPE_AUTH_RES){
      struct protocol_auth_res * body = &msg->payload.auth_res;
      if(body->id == -1){
	LOG_DEBUG("client: authentication rejected by server: %s", body->reason);
	players[0].state = CLIENT_PLAYER_STATE_REJECTED;
	state = CLIENT_STATE_REJECTED;
      }else{
	LOG_DEBUG("client: authentication accepted for player %d", body->id);
	players[0].id = body->id;
	players[0].state = CLIENT_PLAYER_STATE_AUTHORIZED;
	state = CLIENT_STATE_INITIALIZING;
      }
    }else{
      LOG_ERROR("client: unexpected message received: %s", get_protocol_msg_type_label(msg->payload.type));
    }
    destroy_client_msg(msg);
  }
  
  return 0;
}

static int at_client_rejected(){
  return 0;
}


static int at_client_initializing(){
  return 0;
}

static int at_client_ready(){
  return 0;
}

static int at_client_stopping(){
  return 0;
}

static int at_client_stopped(){
  return 0;
}

static int at_client_error(){
  return 0;
}

void init_client_state(){
  memset(transitions, 0, sizeof(transitions));
  transitions[CLIENT_STATE_STARTED] = at_client_started;
  transitions[CLIENT_STATE_UNAUTHORIZED] = at_client_unauthorized;
  transitions[CLIENT_STATE_AUTHORIZING] = at_client_authorizing;
  transitions[CLIENT_STATE_REJECTED] = at_client_rejected;
  transitions[CLIENT_STATE_INITIALIZING] = at_client_initializing;
  transitions[CLIENT_STATE_READY] = at_client_ready;
  transitions[CLIENT_STATE_STOPPING] = at_client_stopping;
  transitions[CLIENT_STATE_STOPPED] = at_client_stopped;
  transitions[CLIENT_STATE_ERROR] = at_client_error;
  state = CLIENT_STATE_STARTED;

  memset(&players, 0, sizeof(players));
  player_count = 0;
}

int update_client_state(){
  enum client_state last_state = state;
  int result = (*transitions[(int)state])();
  if(result){
    LOG_ERROR("client state transition from %s has encountered an error", state_labels[(int)last_state]);
  }else if(state != last_state){
    LOG_DEBUG("client state transitioned from %s to %s", state_labels[(int)last_state], state_labels[(int)state]);
  }
  return result;
}

void dispose_client_state(){}


