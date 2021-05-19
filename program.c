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
#include "ipc.h"
#include "logger.h"
#include "program.h"
#include "server.h"
#include "server_state.h"
#include "signal_utils.h"
#include "status.h"
#include "thread_utils.h"

#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

static struct program_settings settings;

static pthread_mutex_t program_mutex;
static pthread_cond_t program_cond;
static bool running = false;

static pthread_t server_worker;

static int server_result;

static pthread_t client_worker;
static int client_result;

static int init_program_mutex(){
  return init_named_mutex(&program_mutex, "program");
}

static int dispose_program_mutex(){
  return dispose_named_mutex(&program_mutex, "program");
}

static int lock_program_mutex(){
  return lock_named_mutex(&program_mutex, "program");
}

static int unlock_program_mutex(){
  return unlock_named_mutex(&program_mutex, "program");
}

static int init_program_locks(){
  if(init_program_mutex()){
    return -1;
  }

  if(pthread_cond_init(&program_cond, NULL)){
    LOG_ERROR("could not create program condition variable");
    set_status(STATUS_CREATE_CV_FAILED);
    dispose_program_mutex();
    return -1;
  }

  return 0;
}

static int dispose_program_locks(){
  int result = 0;
  if(pthread_cond_destroy(&program_cond)){
    LOG_ERROR("could not destroy program condition variable");
    set_status(STATUS_DESTROY_CV_FAILED);
    result = -1;
  }

  if(dispose_program_mutex()){
    result = -1;
  }

  return result;
}
    
static bool is_running(){
  bool result;

  if(lock_program_mutex()){
    return false;
  }
  
  result = running;
  
  if(unlock_program_mutex()){
    return false;
  }

  return result;
}

static void * run_server_loop(void * arg){

  init_thread();

  init_server_state();
  
  server_result = 0;
  
  if(server_result == 0){
    while(true){
      struct ipc_msg * msg;

      if(receive_server_msg(&msg)){
	LOG_ERROR("server loop will exit due to an error");
	server_result = -1;
	break;
      }
      
      if(msg == NULL){
	LOG_INFO("server loop will exit because there are no more messages");
	break;
      }

      if(update_server_state(msg)){
	server_result = -1;
      }
      
      if(discard_server_msg(msg)){
	LOG_ERROR("server loop will exit due to an error");
	server_result = -1;
	break;
      }
    }
  }

  dispose_server_state();
  
  return (void *)&server_result;
}

static void * run_client_loop(void * arg){

  init_thread();
  
  LOG_DEBUG("client loop started");
  
  init_client_state();
  
  while(true){
    if(!is_running()){
      break;
    }

    if(receive_client_messages()){
      LOG_ERROR("client loop will exit due to an error");
      client_result = -1;
      break;
    }

    if(update_client_state()){
      client_result = -1;
      break;
    }
    
    sleep(1);
  }

  dispose_client_state();

  LOG_DEBUG("client loop stopped");
  
  return (void *)&client_result;
}

static int wait_for_program_stop(){

  int result = 0;
  void * result_ptr;
  
  if(settings.client){
    if(pthread_join(client_worker, &result_ptr)){
      LOG_ERROR("could not client server thread");
      set_status(STATUS_JOIN_THREAD_FAILED);
      result = -1;
    }
    if(*(int *)result_ptr){
      LOG_WARNING("client loop has exited with errors");
      result = -1;
    }
  }
  
  if(settings.server){
    if(pthread_join(server_worker, &result_ptr)){
      LOG_ERROR("could not join server thread");
      set_status(STATUS_JOIN_THREAD_FAILED);
      result = -1;
    }
    if(*(int *)result_ptr){
      LOG_WARNING("server loop has exited with errors");
      result = -1;
    }
  }
  
  return result;
}

static int start_server_loop(){
  if(init_server()){
    return -1;
  }
  
  if(start_server()){
    dispose_server();
    return -1;
  }
  
  if(pthread_create(&server_worker, NULL, run_server_loop, NULL)){
    LOG_ERROR("could not start server thread");
    set_status(STATUS_CREATE_THREAD_FAILED);
    stop_server();
    dispose_server();
    return -1;
  }

  return 0;
}

static int start_client_loop(){
  if(init_client()){
    return -1;
  }
  
  if(start_client()){
    dispose_client();
    return -1;
  }
  
  if(pthread_create(&client_worker, NULL, run_client_loop, NULL)){
    LOG_ERROR("could not start client thread");
    set_status(STATUS_CREATE_THREAD_FAILED);
    stop_client();
    dispose_client();
    return -1;
  }
  
  return 0;
}


int run_program_loop(const struct program_settings * s){
  settings = *s;

  LOG_DEBUG("starting program loop...");
  
  if(init_program_locks()){
    return -1;
  }

  if(start_signal_handler()){
    dispose_program_locks();
    return -1;
  }

  if(lock_program_mutex()){
    stop_signal_handler();
    dispose_program_locks();
    return -1;
  }

  running = true;
  
  int result = 0;
  
  if(settings.server){
    if(start_server_loop()){
      result = -1;
    }
  }
  
  if(settings.client){
    if(start_client_loop()){
      result = -1;
    }
  }
  
  if(result == 0){
    LOG_DEBUG("program loop started");
     
    while(true){
      if(pthread_cond_wait(&program_cond, &program_mutex)){
	LOG_ERROR("could not wait for program condition variable");
	set_status(STATUS_WAIT_CV_FAILED);
	result = -1;
	break;
      }
      if(!running){
	break;
      }
    }
     
    LOG_DEBUG("stopping program loop...");
  }

   
  if(unlock_program_mutex()){
    dispose_program_locks();
    result = -1;
  }

  if(settings.server){
    if(stop_server()){
      result = -1;
    }
  }
  if(settings.client){
    if(stop_client()){
      result = -1;
    }
  }
  
  if(wait_for_program_stop()){
    result = -1;
  }

  if(settings.server){
    if(dispose_server()){
      result = -1;
    }
  }
  if(settings.client){
    if(dispose_client()){
      result = -1;
    }
  }

  if(stop_signal_handler()){
    return -1;
  }
  
  if(dispose_program_locks()){
    return -1;
  }

  LOG_DEBUG("program loop stopped");
  
  return 0;
}

const struct program_settings * get_program_settings(){
  return &settings;
}

int request_program_stop(){
  LOG_INFO("requesting program to stop");

  if(lock_program_mutex()){
    return -1;
  }
  
  running = false;
  
  if(unlock_program_mutex()){
    return -1;
  }

  if(settings.server){
    
  }
  
  if(pthread_cond_signal(&program_cond)){
    LOG_ERROR("could not signal program condition variable to request program stop");
    return -1;
  }
  
  return 0;
}
