// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-13

// verify_util.h
// Header file for constants, i/o functions, and structs for verify_timing.c

#ifndef VERIFY_UTIL_H
#define VERIFY_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Maximum instruction length
#define I_LEN 4

// Error definitions
#define ERROR_EOF -1
#define ERROR_INSTRUCTION -2

// Debug definitions
#define CONSOLE_DEBUG 1
#define FILE_DEBUG 1

// DEFINES FROM CONTROL.H (To minimize linking errors)
/* DRAM COMMANDS */
#define DRAM_COM_NONE 0
#define DRAM_COM_ACT 1
#define DRAM_COM_READ 2
#define DRAM_COM_WRITE 3
#define DRAM_COM_REFRESH 4
#define DRAM_COM_PRE 5

/* DRAM INFO */
#define NUM_BANKS 4					// number of banks per group
#define NUM_GROUPS 4				// number of bank groups
#define CPU_CLOCK_SPEED 3.2			// CPU clock speed in GHz
#define DIMM_CLOCK_SPEED 1.6		// DRAM/DIMM clock speed in GHz
#define OK_TO_SEND 1				// result of analyzing the memory requests if we have a command to send
#define DO_NOT_SEND 0				// no command to send

/* DRAM TIMING PARAMETERS */
#define tRC 76		// Row Cycle Time - Time between ACTIVATE commands
#define tRAS 52		// Row Access Time - Time between ACTIVATE command and when you start
#define tRRD_L 6    // Row to Row Delay Long - Time between consecutive ACTIVATE commands in the same bank group.
#define tRRD_S 4    // Row to Row Delay Short - Time between consecutive ACTIVATE commands in different bank groups.
#define tRP 24		// Precharge Time - Time for the DRAM to precharge a bank before an ACTIVATE command.
#define tCWL 20		// CAS Write Latency - Time between when data is written to the first data in
#define tCL 24		// CAS Latency - Time between a READ command and when data is available
#define tRCD 24     // Read Column Delay - Time between issuing an ACTIVATE command for a group/bank/row and issuing the READ command for that row.
#define tWR 20		// Write Recovery Time - 
#define tRTP 12		// Read to Precharge - Time to issue PRECHARGE command after READ command
#define tCCD_L 8    // Column to Column Delay Long - Time to issue consecutive READ/WRITE commands to the same bank group.
#define tCCD_S 4    // Column to Column Delay Short - Time to issue consecutive READ/WRITE commands to different bank groups.
#define tBURST 4    // Burst Length - Time to complete a burst transfer. 8 bytes on 4 clock cycles
#define tWTR_L 12   // Write To Read Long - Delay from start of internal WRITE transaction to internal READ command in the same bank group.
#define tWTR_S 4    // Write To Read Short - Delay from start of internal WRITE transaction to internal READ command in different bank groups.
#define tREFI 12480  // Refresh Interval - Time to issue refresh commands
#define tRFC 560    // Refresh Cycle Time - Time to complete a refresh command


#define MAX_TIME 561 // Time greater than any timing constraints

/* DRAM BANK STATUSES */
#define BANK_IDLE 0				// bank is precharged and waiting for an ACTIVATE command
#define BANK_ACTIVE 1			// ACTIVATE command has been issued
#define BANK_PRECHARGE 2
#define BANK_REFRESH 3

/* STRUCTS FOR DRAM_COMMANDS AND BANK INFO */

struct dram_command {
    int valid; // Is this a valid command? 0 is False
    long long time;
    int instruction; // Uses the #define statements in controller.h
    int group;
    int bank;
    int row;
    int col;
};

// relative time since each command was issued
struct command_time {
	int activate;			// Each one of these variables tracks the time since the command was issued
	int read;				// They are reset when the command is issued and automatically incremented
	int write;				// on every clock tick
	int refresh;
	int precharge;
};

/* keeps track of the current status of the bank */
struct bank_info {
	unsigned short row;					// currently open row
	unsigned char status;				// status of the bank: open/closed
	unsigned char command;				// last command that was issued to the bank
	struct command_time time;	// tracks the time since every command was issued
};

/* keeps track of the current status of the bank group */
struct bank_group {
	struct bank_info bank[NUM_BANKS];	// info for each bank
	char command;						// the last command that was issued to the group. Use this for CCD_L/S and 
	struct command_time time;			// tracks the time since every command was issued
};

// ADDITIONAL FUNCTIONS
int read_output(FILE * file, struct dram_command * com_in);
char output_write(FILE * out_file, struct dram_command send);

FILE * open_file(char * filename);
FILE * open_fileout(char * filename);

int close_file(FILE * file);

int reset_time(struct command_time * time_block);
int increment_time(int add_time, struct command_time * time_block);


int time_check_ref(struct bank_group * group, struct dram_command * current_com, FILE * out_file);
int time_check_pre(struct bank_group * group, struct dram_command * current_com, FILE * out_file);
int time_check_act(struct bank_group * group, struct dram_command * current_com, FILE * out_file);
int time_check_read(struct bank_group * group, struct dram_command * current_com, FILE * out_file);
int time_check_write(struct bank_group * group, struct dram_command * current_com, FILE * out_file);



#endif // End of verify_util.h