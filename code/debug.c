// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-08

// debug.h


#include "debug.h"

// This file contains the function definitions for debugging


// this function prints out the memory request as it has been read in from the file

void d_print_readline(struct mem_req print)
{
#ifdef D_FILE_READ
	printf("Read:   ");
	d_print_memreq(print);
#endif
}


// This function prints the DRAM command header information for screen debugging
void d_print_screen_header()
{
#ifdef D_FILE_PRINT
	printf("\n      TIME     COM   GROUP   BANK     ROW     COL     ADDRESS\n");
#endif
}


// this function prints out the DRAM command that is getting printed to the file
void d_print_writeline(struct dram_command print)
{
	static char printcount = 0;
#ifdef D_FILE_PRINT
	unsigned char group = print.group;
	unsigned char bank = print.bank;
	unsigned short row = print.row;
	unsigned short col = print.col;
	unsigned char command = print.status;
	unsigned long long int time = print.time;
	unsigned long long int address = print.address;

	if (printcount % 10 == 0)
		d_print_screen_header();

	switch (command)
	{
	case DRAM_COM_ACT:
		printf("%10lld     ACT %5X %7X %8X   %17llX\n", time, group, bank, row, address);
		break;
	case DRAM_COM_PRE:
		printf("%10lld     PRE %5X %7X    %25llX\n", time, group, bank, address);
		break;
	case DRAM_COM_READ:
		printf("%10lld      RD %5X %7X %16X  %10llX\n", time, group, bank, col, address);
		break;
	case DRAM_COM_WRITE:
		printf("%10lld      WR %5X %7X %16X  %10llX\n", time, group, bank, col, address);
		break;
	case DRAM_COM_REF:
		printf("%10lld REF\n", time);
		break;
	}

	++printcount;
#endif
	return;
}


// This function prints out the memory request information
void d_print_memreq(struct mem_req print)
{
#ifdef DEBUG
	printf("CPU request: Time: %lld, ", print.request.time);

	if (print.request.type == I_FETCH)
		printf("Inst: FETCH, ");
	else if (print.request.type == I_READ)
		printf("Inst: READ,  ");
	else if (print.request.type == I_WRITE)
		printf("Inst: WRITE, ");

	printf("Address:  %9llX\n", print.request.address);
#endif
	return;
}


// This function prints out notice if something was added to the queue
void d_enqueue_check(struct mem_req * print)
{
#ifdef D_QUEUE_ENQ
	printf("Added:  ");
	d_print_memreq(*print);
#endif
	return;
}


// This function prints out notice if something was removed from the queue
void d_dequeue_check(struct mem_req * print)
{
#ifdef D_QUEUE_DEQ
	printf("Remove: ");
	d_print_memreq(*print);
#endif // DEBUG
	return;
}


// This function prints head of the queue information
void d_gethead_check(void)
{
#ifdef DEBUG
	printf("head is NULL\n");
#endif // DEBUG
	return;
}


// This function prints the end of program notifcation
void d_print_end(void)
{
#ifdef DEBUG
	printf("\nEnding program\n");
#endif
	return;
}


void d_print_clock_jump(unsigned long long clock)
{
#ifdef D_CLOCK_JUMP
	printf("Time jump to: %lld\n", clock);
#endif
	return;
}


// This function prints out the clock information
void d_print_clock(unsigned long long int clock)
{
#ifdef D_ALL_CLOCK
	printf("%lld\n", clock);
#endif // DEBUG
	return;
}

