// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-08
// controller.h



/*	This file contains the function prototypes and definitons for the
	memory controller algorithm.
	The main function of the memory is the command scheduler which determines
	which command to send next based on the requests in the queue.
*/


#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "fileio.h"
#include "debug.h"
#include "queue.h"


/* DRAM INFO */
// These definitions can be altered based on the system the memory controller is working with
#define NUM_BANKS_PER_GROUP 4		// number of banks per group  *** CHANGE TO BANKS PER GROUP?
#define NUM_GROUPS 4			// number of bank groups      
#define CPU_CLOCK_SPEED 3.2		// CPU clock speed in GHz
#define DIMM_CLOCK_SPEED 1.6		// DRAM/DIMM clock speed in GHz


/* SCHEDULING POLICY SETTINGS */
#define CLOSED_PAGE_INORDER 0
#define FIRSTREADY_FIRSTACCESS 1

/* DRAM TIMING PARAMETERS */
#define tRC 76		// Row Cycle Time - Time between ACTIVATE commands
#define tRAS 52		// Row Access Time - Time between ACTIVATE command and when you start
#define tRRD_L 6    	// Row to Row Delay Long - Time between consecutive ACTIVATE commands in the same bank group.
#define tRRD_S 4    	// Row to Row Delay Short - Time between consecutive ACTIVATE commands in different bank groups.
#define tRP 24		// Precharge Time - Time for the DRAM to precharge a bank before an ACTIVATE command.
#define tCWL 20		// CAS Write Latency - Time between when data is written to the first data in
#define tCL 24		// CAS Latency - Time between a READ command and when data is available
#define tRCD 24 	// Read Column Delay - Time between issuing an ACTIVATE 
			// command for a group/bank/row and issuing the READ command for that row.
#define tWR 20		// Write Recovery Time - 
#define tRTP 12		// Read to Precharge - Time to issue PRECHARGE command after READ command
#define tCCD_L 8    	// Column to Column Delay Long - Time to issue consecutive READ/WRITE commands to the same bank group.
#define tCCD_S 4    	// Column to Column Delay Short - Time to issue consecutive READ/WRITE commands to different bank groups.
#define tBURST 4    	// Burst Length - Time to complete a burst transfer. 8 bytes on 4 clock cycles
#define tWTR_L 12   	// Write To Read Long - Delay from start of internal WRITE transaction 
			// to internal READ command in the same bank group.
#define tWTR_S 4    	// Write To Read Short - Delay from start of internal WRITE transaction
			// to internal READ command in different bank groups.
#define tREFI 12480  	// Refresh Interval - Time to issue refresh commands
#define tRFC 560    	// Refresh Cycle Time - Time to complete a refresh command



/* MEMORY REFERENCES MADE BY THE CPU */
#define I_FETCH 0		// This is the list of possible commands that the CPU will send
#define I_WRITE 1		// to the memory controller.
#define I_READ 2
#define I_INVALID 3


/* DRAM COMMANDS */
#define DRAM_COM_NONE 0		// List of the possible commands that the memory controller will send
#define DRAM_COM_ACT 1		// to the DRAM
#define DRAM_COM_READ 2
#define DRAM_COM_WRITE 3
#define DRAM_COM_REF 4
#define DRAM_COM_PRE 5


/* BIT WISE MACROS FOR EXTRACTING AND SETTING MEMORY ADDRESS PARTS*/
#define MASK_2 0x03
#define MASK_3 0x07
#define MASK_7 0x7F
#define MASK_16 0xFFFF
#define MASK_ROW 0x3FFFC0000

#define BURST_SHIFT 3
#define GROUP_SHIFT 6
#define BANK_SHIFT 8
#define COL_SHIFT 10
#define ROW_SHIFT 17


#define GETROW(address)		((unsigned short)((address >> ROW_SHIFT) & (MASK_16)))
#define GETGROUP(address)	((char)(((unsigned long long int)address >> GROUP_SHIFT) & (MASK_2)))
#define GETBANK(address)	((char)(((unsigned long long int)address >> BANK_SHIFT) & (MASK_2)))
#define GETCOL(address)		((char)(((unsigned long long int)address >> COL_SHIFT) & (MASK_7)))
#define GETBURST(address)	((char)(((unsigned long long int)address >> BURST_SHIFT) & (MASK_3)))


/* DRAM BANK STATUSES */
#define BANK_IDLE 0				// bank is precharged and waiting for an ACTIVATE command
#define BANK_ACTIVE 1			// ACTIVATE command has been issued
#define BANK_PRECHARGE 2


/* MEMORY CONTROLLER INTERNALS */
#define OK_TO_SEND 1				// result of analyzing the memory requests if we have a command to send
#define DO_NOT_SEND 0				// no command to send
#define STAT_TIME 0
#define STAT_PAGE_HIT 1
#define STAT_PAGE_MISS 2


/* MEMORY CONTROLLER DATA STRUCTURES */
// Organizes the information of the DRAM command to send
struct dram_command {
	unsigned char status;			
	unsigned char group;			// Decoded group from the memory address
	unsigned char bank;			// Decoded bank from the memory address		
	unsigned short col;			// Decoded column from the memory address
	unsigned short row;
	unsigned long long time;
	unsigned long long overflow;
	unsigned long long int address;		// address recieved from the CPU

	struct dram_command * next;
};


// this is a sub structure for the memory request to handle the command information
struct command_info {
	char status;		// keeps track of the commands issued to the DRAM
	unsigned long time;	// keeps track of the time since the command was issued
};


// Request information received from the CPU
struct cpu_request {
	int type;			// request type from the CPU
	unsigned long long int time;	// time request was received from the CPU 
	unsigned long long int address;	// address recieved from the CPU
};


// Main structure that tracks the memory requests in the queue and what the commands
// issed to them are.
struct mem_req {
	struct cpu_request request;	// CPU request information
	unsigned char group;		// Decoded group from the memory address
	unsigned char bank;		// Decoded bank from the memory address		
	unsigned short col;		// Decoded column from the memory address
	unsigned short row;

	unsigned short queue_time;	// keeps track of how long the request has been in the queue

	struct command_info command;	// DRAM command information

	struct mem_req * next;
};


// relative time since each command was issued
struct command_time {
	int activate;		// Each one of these variables tracks the time since the command was issued
	int read;		// They are reset when the command is issued and automatically incremented
	int write;		// on every clock tick
	int refresh;
	int precharge;
};


// keeps track of the current status of the bank
struct bank_info {
	unsigned short row;		// currently open row
	unsigned char status;		// status of the bank: open/closed
	unsigned char command;		// last command that was issued to the bank
	struct command_time time;	// tracks the time since every command was issued
};


// keeps track of the current status of the bank group
struct bank_group {
	struct bank_info bank[NUM_BANKS_PER_GROUP];	// info for each bank
	char command;					// the last command that was issued to the group. Use this for CCD_L/S and 
	struct command_time time;			// tracks the time since every command was issued
};


// maintains clock data members
struct clock_control {
	unsigned long long int overflow;	// keeps track of the overflow
	unsigned long long int count;		// Keeps track of the CPU clock
	unsigned long long int old;		// the maximum value of the counter
	char mult;				// multiplier for the clock/DRAM conversion
};


// tracks diagnostic/performance information for the memory controller
struct stats_info {
	unsigned int lowtime;			// tracks what the longest time something starved for
	unsigned int hightime;			// tracks the fastest request service
	unsigned int pagehit;
	unsigned int pagemiss;
};


// Main data structure for the memory controller that keep track of the group/bank settings and the time
struct mem_control {
	struct bank_group group[NUM_GROUPS];	// the DIMM/DRAM groups and banks information
	struct clock_control clock;		// maintains clock data members
	struct stats_info stats;		// performance information
};


/*		FUNCTION PROTOTYPES		*/
void init_controller(void);

void inc_clock(struct mem_req * current);

void inc_clock_bankgroup(void);

unsigned long long get_clock(void);

void close_bank_status(void);

void get_bank_info(struct mem_req * curr, struct bank_info * bank);

char command_scheduler(struct dram_command * comsend);

char compute_command_none(struct mem_req ** curr, struct dram_command * comsend, struct bank_info bank);

char compute_command_activate(struct mem_req ** curr, struct dram_command * comsend, struct bank_info bank);

char compute_command_readwrite(struct mem_req ** curr, struct dram_command * comsend, struct bank_info bank);

void set_command_info(struct dram_command * command, struct mem_req * current);

void update_request_command(struct mem_req * request, char command);

char check_tWTR(struct mem_req * current);

char check_tRRD(struct mem_req * current);

char check_tCCD(struct mem_req * current);

char check_read_write_conflict();

void update_bank_status(struct mem_req * curr, char command);

void d_print_controller_stats();

void d_update_stats(char stat, unsigned int time);

#endif // !CONTROLLER_H
