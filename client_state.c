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

#include "client_state.h"
#include "logger.h"
#include "unicode.h"

#include <string.h>

#define CLIENT_STATE_COUNT 9

typedef int (*client_transition_fn)();

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

static int at_client_started(){
  state = CLIENT_STATE_READY;
  return 0;
}


static int at_client_unauthorized(){
  return 0;
}


static int at_client_authorizing(){
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


