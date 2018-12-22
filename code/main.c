// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg

// ECE 485/858

// Final Project

// 2018-06-08

// main.c

// This file contains the main algorithmic functions for the memory controller.
// It is the central intersection of the controller files and the file in/out
// It handles the logic for when to queue files and sending any commands the 
// scheduler determines can be sent.


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "debug.h"
#include "fileio.h"
#include "queue.h"
#include "controller.h"


/* FILE DEFINITIONS */
#define INVALID_COMMANDS 0	// determines whether the correct commands were entered
#define VALID_COMMANDS 1	// at the command line


/* FUNCTION PROTOTYPES */ 
char check_flags(int argc);


/* MAIN FUNCTION FOR MEMORY CONTROLLER */
int main(int argc, char * argv[])
{
	struct mem_req next_req;
	// This is the current (or next) request from the CPU, we store the request 
	// here until the simulated time reaches the time of the request coming in 
	// and we can try to add it to the queue 

	struct dram_command send_command;	
	// command scheduler returns the DRAM command to send here

	// input and output file name dynamically allocated so you can pass char * to fopen()
	// compiler gets angry if you try static allocation
	char * infilename = (char*) malloc (sizeof(char)*FILENAME_MAX);	
	char * outfilename = (char*)malloc(sizeof(char)*FILENAME_MAX);

	int f_status;		// Are we at the end of the file? 
	char q_status;		// Is the queue full or empty? 
	char c_status;		// What is the command status?

	// Initialize variables and allocate memory
	init_queue();			// initialize the queue
	init_controller();

	c_status = DO_NOT_SEND;
	q_status = QUEUE_EMPTY;		// queue is empty
	f_status;			// not at the end of the file
	
	if (check_flags(argc) == INVALID_COMMANDS)
		return -1;
	
	strcpy(infilename, argv[1]);
	strcpy(outfilename, argv[2]);			

	// open the files and make sure they opened correctly 
	if (open_file(&infilename, 'r') == FILE_FAIL)
		return -1;

	// open the output file
	if (open_file(&outfilename, 'w') == FILE_FAIL)
		return -1;

	// read in the first request from the CPU 
	f_status = read_line(&next_req);

	// read the next request from the CPU until we get to the end of the file 
	do
	{
		// if the time of the request <= clock time, attempt to add it to the queue
		if (next_req.request.time <= get_clock())
		{
			// if we are at the end of the file, then what is stored in next_req is garbage
			// and everything is added to the queue so we can stop queuing things
			if (f_status != END_OF_FILE)
			{
				// attempt to add the item to the queue
				q_status = enqueue_request(&next_req);

				// if the queue is full, try again later, otherwise something was added so we can
				// read in the next request from the CPU
				if (q_status == QUEUE_EMPTY || q_status == QUEUE_OPEN)
				{
					f_status = read_line(&next_req);
				}
			}	
		}

		// determine any DRAM commands that should be issued and set the time of the command
		// if there is a command, com_req is returned with the information and c_status is set
		c_status = command_scheduler(&send_command);

		// if there is a command to send
		if (c_status == OK_TO_SEND)
		{
			// send the command to the DRAM
			 output_write(send_command);
			debug_write(send_command);
		}	

		// check if any banks are done precharging
		// if the bank is done precharging, then update the status to closed (ready for new request)
		close_bank_status();

		// get the queue status
		q_status = get_queue_status();

		// INCREMENT THE CPU CLOCK
		inc_clock(&next_req);

	// while not the end of file and queue is not empty
	} while (f_status != END_OF_FILE || q_status != QUEUE_EMPTY);		
		

	// close the files
	f_status = close_files();

	// print any statistics information
	d_print_stats();
	d_print_controller_stats();

	// print end of program
	d_print_end();

	return 0;
}

// Checks the input parameters at runtime to make sure they are valid 
char check_flags(int argc)
{
	// Command arguments: executable, input file, output file, debug
	if (argc < 3 || argc > 4)
	{
		printf("Error: Incorrect number of arguments. Terminating program.\n");
		return INVALID_COMMANDS;
	}
	return VALID_COMMANDS;
}
