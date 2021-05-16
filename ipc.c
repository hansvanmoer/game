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

#include "ipc.h"
#include "logger.h"
#include "status.h"
#include "thread_utils.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>


#define IPC_MSG_BLOCK_LEN 32

static int init_ipc_mt_queue(struct ipc_mt_queue * q, struct ipc_alloc * alloc);

static int push_onto_ipc_mt_queue(struct ipc_mt_queue * q, struct ipc_msg * msg);

static int move_onto_ipc_mt_queue(struct ipc_mt_queue * dest, struct ipc_queue * src);

static int pop_from_ipc_mt_queue(struct ipc_msg ** dest, struct ipc_mt_queue * src);

static int try_pop_from_ipc_mt_queue(struct ipc_msg ** dest, struct ipc_mt_queue * src);

static int move_from_ipc_mt_queue(struct ipc_queue * dest, struct ipc_mt_queue * src);

static int try_move_from_ipc_mt_queue(struct ipc_queue * dest, struct ipc_mt_queue * src);

static int start_ipc_mt_queue(struct ipc_mt_queue * q);

static int stop_ipc_mt_queue(struct ipc_mt_queue *q);

static int dispose_ipc_mt_queue(struct ipc_mt_queue * q);

static int init_ipc_channel(struct ipc_channel * ch, int id, struct ipc_mt_queue * receive_queue);

static int start_ipc_channel(struct ipc_channel * ch, int fd);

static int stop_ipc_channel(struct ipc_channel * ch);

static int dispose_ipc_channel(struct ipc_channel * ch);

/*
 * ipc alloc functions
 */


int init_ipc_alloc(struct ipc_alloc * alloc){
  assert(alloc != NULL);

  if(init_named_mutex(&alloc->mutex, "ipc alloc")){
    return -1;
  }
  
  init_deque(&alloc->deque, sizeof(struct ipc_msg), IPC_MSG_BLOCK_LEN);
  init_ipc_queue(&alloc->recycle_queue, alloc);
  
  return 0;
}

int dispose_ipc_alloc(struct ipc_alloc * alloc){
  assert(alloc != NULL);

  int result = dispose_named_mutex(&alloc->mutex, "ipc alloc");

  dispose_deque(&alloc->deque);

  //no need to dispose recycle queue as all messages are already freed
  return result;
}

/*
 * ipc msg functions
 */

struct ipc_msg * create_ipc_msg(struct ipc_alloc * alloc){
  assert(alloc != NULL);

  if(lock_named_mutex(&alloc->mutex, "ipc alloc")){
    return NULL;
  }

  struct ipc_msg * msg = pop_from_ipc_queue(&alloc->recycle_queue);
  
  if(msg == NULL){
    msg = (struct ipc_msg *)emplace_onto_deque(&alloc->deque);
    if(msg == NULL){
      LOG_ERROR("could not allocate ipc message");
    }else{
      msg->alloc = alloc;
    }
  }

  if(unlock_named_mutex(&alloc->mutex, "ipc_alloc")){
    push_onto_ipc_queue(&alloc->recycle_queue, msg);
    msg = NULL;
  }
  
  return msg;
}

int destroy_ipc_msg(struct ipc_msg * msg){
  assert(msg != NULL);
  
  if(lock_named_mutex(&msg->alloc->mutex, "ipc alloc")){
    return -1;
  }
  
  push_onto_ipc_queue(&msg->alloc->recycle_queue, msg);

  if(unlock_named_mutex(&msg->alloc->mutex, "ipc alloc")){
    return -1;
  }

  return 0;
}

/*
 * ipc queue functions
 */

void init_ipc_queue(struct ipc_queue * q, struct ipc_alloc * alloc){
  assert(q != NULL);
  assert(alloc != NULL);

  q->head = NULL;
  q->tail = NULL;
  q->alloc = alloc;
}

void push_onto_ipc_queue(struct ipc_queue * q, struct ipc_msg *msg){
  assert(q != NULL);
  assert(msg != NULL);
  assert(msg->alloc == q->alloc);
  
  if(q->head == NULL){
    assert(q->tail == NULL);
    q->head = msg;
  }else{
    assert(q->tail != NULL);
    q->tail->next = msg;
  }
  q->tail = msg;
  msg->next = NULL;
}

void move_onto_ipc_queue(struct ipc_queue * dest, struct ipc_queue * src){
  assert(dest != NULL);
  assert(src != NULL);
  assert(src->alloc == dest->alloc);

  if(src->head == NULL){
    assert(src->tail == NULL);
  }else{
    assert(src->tail != NULL);
    if(dest->head == NULL){
      assert(dest->tail == NULL);
      dest->head = src->head;
    }else{
      assert(dest->tail != NULL);
      dest->tail->next = src->head;
    }
    dest->tail = src->tail;
    assert(src->tail->next == NULL);
  }
  src->head = NULL;
  src->tail = NULL;
}

struct ipc_msg * pop_from_ipc_queue(struct ipc_queue * q){
  assert(q != NULL);

  if(q->head == NULL){
    assert(q->tail == NULL);
    return NULL;
  }else{
    assert(q->tail != NULL);
    struct ipc_msg * msg = q->head;
    if(q->head == q->tail){
      q->head = NULL;
      q->tail = NULL;
    }else{
      q->head = q->head->next;
    }
    return msg;
  }
}

int clear_ipc_queue(struct ipc_queue * q){
  assert(q != NULL);

  if(q->head == NULL){
    return 0;
  }
  
  if(lock_named_mutex(&q->alloc->mutex, "ipc alloc")){
    return -1;
  }
  
  move_onto_ipc_queue(&q->alloc->recycle_queue, q);

  if(unlock_named_mutex(&q->alloc->mutex, "ipc alloc")){
    return -1;
  }

  return 0;
}

int dispose_ipc_queue(struct ipc_queue * q){
  int result = clear_ipc_queue(q);
  q->head = NULL;
  q->tail = NULL;
  q->alloc = NULL;
  return result;
}

/*
 * ipc mt queue functions
 */

static int init_ipc_mt_queue(struct ipc_mt_queue * q, struct ipc_alloc * alloc){
  assert(q != NULL);

  if(init_named_mutex(&q->mutex, "ipc mt queue")){
    return -1;
  }
  if(pthread_cond_init(&q->cond, NULL)){
    set_status(STATUS_CREATE_CV_FAILED);
    LOG_ERROR("could not create condition variable for ipc mt queue");
    dispose_named_mutex(&q->mutex, "ipc mt queue");
    return -1;
  }
  
  init_ipc_queue(&q->queue, alloc);
  q->active = false;
  return 0;
}

static int push_onto_ipc_mt_queue(struct ipc_mt_queue * q, struct ipc_msg * msg){
  assert(q != NULL);
  assert(msg != NULL);
  
  if(lock_named_mutex(&q->mutex, "ipc mt queue")){
    return -1;
  }

  push_onto_ipc_queue(&q->queue, msg);
  
  if(unlock_named_mutex(&q->mutex, "ipc mt queue")){
    return -1;
  }

  if(pthread_cond_signal(&q->cond)){
    set_status(STATUS_SIGNAL_CV_FAILED);
    LOG_ERROR("could not signal ipc msg consumer");
    return -1;
  }

  return 0;
}

static int move_onto_ipc_mt_queue(struct ipc_mt_queue * dest, struct ipc_queue * src){
  assert(dest != NULL);
  assert(src != NULL);

  if(lock_named_mutex(&dest->mutex, "ipc mt queue")){
    return -1;
  }

  move_onto_ipc_queue(&dest->queue, src);
  
  if(unlock_named_mutex(&dest->mutex, "ipc mt queue")){
    return -1;
  }

  if(pthread_cond_signal(&dest->cond)){
    set_status(STATUS_SIGNAL_CV_FAILED);
    LOG_ERROR("could not signal ipc msg consumer");
    return -1;
  }

  return 0;
}

static int pop_from_ipc_mt_queue(struct ipc_msg ** dest, struct ipc_mt_queue * src){
  assert(dest != NULL);
  assert(src != NULL);

  if(lock_named_mutex(&src->mutex, "ipc mt queue")){
    return -1;
  }
  
  if(!src->active){
    if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
      return -1;
    }
    *dest = NULL;
    return 0;
  }
  
  struct ipc_msg * msg = pop_from_ipc_queue(&src->queue);
  if(msg != NULL){
    if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
      return -1;
    }
    *dest = msg;
    return 0;
  }
    
  if(pthread_cond_wait(&src->cond, &src->mutex)){
    LOG_ERROR("could not wait on ipc msg queue");
    set_status(STATUS_WAIT_CV_FAILED);
    return -1;
  }

  if(!src->active){
    if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
      enable_thread_cancel();
      return -1;
    }
    *dest = NULL;
    enable_thread_cancel();
    return 0;
  }

  msg = pop_from_ipc_queue(&src->queue);

  if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
    return -1;
  }
  *dest = msg;
  return 0;
}

static int try_pop_from_ipc_mt_queue(struct ipc_msg ** dest, struct ipc_mt_queue * src){
  assert(dest != NULL);
  assert(src != NULL);

  if(lock_named_mutex(&src->mutex, "ipc mt queue")){
    return -1;
  }
  
  if(!src->active){
    if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
      return -1;
    }
    *dest = NULL;
    return 0;
  }
  
  struct ipc_msg * msg = pop_from_ipc_queue(&src->queue);
 
  if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
    return -1;
  }
  *dest = msg;
  return 0;
}

static int move_from_ipc_mt_queue(struct ipc_queue * dest, struct ipc_mt_queue * src){
  assert(dest != NULL);
  assert(src != NULL);

  if(lock_named_mutex(&src->mutex, "ipc mt queue")){
    return -1;
  }
  
  if(!src->active){
    if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
      return -1;
    }
    return 0;
  }
  
  struct ipc_queue q;
  init_ipc_queue(&q, src->queue.alloc);
  move_onto_ipc_queue(&q, &src->queue);
 
  if(q.head != NULL){
    if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
      return -1;
    }
    move_onto_ipc_queue(dest, &q);
    return 0;
  }
    
  if(pthread_cond_wait(&src->cond, &src->mutex)){
    LOG_ERROR("could not wait on ipc msg queue");
    return -1;
  }

  if(!src->active){
    if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
      return -1;
    }
    return 0;
  }

  move_onto_ipc_queue(&q, &src->queue);

  if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
    return -1;
  }
  move_onto_ipc_queue(dest, &q);
  return 0;
}

static int try_move_from_ipc_mt_queue(struct ipc_queue * dest, struct ipc_mt_queue * src){
  assert(dest != NULL);
  assert(src != NULL);

  if(lock_named_mutex(&src->mutex, "ipc mt queue")){
    return -1;
  }
  
  if(!src->active){
    if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
      return -1;
    }
    return 0;
  }
  
  move_onto_ipc_queue(dest, &src->queue);
  
  if(unlock_named_mutex(&src->mutex, "ipc mt queue")){
    return -1;
  }
  return 0;
}


static int start_ipc_mt_queue(struct ipc_mt_queue * q){
  if(lock_named_mutex(&q->mutex, "ipc mt queue")){
    return -1;
  }

  if(q->active){
    if(unlock_named_mutex(&q->mutex, "ipc mt queue")){
      return -1;
    }
    return 0;
  }

  q->active = true;
  
  if(unlock_named_mutex(&q->mutex, "ipc mt queue")){
    return -1;
  }

  if(pthread_cond_signal(&q->cond)){
    LOG_ERROR("could not signal ipc mt condition variable");
    return -1;
  }

  return 0;
}

static int stop_ipc_mt_queue(struct ipc_mt_queue *q){
  if(lock_named_mutex(&q->mutex, "ipc mt queue")){
    return -1;
  }
  
  if(!q->active){
    if(unlock_named_mutex(&q->mutex, "ipc mt queue")){
      return -1;
    }
    return 0;
  }
  
  q->active = false;
  
  if(unlock_named_mutex(&q->mutex, "ipc mt queue")){
    return -1;
  }
  
  if(pthread_cond_signal(&q->cond)){
    LOG_ERROR("could not signal ipc mt condition variable");
    return -1;
  }

  return 0;
}

static int dispose_ipc_mt_queue(struct ipc_mt_queue * q){
  assert(q != NULL);
  assert(!q->active);
  
  int result = dispose_ipc_queue(&q->queue);

  if(pthread_cond_destroy(&q->cond)){
    LOG_ERROR("could not destroy condition variable for ipc mt queue");
    result = -1;
  }
  
  if(dispose_named_mutex(&q->mutex, "ipc mt queue")){
    result = -1;
  }

  return result;
}

/*
 * ipc channel functions
 */

static int init_ipc_channel(struct ipc_channel * ch, int id, struct ipc_mt_queue * receive_queue){
  assert(ch != NULL);
  assert(id != -1);
  assert(receive_queue != NULL);

  struct ipc_alloc * alloc = receive_queue->queue.alloc;
  
  ch->state = IPC_STATE_INACTIVE;
  ch->fd = -1;
  if(init_named_mutex(&ch->mutex, "ipc channel")){
    return -1;
  }

  if(init_ipc_mt_queue(&ch->send_queue, alloc)){
    dispose_named_mutex(&ch->mutex, "ipc channel");
    return -1;
  }

  ch->receive_queue = receive_queue;

  if(init_protocol_state(&ch->protocol)){
    dispose_ipc_mt_queue(&ch->send_queue);
    dispose_named_mutex(&ch->mutex, "ipc channel");
    return -1;
  }
  
  return 0;
}

static bool is_running(struct ipc_channel * ch){
  if(lock_named_mutex(&ch->mutex, "ipc channel")){
    return false;
  }
  if(ch->state == IPC_STATE_ACTIVE){
    unlock_named_mutex(&ch->mutex, "ipc channel");
    return true;
  }
  lock_named_mutex(&ch->mutex, "ipc channel");
  return false;
}

static void * produce_ipc_msg(void * arg){
  struct ipc_channel * ch = (struct ipc_channel *)arg;
  struct ipc_alloc * alloc = ch->receive_queue->queue.alloc;
  
  while(true){

    if(!is_running(ch)){
      break;
    }
    
    ch->receive_msg = create_ipc_msg(alloc);
    ch->receive_msg->recipient = ch->id;
    
    if(read_protocol_msg(&ch->protocol, &ch->receive_msg->payload, ch->fd)){
      LOG_ERROR("error while reading ipc message");
    }else{
      if(push_onto_ipc_mt_queue(ch->receive_queue, ch->receive_msg)){
	LOG_ERROR("could not push ipc message onto receive queue");
      }
      ch->receive_msg = NULL;
    }
  }
  
  return NULL;
}

static void * consume_ipc_msg(void * arg){
  struct ipc_channel * ch = (struct ipc_channel *)arg;
 
  while(true){

    if(!is_running(ch)){
      break;
    }
    
    if(pop_from_ipc_mt_queue(&ch->send_msg, &ch->send_queue)){
      LOG_ERROR("could not pop message from send queue");
      break;
    }

    if(ch->send_msg == NULL){
      break;
    }else{
      if(write_protocol_msg(&ch->protocol, ch->fd, &ch->send_msg->payload)){
	LOG_ERROR("error while writing ipc message");
      }
      destroy_ipc_msg(ch->send_msg);
      ch->send_msg = NULL;
    }
  }

  return NULL;
}

static int start_ipc_channel(struct ipc_channel * ch, int fd){
  assert(ch != NULL);

  if(lock_named_mutex(&ch->mutex, "ipc channel")){
    return -1;
  }

  ch->fd = fd;

  if(ch->state != IPC_STATE_INACTIVE){
    unlock_named_mutex(&ch->mutex, "ipc channel");
    return -1;
  }

  if(start_ipc_mt_queue(&ch->send_queue)){
    unlock_named_mutex(&ch->mutex, "ipc channel");
    return -1;
  }
  
  ch->state = IPC_STATE_STARTING;
  ch->receive_msg = NULL;
  ch->send_msg = NULL;
  
  if(pthread_create(&ch->consumer, NULL, &consume_ipc_msg, ch)){
    LOG_ERROR("could not create ipc msg consumer");
    stop_ipc_mt_queue(ch->receive_queue);   
    stop_ipc_mt_queue(&ch->send_queue);
    ch->state = IPC_STATE_STOPPING;
    unlock_named_mutex(&ch->mutex, "ipc channel");
    ch->state = IPC_STATE_INACTIVE;
    return -1;
  }

  if(pthread_create(&ch->producer, NULL, &produce_ipc_msg, ch)){
    LOG_ERROR("could not create ipc msg producer");
    stop_ipc_mt_queue(ch->receive_queue);   
    stop_ipc_mt_queue(&ch->send_queue);
    pthread_cancel(ch->consumer);
    pthread_join(ch->consumer, NULL);
    ch->state = IPC_STATE_STOPPING;
    unlock_named_mutex(&ch->mutex, "ipc channel");
    ch->state = IPC_STATE_INACTIVE;
    return -1;
  }

  ch->state = IPC_STATE_ACTIVE;
  
  if(unlock_named_mutex(&ch->mutex, "ipc channel")){
    stop_ipc_mt_queue(ch->receive_queue);   
    stop_ipc_mt_queue(&ch->send_queue);
    pthread_cancel(ch->consumer);
    pthread_join(ch->consumer, NULL);
    pthread_cancel(ch->producer);
    pthread_join(ch->producer, NULL);
    ch->state = IPC_STATE_INACTIVE;
    return -1;
  }

  return 0;
}

static int stop_ipc_channel(struct ipc_channel * ch){
  assert(ch != NULL);
  
  if(lock_named_mutex(&ch->mutex, "ipc channel")){
    return -1;
  }

  if(ch->state != IPC_STATE_ACTIVE){
    unlock_named_mutex(&ch->mutex, "ipc channel");
    return -1;
  }

  ch->state = IPC_STATE_STOPPING;
  
  if(unlock_named_mutex(&ch->mutex, "ipc channel")){
    return -1;
  }
  
  int result = 0;
  
  if(stop_ipc_mt_queue(&ch->send_queue)){
    result = -1;
  }
  
  if(pthread_cancel(ch->producer)){
    LOG_ERROR("could not stop ipc producer");
    set_status(STATUS_CANCEL_THREAD_FAILED);
    result = -1;
    pthread_detach(ch->producer);
  }else{
    if(pthread_join(ch->producer, NULL)){
      LOG_ERROR("could not join with ipc producer");
      set_status(STATUS_JOIN_THREAD_FAILED);
      result = -1;
    }
  }
  
  if(pthread_join(ch->consumer, NULL)){
    LOG_ERROR("could not join with ipc consumer");
    set_status(STATUS_JOIN_THREAD_FAILED);
    unlock_named_mutex(&ch->mutex, "ipc channel");
    result = -1;
  }
  
  if(ch->receive_msg != NULL){
    destroy_ipc_msg(ch->receive_msg);
    ch->receive_msg = NULL;
  }

  if(ch->send_msg != NULL){
    destroy_ipc_msg(ch->send_msg);
    ch->send_msg = NULL;
  }

  if(lock_named_mutex(&ch->mutex, "ipc channel")){
    return -1;
  }
  
  ch->state = IPC_STATE_INACTIVE;
  
  if(unlock_named_mutex(&ch->mutex, "ipc channel")){
    return -1;
  }
  
  return result;
}

static int dispose_ipc_channel(struct ipc_channel * ch){
  assert(ch != NULL);

  int result = 0;

  if(dispose_ipc_mt_queue(&ch->send_queue)){
    result = -1;
  }

  if(dispose_named_mutex(&ch->mutex, "ipc channel")){
    result = -1;
  }

  dispose_protocol_state(&ch->protocol);
  
  return result;
}

/*
 * ipc duplex functions
 */

int init_ipc_duplex(struct ipc_duplex * d, struct ipc_alloc * alloc){
  assert(d != NULL);
  
  if(init_ipc_mt_queue(&d->receive_queue, alloc)){
    return -1;
  }

  if(init_ipc_channel(&d->channel, 0, &d->receive_queue)){
    dispose_ipc_mt_queue(&d->receive_queue);
    return -1;
  }
  
  return 0;
}

int open_ipc_duplex(struct ipc_duplex * d, int fd){
  assert(d != NULL);
  assert(fd != -1);

  if(d->channel.fd != -1){
    set_status(STATUS_INVALID_IPC_STATE);
    LOG_ERROR("ipc duplex already open");
    return -1;
  }

  if(start_ipc_mt_queue(&d->receive_queue)){
    return -1;
  }
  
  if(start_ipc_channel(&d->channel, fd)){
    stop_ipc_mt_queue(&d->receive_queue);
    return -1;
  }

  return 0;
}

int send_to_ipc_duplex(struct ipc_duplex *dest, struct ipc_msg * msg){
  assert(dest != NULL);
  assert(msg != NULL);

  return push_onto_ipc_mt_queue(&dest->channel.send_queue, msg);
}

int send_all_to_ipc_duplex(struct ipc_duplex * dest, struct ipc_queue * src){
  assert(dest != NULL);
  assert(src != NULL);

  return move_onto_ipc_mt_queue(&dest->channel.send_queue, src);
}

int receive_from_ipc_duplex(struct ipc_msg ** dest, struct ipc_duplex * src){
  assert(dest != NULL);
  assert(src != NULL);

  return pop_from_ipc_mt_queue(dest, &src->receive_queue);
}

int try_receive_from_ipc_duplex(struct ipc_msg ** dest, struct ipc_duplex * src){
  assert(dest != NULL);
  assert(src != NULL);
  
  return try_pop_from_ipc_mt_queue(dest, &src->receive_queue);
}

int receive_all_from_ipc_duplex(struct ipc_queue * dest, struct ipc_duplex * src){
  assert(dest != NULL);
  assert(src != NULL);
  
  return move_from_ipc_mt_queue(dest, &src->receive_queue);
}

int try_receive_all_from_ipc_duplex(struct ipc_queue * dest, struct ipc_duplex * src){
  assert(dest != NULL);
  assert(src != NULL);
  
  return try_move_from_ipc_mt_queue(dest, &src->receive_queue);
}

int close_ipc_duplex(struct ipc_duplex * d){
  assert(d != NULL);
  
  int result = stop_ipc_channel(&d->channel);
  d->channel.fd = -1;
  
  if(stop_ipc_mt_queue(&d->receive_queue)){
    result = -1;
  }
  
  return result;
}

int dispose_ipc_duplex(struct ipc_duplex * d){
  assert(d != NULL);

  int result = dispose_ipc_channel(&d->channel);

  if(dispose_ipc_mt_queue(&d->receive_queue)){
    result = -1;
  }
  
  return result;
}


int init_ipc_multiplex(struct ipc_multiplex * m, struct ipc_alloc * alloc){
  assert(m != NULL);
  assert(alloc != NULL);

  if(init_named_mutex(&m->mutex, "ipc multiplex")){
    return -1;
  }
  
  if(init_ipc_mt_queue(&m->receive_queue, alloc)){
    dispose_named_mutex(&m->mutex, "ipc multiplex");
    return -1;
  }

  memset(m->acquired, 0, sizeof(m->acquired));
  
  m->alloc = alloc;

  return 0;
}

int open_ipc_multiplex(struct ipc_multiplex * m){
  assert(m != NULL);

  return start_ipc_mt_queue(&m->receive_queue);
}

static int release_disposed_channel(struct ipc_multiplex * m, int id){
  assert(id >= 0 && id < MAX_IPC_CHANNELS);
  if(lock_named_mutex(&m->mutex, "ipc multiplex")){
     return -1;
   }
   assert(m->acquired[id] == false);
   m->acquired[id] = false;
   
   if(unlock_named_mutex(&m->mutex, "ipc multiplex")){
     return -1;
   }

   return 0;
}

static struct ipc_channel * acquire_channel(struct ipc_multiplex * m){
  assert(m != NULL);

  if(lock_named_mutex(&m->mutex, "ipc multiplex")){
    return NULL;
  }
  struct ipc_channel * ch = NULL;
  int id;
  for(id = 0; ch == NULL && id < MAX_IPC_CHANNELS; ++id){
    if(!m->acquired[id]){
      m->acquired[id] = true;
      ch = &m->channels[id];
      break;
    }
  }
  if(unlock_named_mutex(&m->mutex, "ipc multiplex")){
    return NULL;
  }

  if(init_ipc_channel(ch, id, &m->receive_queue)){
    release_disposed_channel(m, id);
  }
  
  return ch;
}

static int release_ipc_channel(struct ipc_multiplex * m, struct ipc_channel * ch){
  assert(m != NULL);  
  assert(ch != NULL);
  
  int id = ch->id;
  
  int result = dispose_ipc_channel(ch);

  if(release_disposed_channel(m, id)){
    result = -1;
  }
  
  return result;
}

int open_ipc_channel(struct ipc_multiplex * m, int fd){
  assert(m != NULL);
  assert(fd != -1);
  
  struct ipc_channel * ch = acquire_channel(m);
  
  if(ch == NULL){
    set_status(STATUS_IPC_CONNECTION_LIMIT_REACHED);
    LOG_ERROR("maximum number of ipc connections reached");
    return -1;
  }

  if(start_ipc_channel(ch, fd)){
    release_ipc_channel(m, ch);
    return -1;
  }

  return ch->id;
}

int send_to_ipc_multiplex(struct ipc_multiplex *dest, struct ipc_msg * msg){
  assert(dest != NULL);
  assert(msg != NULL);

  int id = msg->recipient;
  if(id < 0 || id > MAX_IPC_CHANNELS){
    LOG_ERROR("invalid ipc recipient: %d", id);
    set_status(STATUS_INVALID_IPC_RECIPIENT);
    return -1;
  }

  return push_onto_ipc_mt_queue(&dest->channels[id].send_queue, msg);
}

int receive_from_ipc_multiplex(struct ipc_msg ** dest, struct ipc_multiplex * src){
  assert(dest != NULL);
  assert(src != NULL);
  
  return pop_from_ipc_mt_queue(dest, &src->receive_queue);
}

int try_receive_from_ipc_multiplex(struct ipc_msg ** dest, struct ipc_multiplex * src){
  assert(dest != NULL);
  assert(src != NULL);

  return try_pop_from_ipc_mt_queue(dest, &src->receive_queue);
}
 
int receive_all_from_ipc_multiplex(struct ipc_queue * dest, struct ipc_multiplex * src){
  assert(dest != NULL);
  assert(src != NULL);

  return move_from_ipc_mt_queue(dest, &src->receive_queue);
}

int try_receive_all_from_ipc_multiplex(struct ipc_queue * dest, struct ipc_multiplex * src){
  assert(dest != NULL);
  assert(src != NULL);

  return try_move_from_ipc_mt_queue(dest, &src->receive_queue);
}
 
int close_ipc_channel(struct ipc_multiplex * m, int id){
  assert(m != NULL);

  struct ipc_channel * ch= &m->channels[id];
  
  int result = stop_ipc_channel(ch);
  
  if(release_ipc_channel(m, ch)){
    result = -1;
  }

  return result;
}

int close_ipc_multiplex(struct ipc_multiplex * m){
  assert(m != NULL);

  if(lock_named_mutex(&m->mutex, "ipc multiplex")){
    return -1;
  }
  int result = 0;
  
  for(int id = 0; id < MAX_IPC_CHANNELS; ++id){
    if(m->acquired[id]){
      if(stop_ipc_channel(&m->channels[id])){
	result = -1;
      }
      if(dispose_ipc_channel(&m->channels[id])){
	result = -1;
      }
      m->acquired[id] = false;
    }
  }
  
  if(unlock_named_mutex(&m->mutex, "ipc multiplex")){
    result = -1;
  }

  if(stop_ipc_mt_queue(&m->receive_queue)){
    result = -1;
  }
  
  return result;
}

int dispose_ipc_multiplex(struct ipc_multiplex * m){
  int result = dispose_ipc_mt_queue(&m->receive_queue);

  if(dispose_named_mutex(&m->mutex, "ipc multiplex")){
    result = -1;
  }

  return result;
}
