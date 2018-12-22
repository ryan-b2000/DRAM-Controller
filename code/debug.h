// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-08

// debug.h


#ifndef DEBUG_H
#define DEBUG_H


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>


/*************************************************************************
This file contains the customizable debugging information. There are multiple
layers of debugging that you can refine to look at different things.

The Top Debugging Layers need to be enabled to select lower level capabilities.
Top Layer Debugging:
//DEBUG		- master debugging
#ifdef DEBUG
//DEBUG_FILE	- for file i/o debugging
//DEBUG_QUEUE	- for queue debugging
//DEBUG_CLOCK	- for clock debugging
//DEBUG_STATS	- for debugging queue statistics
#endif // DEBUG
Refinement layer debugging is indicated by a "D_".

**************************************************************************/

#include "controller.h"


/* MASTER DEBUG SETTING */

//#define DEBUG		// comment out to turn off all debugging
#ifdef DEBUG		// customize debugging output

// TOP LEVEL DUGGING SETTINGS
//----------------------------------------------------------------------

#define DEBUG_QUEUE		// handles debugging queue information
#define DEBUG_FILE		// handles debugging for file operations
//#define DEBUG_CLOCK		// prints out every clock tick
#define DEBUG_STATS		// prints performance statistics


// REFINED DEBUG SETTINGS FOR THE CLOCK
#ifdef DEBUG_CLOCK		
//----------------------------------------------------------------------

//#define D_ALL_CLOCK		// prints every clock tick
//#define D_CLOCK_JUMP		// prints the time jump for the clock
#endif //DEBUG_CLOCK


// REFINED DEBUG SETTINGS TO CHECK DIFFERENT PARTS OF THE QUEUE
#ifdef DEBUG_QUEUE
//----------------------------------------------------------------------

//#define D_QUEUE_ENQ		// check the enqueue function
//#define D_QUEUE_DEQ		// check the dequeue function
//#define D_QUEUE_DEQ_REQ_SEL		// check the remove by selected address function
#endif // DEBUG_QUEUE


// REFINED DEBUG SETTINGS FOR DEBUGGING FILE OPS
#ifdef DEBUG_FILE
//----------------------------------------------------------------------

#define D_FILE_OPS		// print out file ops debugging
//#define D_FILE_READ		// print out reading of new CPU requests
#define D_FILE_PRINT		// prints out the commmand outputs
#define D_FILE_WRITE		// prints the output to a special file for debugging
#endif //DEBUG_FILE


// REFINED DEBUG SETTINGS FOR STATS PRINTING
#ifdef DEBUG_STATS
//----------------------------------------------------------------------

#define D_STATS_QUEUE				// for debugging queue statistics
#define D_STATS_CONTROL
#endif //DEBUG_STATS

#endif // DEBUG



struct dram_command;

struct mem_req;

void d_print_clock(unsigned long long int clock);

void d_print_readline(struct mem_req print);

void d_print_writeline(struct dram_command print);

void d_print_memreq(struct mem_req print);

void d_enqueue_check(struct mem_req * print);

void d_dequeue_check(struct mem_req * print);

void d_gethead_check(void);

void d_print_end(void);

void d_print_screen_header();

void d_print_clock_jump(unsigned long long clock);

#endif // !DEBUG_H
