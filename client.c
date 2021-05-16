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
#include "deque.h"
#include "ipc.h"
#include "logger.h"
#include "status.h"
#include "thread_utils.h"

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int client_socket;
static struct ipc_alloc alloc;
static struct ipc_duplex duplex;

static struct ipc_queue client_msg_queue;
static struct ipc_queue client_discard_queue;

int init_client(){
  
  LOG_INFO("initializing client...");
  if(init_ipc_alloc(&alloc)){
    return -1;
  }
  
  if(init_ipc_duplex(&duplex, &alloc)){
    dispose_ipc_alloc(&alloc);
    return -1;
  }

  init_ipc_queue(&client_msg_queue, &alloc);
  
  LOG_INFO("client initialized");

  return 0;
}

int start_client(){

  LOG_INFO("starting client...");
  
  LOG_INFO("attempting to connect to server at host %s and port %s", DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT);

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_NUMERICSERV;

  struct addrinfo * result;

  if(getaddrinfo(DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT, &hints, &result)){
    LOG_ERROR("could not find suitable service for host %s and port %s", DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT);
    set_status(STATUS_NO_SERVER_ADDRESS);
    dispose_ipc_duplex(&duplex);
    dispose_ipc_alloc(&alloc);
    return -1;
  }

  client_socket = -1;
  struct addrinfo * addr;
  for(addr = result; addr != NULL; addr = addr->ai_next){
    client_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if(client_socket != -1){
      if(connect(client_socket, addr->ai_addr, addr->ai_addrlen) == -1){
	close(client_socket);
	client_socket = -1;
      }else{
	break;
      }
    }
  }

  freeaddrinfo(result);

  if(client_socket == -1){
    LOG_ERROR("could not connect to service for host %s and port %s", DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT);
    set_status(STATUS_NO_SERVER_ADDRESS);
    dispose_ipc_duplex(&duplex);
    dispose_ipc_alloc(&alloc);
    return -1;
  }

  if(open_ipc_duplex(&duplex, client_socket)){
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    dispose_ipc_duplex(&duplex);
    dispose_ipc_alloc(&alloc);
  }

  LOG_INFO("client started");
  
  return 0;
}

int stop_client(){

  LOG_INFO("stopping client...");
  
  int result = close_ipc_duplex(&duplex);

  if(shutdown(client_socket, SHUT_RDWR)){
    LOG_ERROR("could not shut down client socket: %s", strerror(errno));
    result = -1;
  }

  if(close(client_socket)){
    LOG_ERROR("could not close client socket: %s",  strerror(errno));
    result = -1;
  }

  LOG_INFO("client stopped");

  return result;
}

int dispose_client(){

  LOG_INFO("disposing client...");
  int result = 0;
  
  if(dispose_ipc_duplex(&duplex)){
    result = -1;
  }
  
  if(dispose_ipc_alloc(&alloc)){
    result = -1;
  }

  LOG_INFO("client disposed");

  return result;
}

int receive_client_messages(){
  if(clear_ipc_queue(&client_discard_queue)){
    return -1;
  }
  
  return try_receive_all_from_ipc_duplex(&client_msg_queue, &duplex);
}

struct ipc_msg * get_received_client_msg(){
  return pop_from_ipc_queue(&client_msg_queue);
}

struct ipc_msg * create_client_msg(){
  return create_ipc_msg(&alloc);
}

void discard_client_msg(struct ipc_msg * msg){
  push_onto_ipc_queue(&client_discard_queue, msg);
}

int send_client_msg(struct ipc_msg * msg){
  assert(msg != NULL);

  return send_to_ipc_duplex(&duplex, msg);
}
