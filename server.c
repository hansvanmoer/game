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

#include "deque.h"
#include "ipc.h"
#include "logger.h"
#include "server.h"
#include "status.h"
#include "thread_utils.h"

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_BACKLOG 10

static int listen_socket;
static pthread_t listen_worker;

static struct ipc_alloc alloc;
static struct ipc_multiplex multiplex;


int init_server(){
  LOG_INFO("initializing server...");

  if(init_ipc_alloc(&alloc)){
    return -1;
  }
  if(init_ipc_multiplex(&multiplex, &alloc)){
    return -1;
  }
  
  LOG_INFO("server initialized");
  return 0;
}

static void * run_listener(void * arg){
  while(true){
    int con_socket = accept(listen_socket, NULL, NULL);
    if(con_socket == -1){
      if(errno == EINVAL){
	// socket has been closed
	break;
      }else{
	LOG_ERROR("error while accepting connection: %s", strerror(errno));
      }
    }else{
      if(open_ipc_channel(&multiplex, con_socket) == -1){
	LOG_WARNING("maximum number of clients reached: refusing connection");
	close(con_socket);
      }
    }
  }
  return NULL;
}

int start_server(){
  LOG_INFO("starting server...");
   
  // ignore the SIGPIPE signal, otherwise program will terminate if client unexpectedly disconnects
  /*  if(signal(SIGPIPE, SIG_IGN) == SIG_ERR){
    LOG_ERROR("unable to start server: SIGPIPE signal handler could not be registered");
    set_status(STATUS_SET_SIGNAL_HANDLER_FAILED);
    return -1;
    }*/

  struct addrinfo hints;
  struct addrinfo * results;
  
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
  int error = getaddrinfo(DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT, &hints, &results);
  if(error){
    LOG_ERROR("unable to start server: %s", gai_strerror(error));
    set_status(STATUS_NO_SERVER_ADDRESS);
    return -1;
  }

  int opt_val = 1; // "true" 
  struct addrinfo * info = results;
  while(info != NULL){
     listen_socket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
     if(listen_socket == -1){
       info = info->ai_next;
     }else{
       if(setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) == -1){
	 freeaddrinfo(results);
	 LOG_ERROR("unable to start server: socket options could not be set");
	 set_status(STATUS_SOCKET_ERROR);
	 return -1;
       }
       if(bind(listen_socket, info->ai_addr, info->ai_addrlen) != 0){
	 info = info->ai_next;
       }else{
	 break;
       }
     }
  }
  
  freeaddrinfo(results);
  
  if(info == NULL){
    LOG_ERROR("unable to start server: could not bind server to any valid address");
    set_status(STATUS_NO_SERVER_ADDRESS);
    close(listen_socket);
    return -1;
  }

  if(listen(listen_socket, MAX_BACKLOG)){
    LOG_ERROR("unable to start server: unable to start listening for new clients");
    set_status(STATUS_SOCKET_ERROR);
    shutdown(listen_socket, SHUT_RDWR);
    close(listen_socket);
    return -1;
  }

  if(open_ipc_multiplex(&multiplex)){
     shutdown(listen_socket, SHUT_RDWR);
     close(listen_socket);
     return -1;
  }
  
  if(pthread_create(&listen_worker, NULL, run_listener, NULL) != 0){
    LOG_ERROR("unable to start server: could not create listen thread");
    set_status(STATUS_CREATE_THREAD_FAILED);
    shutdown(listen_socket, SHUT_RDWR);
    close(listen_socket);
    close_ipc_multiplex(&multiplex);
    return -1;
  }

  LOG_INFO("server started");
  
  return 0;
}

int stop_server(){  
  LOG_INFO("stopping server...");

  int result = 0;
  
  if(shutdown(listen_socket, SHUT_RDWR) != 0){
    LOG_ERROR("unable to stop server: could not shut down listening socket");
    set_status(STATUS_SOCKET_ERROR);
    result = -1;
  }

  if(close(listen_socket) != 0){
    LOG_ERROR("unable to stop server: could not close listening socket");
    set_status(STATUS_SOCKET_ERROR);
    result = -1;
  }

  if(pthread_cancel(listen_worker)){
    LOG_ERROR("unable to cancel listen thread");
    set_status(STATUS_CANCEL_THREAD_FAILED);
    result = -1;
  }
  
  if(pthread_join(listen_worker, NULL) != 0){
    LOG_ERROR("unable to stop server: could not join with listen thread");
    set_status(STATUS_JOIN_THREAD_FAILED);
    result = -1;
  }

  if(close_ipc_multiplex(&multiplex)){
    result = -1;
  }
 
  LOG_INFO("server stopped");
  return result;
}

int dispose_server(){
  LOG_INFO("disposing server...");

  int result = 0;
  
  if(dispose_ipc_multiplex(&multiplex)){
    result = -1;
  }
  
  if(dispose_ipc_alloc(&alloc)){
    result = -1;
  }
  
  LOG_INFO("server disposed");
  return result;
}

int receive_server_msg(struct ipc_msg ** msg){
  assert(msg != NULL);
  
  return receive_from_ipc_multiplex(msg, &multiplex);
}

int send_server_msg(int to, struct ipc_msg * msg){
  assert(msg != NULL);
  msg->recipient = to;
  return send_to_ipc_multiplex(&multiplex, msg);
}

struct ipc_msg * create_server_msg(){
  return create_ipc_msg(&alloc);
}

int discard_server_msg(struct ipc_msg * msg){
  assert(msg != NULL);
  return destroy_ipc_msg(msg);
}
