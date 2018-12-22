// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-08
// fileio.h

// This file contains the file input output prototypes

#ifndef FILEIO_H
#define FILEIO_H

#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "string.h"
#include "controller.h"

/* FILE DEFINITIONS */
#define FILE_FAIL 0			// Return statuses for file operations
#define FILE_OPEN 1			//
#define END_OF_FILE 0		// so we can stop reading the file and won't keep queueing garbage data
#define NOT_END_OF_FILE 1

/* STRUCTURE PROTOTYPES */
struct dram_command;
struct mem_req;

/* FUNCTION PROTOTYPES */
char open_file(char ** filename, char mode);

char read_line(struct mem_req * request);

char close_files(void);

void output_write(struct dram_command send);

void debug_write(struct dram_command send);

#endif // !FILEIO_H
