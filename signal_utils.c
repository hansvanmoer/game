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

/*
 * NOTE: logging subsystem will not have started yet as it starts
 * threads that will use the signal mask set in init_signals()
 * Do not use logging in these functions
 * Failure will likely bring the program down anyway
 */

#include "program.h"
#include "status.h"

#include <pthread.h>
#include <signal.h>

static pthread_t signal_worker;

static void init_signal_mask(sigset_t * mask){
  sigemptyset(mask);
  sigaddset(mask, SIGINT);
  sigaddset(mask, SIGPIPE);
}

int init_signals(){
  
  /* 
   * block certain signals for the entire process
   * all subsequent threads will inherit this mask and block the signals
   */
  sigset_t mask;
  init_signal_mask(&mask);
  
  if(pthread_sigmask(SIG_BLOCK, &mask, NULL)){
    set_status(STATUS_SET_SIGNAL_MASK_FAILED);
    return -1;
  }

  return 0;
}

static void * run_signal_worker(){
  /*
   * unblock signals for this specific thread
   * Note that error handling is rather pointless here
   */
  sigset_t mask;
  init_signal_mask(&mask);
  
  int signal;

  while(true){
    if(sigwait(&mask, &signal)){ // cancel point
      return NULL;
    }
    // do not cancel while attempting to handle signal
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    if(signal == SIGINT){
      request_program_stop();
    }else if(signal == SIGPIPE){
      // happens when server or client writes to socket that is disconnected from the other end
      // ignore
    }else{
      fprintf(stderr, "unexpected signal: %d\n", signal);
      break;
    }
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  }
  return NULL;
}

int start_signal_handler(){
  /*
   *  create a dedicated thread to handle signals
   */
  
  if(pthread_create(&signal_worker, NULL, run_signal_worker, NULL)){
    set_status(STATUS_CREATE_THREAD_FAILED);
    return -1;
  }

  return 0;
}

int stop_signal_handler(){
  if(pthread_cancel(signal_worker)){
    set_status(STATUS_CANCEL_THREAD_FAILED);
    return -1;
  }
  if(pthread_join(signal_worker, NULL)){
    set_status(STATUS_JOIN_THREAD_FAILED);
    return -1;
  }
  return 0;
}
