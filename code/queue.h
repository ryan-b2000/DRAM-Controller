// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-08
// queue.h

// This file contains the definitions and function prototypes for the queue.

#ifndef QUEUE_H
#define QUEUE_H


#include "controller.h"
#include "debug.h"
#include <stdio.h>
#include <stdbool.h>


/* DEFINITIONS */
// Queue status definitions
#define QUEUE_EMPTY 1		// Queue is empty
#define QUEUE_FULL 0		// Queue is full
#define QUEUE_OPEN 2		// Queue has something in it but is not full

#define QUEUE_LEN 16		// maximum number of entries in the queue

#define CLOSE_ROW 0			// status return to the command scheduler if it is ok to
#define LEAVE_ROW_OPEN 1	// close and precharge (no other requests need the row


/* STRUCTURE PROTOTYPES */
struct dram_command;
struct mem_req;


/* FUNCTION PROTOTYPES */
void init_queue(void);

char enqueue_request(struct mem_req * add);

char dequeue_request_select(struct mem_req * remove);

char get_request_info(struct mem_req * current);

void set_request_info(struct mem_req * update);

char check_request_row_addr(struct mem_req * curr);

char get_queue_status(void);

void inc_q_clocks();

void get_queue_headptr(struct mem_req ** current);

void get_queue_end(struct mem_req ** current);

void d_print_stats();

void disp_queue(void);


#endif // !QUEUE_H
