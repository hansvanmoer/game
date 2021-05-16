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

/**
 * This compilation unit should have no dependencies whatsoever so that
 * every other unit can utilize logging
 */

#include "logger.h"

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * Arbitrary max length of a log message
 */
#define MAX_LOG_MSG_LEN 255

/**
 * Number of messages that are allocated at once on the heap
 */
#define LOG_MSG_BLOCK_SIZE 32

/**
 * Used to store log entries until they can be written to the output file
 */
struct log_msg{

  /**
   * The priority of the message
   * Note that log levels are set per thread, 
   * so this is only used for display purposes
   */
  enum log_priority priority;

  /**
   * The text of the message
   */
  char text[MAX_LOG_MSG_LEN];

  /**
   * Pointer to next message in a single linked queue
   */
  struct log_msg * next;
};

/**
 * A block of messages to be allocated at once
 */
struct log_msg_block{
  /**
   * The array of messages
   */
  struct log_msg messages[LOG_MSG_BLOCK_SIZE];

  /**
   * A link to the next block
   */
  struct log_msg_block * next;
};

/**
 * Labels for the log priorities as they appear in the output
 */
static const char * log_priority_labels[] = {"DEBUG  ", "INFO   ", "WARNING", "ERROR  "};

/**
 * A single linked list of allocated message blocks
 */
static struct log_msg_block * block = NULL;

/**
 * The index of the next unused message in the current bloc;
 */
static size_t next_msg_index = LOG_MSG_BLOCK_SIZE;

/**
 * A single linked list of messages waiting to be printed
 */
static struct log_msg * in_head = NULL;
static struct log_msg * in_tail = NULL;

/**
 * A mutex and condition variable to synchronize access to the in queue
 */
static pthread_cond_t in_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t in_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * A single linked list of messages waiting to be re-used
 */
static struct log_msg * out_head = NULL;

/**
 * A mutex to synchronize access to the out queue
 */
static pthread_mutex_t out_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * The worker thread that will print the messages
 */
static pthread_t worker;

/**
 * tracks whether the logger is still running
 */
static bool running = false;

/**
 * The output file for this logger
 * Note that the logger does not close the file,
 * this should be handled by the caller
 */
static FILE * file;

/**
 * Thread local variable specifying the minimul log priority of the thread
 */
__thread enum log_priority min_priority = LOG_PRIORITY_ERROR;

/**
 * Returns a pointer to a message that can be used for logging
 * This function will try to reuse recycled messages and allocate new messages
 * only when needed.
 */
static struct log_msg * get_log_msg(){
  if(pthread_mutex_lock(&out_mutex)){
    // no way to recover from this
    return NULL;
  }

  struct log_msg * msg;
  
  if(out_head == NULL){
    if(next_msg_index == LOG_MSG_BLOCK_SIZE){
      struct log_msg_block * new_block = malloc(sizeof(struct log_msg_block));
      memset(new_block->messages, 0, sizeof(new_block->messages));
      if(new_block == NULL){
	// no way to recover from this
	pthread_mutex_unlock(&out_mutex);
	return NULL;
      }
      new_block->next = block;
      block = new_block;
      next_msg_index = 0;
    }
    msg = block->messages + next_msg_index;
    ++next_msg_index;
  }else{
    msg = out_head;
    out_head = out_head->next;
  }
  if(pthread_mutex_unlock(&out_mutex)){
    return NULL;
  }
  msg->next = NULL;
  return msg;
}

/**
 * Prints the messages in this queue to the output
 */
static void print_messages(struct log_msg * head){
  assert(head != NULL);
  while(head != NULL){
    fprintf(file, "%s: %s\n", log_priority_labels[(int)head->priority], head->text);
    head = head->next;
  }
  
  // flush the file to ensure log messages appear in a timely fashion
  fflush(file);
}

/**
 * Recycles the messages in this queue so that they can be reused later on
 */
static int recycle_messages(struct log_msg * head, struct log_msg * tail){
  assert(head != NULL);
  assert(tail != NULL);
  assert(tail->next == NULL);

  if(pthread_mutex_lock(&out_mutex)){
    // no way to recover from this -> message memory is lost
    return -1;
  }
  if(out_head != NULL){
    tail->next = out_head;
  }
  out_head = head;
  if(pthread_mutex_unlock(&out_mutex)){
    //no way to recover from this
    return -1;
  }
  return 0;
}

/**
 * convenience function that prints and recycles a queue of messages
 */
static int print_and_recycle_messages(struct log_msg * head, struct log_msg * tail){
  if(head != NULL){
    assert(tail != NULL);
    print_messages(head);
    if(recycle_messages(head, tail)){
      return -1;
    }
  }
  return 0;
}

/**
 * The main function for the worker thread
 */
static void * run_logger(void * arg){

  // lock mutex before starting
  
  if(pthread_mutex_lock(&in_mutex)){
    return NULL;;
  }
  
  while(true){

    /*
     * keep cycling until the in queue is empty
     * every time when the mutex is unlocked during printing, additional messages
     * could be added to the queue that need to be handled
     * if we do not do this the thread may not wake up to print them until the next message
     * is pushed on the queue
     */
    while(in_head != NULL){
      
      struct log_msg * head = in_head;
      struct log_msg * tail = in_tail;
      in_head = NULL;
      in_tail = NULL;
      
      /*
       * unlock in mutex and print messages
       * note that more log messages can be pushed on the queue while
       * the mutex is unlocked but the idea is to ensure other threads are not blocked
       * trying to log messages while we are printing them
       */
      if(pthread_mutex_unlock(&in_mutex)){
	return NULL;
      }
      
      if(print_and_recycle_messages(head, tail)){
	pthread_mutex_unlock(&in_mutex);
	return NULL;
      }

      // lock the mutex again so that we can check the in queue again
      
      if(pthread_mutex_lock(&in_mutex)){
	return NULL;
      }
    }

    // check if logging system is still running  
    if(running){
      // wait for more messages
      if(pthread_cond_wait(&in_cond, &in_mutex)){
	break;
      }
    }else{
      /*
       * logger has been stopped, loop should print remaining messages and stop
       * it should do this while holding the in mutex, to ensure that no more
       * messages can be pushed onto the queue until after the logging system has stopped
       * note that there can be no deadlock with the out mutex,
       * as that one is never held by any other thread that also holds the in mutex
       */
      print_and_recycle_messages(in_head, in_tail);
      in_head = NULL;
      in_tail = NULL;
      
      // unlock the in mutex and break the loop
      pthread_mutex_unlock(&in_mutex);
      break;
    }
  }
  return NULL;
}

int start_logger(FILE * output_file){
  assert(output_file != NULL);

  /*
   * lock the in mutex to ensure no one logs anything
   * while we are starting up
   */
  if(pthread_mutex_lock(&in_mutex)){
    return -1;
  }

  if(running){
    // log system already up => unlock and return
    pthread_mutex_unlock(&in_mutex); 
    return -1;
  }

  // set up parameters and create worker thread
  
  file = output_file;
  running = true;

  if(pthread_create(&worker, NULL, run_logger, NULL)){
    return -1;
  }

  // unlock in mutex so that worker thread can start
  if(pthread_mutex_unlock(&in_mutex)){
    return -1;
  }

  return 0;
}

/**
 * push log message on the in queue
 */
static int push_log_msg(struct log_msg * msg){
  assert(msg != NULL);
  assert(msg->next == NULL);
  
  if(pthread_mutex_lock(&in_mutex)){
    return -1;
  }

  if(in_tail == NULL){
    assert(in_head == NULL);
    in_head = msg;
    in_tail = msg;
  }else{
    in_tail->next = msg;
    in_tail = msg;
  }

  if(pthread_mutex_unlock(&in_mutex)){
    return -1;
  }

  if(pthread_cond_signal(&in_cond)){
    return -1;
  }
  
  return 0;
}

/**
 * create and send a log message
 */
int log_msg(enum log_priority priority, const char * format, ...){
  struct log_msg * msg = get_log_msg();
  if(msg == NULL){
    return -1;
  }
  
  va_list args;
  va_start(args, format);
  msg->priority = priority;
  int len = vsnprintf(msg->text, MAX_LOG_MSG_LEN, format, args);
  if(len < 0){
    recycle_messages(msg, msg);
    return -1;
  }else if(len < MAX_LOG_MSG_LEN){
    msg->text[len] = '\0';
  }else{
    msg->text[MAX_LOG_MSG_LEN - 1] = '\0'; 
  }
  va_end(args);

  if(push_log_msg(msg)){
    // unable to push the messages => recycle it and return error
    recycle_messages(msg, msg);
    return -1;
  }

  return 0;
}

/**
 * sets thread local min log priority
 */
void set_min_log_priority(enum log_priority priority){
  min_priority = priority;
}


/**
 * returns thread local min log priority
 */
enum log_priority get_min_log_priority(){
  return min_priority;
}

/**
 * stops the logger
 */
int stop_logger(){
  /*
   * lock the in mutex to ensure no one logs while we are
   * giving the signal to shut down
   */
  if(pthread_mutex_lock(&in_mutex)){
    return -1;
  }

  // if we are not running => unlock and return error
  if(running == false){
    pthread_mutex_unlock(&in_mutex);
    return -1;
  }

  running = false;

  // unlock the mutex so that logging can continue for the moment
  if(pthread_mutex_unlock(&in_mutex)){
    return -1;
  }

  // send a signal to wake up the worker thread if it is asleep
  if(pthread_cond_signal(&in_cond)){
    return -1;
  }

  // wait for worker thread shutdown
  if(pthread_join(worker, NULL)){
    return -1;
  }

  
  /*
   * free log message blocks and dispose of queues
   * we lock the mutex again to ensure no one pushes any log messages on those queues
   * while we are doing this.
   */
  
  if(pthread_mutex_lock(&in_mutex)){
    return -1;
  }

  if(in_head != NULL){
    print_messages(in_head);
  }
  
  while(block != NULL){
    struct log_msg_block * next = block->next;
    free(block);
    block = next;
  }
  in_head = NULL;
  in_tail = NULL;
  out_head = NULL;

  if(pthread_mutex_unlock(&in_mutex)){
    return -1;
  }
  
  return 0;
}
