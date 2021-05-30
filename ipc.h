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

#ifndef IPC_H
#define IPC_H

#include "deque.h"
#include "protocol.h"

#include <pthread.h>

#define MAX_IPC_CHANNELS 32

/**
 * basic ipc message
 */
struct ipc_msg{
  int sender;
  int recipient;
  struct protocol_msg payload;
  struct ipc_alloc * alloc;
  struct ipc_msg * next;
};

struct ipc_alloc;

/**
 * ipc message queue
 */
struct ipc_queue{
  struct ipc_msg * head;
  struct ipc_msg * tail;
  struct ipc_alloc * alloc;
};

/**
 * ipc message allocator
 */
struct ipc_alloc{
  struct deque deque;
  pthread_mutex_t mutex;
  struct ipc_queue recycle_queue;
};

/**
 * concurrent ipc message queue
 */
struct ipc_mt_queue{
  struct ipc_queue queue;
  bool active;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
};

/**
 * ipc channel state
 */
enum ipc_state{
	       IPC_STATE_INACTIVE,
	       IPC_STATE_STARTING,
	       IPC_STATE_ACTIVE,
	       IPC_STATE_STOPPING
};


/**
 * mediates an async duplex message based communication
 * channel through a file descriptor
 */
struct ipc_channel{
  int id;
  enum ipc_state state;
  int fd;
  pthread_mutex_t mutex;

  struct ipc_mt_queue send_queue;
  struct ipc_msg * send_msg;
  pthread_t producer;

  struct ipc_mt_queue * receive_queue;
  struct ipc_msg * receive_msg;
  pthread_t consumer;

  struct protocol_state protocol;
};

/**
 * A single peer to peer connection
 * through an ipc channel
 */
struct ipc_duplex{
  struct ipc_channel channel;
  struct ipc_mt_queue receive_queue;
};

/**
 * handles multiple concurrent connections
 * through ipc channels
 */
struct ipc_multiplex{
  struct ipc_channel channels[MAX_IPC_CHANNELS];
  bool acquired[MAX_IPC_CHANNELS];
  struct ipc_mt_queue receive_queue;
  struct ipc_alloc * alloc;
  pthread_mutex_t mutex;
};


int init_ipc_alloc(struct ipc_alloc * alloc);

int dispose_ipc_alloc(struct ipc_alloc * alloc);


struct ipc_msg * create_ipc_msg(struct ipc_alloc * alloc);

int destroy_ipc_msg(struct ipc_msg * msg);


void init_ipc_queue(struct ipc_queue * q, struct ipc_alloc * alloc);

void push_onto_ipc_queue(struct ipc_queue * q, struct ipc_msg *msg);

void move_onto_ipc_queue(struct ipc_queue * dest, struct ipc_queue * src);

struct ipc_msg * pop_from_ipc_queue(struct ipc_queue * q);

int clear_ipc_queue(struct ipc_queue * q);

int dispose_ipc_queue(struct ipc_queue * q);


int init_ipc_duplex(struct ipc_duplex * d, struct ipc_alloc * alloc);

int open_ipc_duplex(struct ipc_duplex * d, int fd);

int send_to_ipc_duplex(struct ipc_duplex *dest, struct ipc_msg * msg);

int send_all_to_ipc_duplex(struct ipc_duplex * dest, struct ipc_queue * src);

int receive_from_ipc_duplex(struct ipc_msg ** dest, struct ipc_duplex * src);

int try_receive_from_ipc_duplex(struct ipc_msg ** dest, struct ipc_duplex * src);

int receive_all_from_ipc_duplex(struct ipc_queue * dest, struct ipc_duplex * src);

int try_receive_all_from_ipc_duplex(struct ipc_queue * dest, struct ipc_duplex * src);

int close_ipc_duplex(struct ipc_duplex * d);

int dispose_ipc_duplex(struct ipc_duplex * d);


int init_ipc_multiplex(struct ipc_multiplex * m, struct ipc_alloc * alloc);

int open_ipc_multiplex(struct ipc_multiplex * m);

int open_ipc_channel(struct ipc_multiplex * m, int fd);

int send_to_ipc_multiplex(struct ipc_multiplex *dest, struct ipc_msg * msg);

int receive_from_ipc_multiplex(struct ipc_msg ** dest, struct ipc_multiplex * src);

int try_receive_from_ipc_multiplex(struct ipc_msg ** dest, struct ipc_multiplex * src);

int receive_all_from_ipc_multiplex(struct ipc_queue * dest, struct ipc_multiplex * src);

int try_receive_all_from_ipc_multiplex(struct ipc_queue * dest, struct ipc_multiplex * src);

int close_ipc_channel(struct ipc_multiplex * m, int id);

int close_ipc_multiplex(struct ipc_multiplex * m);

int dispose_ipc_multiplex(struct ipc_multiplex * m);

#endif
