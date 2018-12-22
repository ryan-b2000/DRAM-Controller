// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-08
// fileio.c

/*	This file contains the function definitions and private data members for the file input/ouput
*/


#include "fileio.h"


/* FILE DEFINITIONS */
// Limits for range of acceptable ascii characters in address
#define CHR_LMT_U 91		// upper limit for A-Z
#define CHR_LMT_L 64		// lower limit for A-Z
#define NUM_LMT_U 58		// upper limit for numbers
#define NUM_LMT_L 47		// lower limit for numbers

#define INSTRUCTION_LEN 10	// length of char array for instruction


/* PRIVATE DATA MEMBERS */

static FILE * in_file;					// input file
static FILE * out_file;					// output file

#ifdef D_FILE_WRITE
static FILE * debug_file;				// debugging file
#endif


/* FUNCTION DEFINITIONS */
// Opens the file with name filename and returns status if the file was opened successfully or not
char open_file(char ** filename, char mode)
{
	if (mode == 'r')
	{
		in_file = fopen(*filename, "r");	// open the file
		if (!in_file)
		{
#ifdef D_FILE_OPS
			printf("Unable to open infile.\n");
#endif
			return FILE_FAIL;
		}
	}
	if (mode == 'w')
	{
		out_file = fopen(*filename, "w");	// open the file
		if (!out_file)
		{
#ifdef D_FILE_OPS
			printf("Unable to open outfile.\n");
#endif
			return FILE_FAIL;
		}
	}
#ifdef D_FILE_WRITE
	if (mode == 'w')
	{
		debug_file = fopen("debug.txt", "w");	// open the file
		if (!debug_file)
		{
			printf("Unable to open outfile.\n");
			return FILE_FAIL;
		}
	}
#endif

#ifdef D_FILE_OPS
	printf("File %s opened successfully.\n", *filename);
#endif		
	return FILE_OPEN;
}


// close both the input file and output files and return
char close_files(void)
{
	fclose(in_file);
	fclose(out_file);
#ifdef D_FILE_WRITE
	fclose(debug_file);
#endif
	return 0;
}


// Read a line from the file and store it in a temporary string
char read_line(struct mem_req * request)
{			
	unsigned long long int time = 0;	// temporary for time value of instruction
	char instruction[INSTRUCTION_LEN];	// temporary storage for the instruction
	unsigned long long int address;		// temporary for the address value

	// scan a line a of the file while ignoring whitespace (space, tab, new line)
	fscanf(in_file, "%lld %s %llX", &time, instruction, &address);

	// copy the local variables to the request struct
	request->request.time = time;
	request->request.address = address;

	// error check that the address is valid
	// parse the instruction into the request type
	if (strcmp(instruction, "IFETCH") == 0)
		request->request.type = I_FETCH;
	else if (strcmp(instruction, "READ") == 0)
		request->request.type = I_READ;
	else if (strcmp(instruction, "WRITE") == 0)
		request->request.type = I_WRITE;

	// Parse the address into individual bits
	request->group = GETGROUP(address);
	request->bank = GETBANK(address);
	request->row = GETROW(address);
	request->col = (GETCOL(address) << 3) | (GETBURST(address));

	// debug print 
	d_print_readline(*request);

	// if it is at the end of the file, return error
	if (feof(in_file))
		return END_OF_FILE;

	return NOT_END_OF_FILE;
}


// Outputs the DRAM command to the output file
void output_write(struct dram_command send)
{
#ifdef D_FILE_PRINT
	unsigned char group = send.group;
    	unsigned char bank = send.bank;
    	unsigned short row = send.row;
    	unsigned short col = send.col;
	unsigned char command = send.status;
	unsigned long long time = send.time;

	switch (command)
	{
	case DRAM_COM_ACT:
		fprintf(out_file, "%lld ACT %3X %3X %6X\n", time, group, bank, row);
		break;
	case DRAM_COM_PRE:
		fprintf(out_file, "%lld PRE %3X %3X\n", time, group, bank);
		break;
	case DRAM_COM_READ:
		fprintf(out_file, "%lld RD %3X %3X %10X\n", time, group, bank, col);
		break;
	case DRAM_COM_WRITE:
		fprintf(out_file, "%lld WR %3X %3X %10X\n", time, group, bank, col);
		break;
	case DRAM_COM_REF:
		fprintf(out_file, "%lld REF\n", time);
		break;
	}

	d_print_writeline(send);
#endif
    return;
}


// Outputs the DRAM command to a debug file
void debug_write(struct dram_command send)
{
#ifdef D_FILE_WRITE	
	unsigned char group = send.group;
	unsigned char bank = send.bank;
	unsigned short row = send.row;
	unsigned short col = send.col;
	unsigned char command = send.status;
	unsigned long long int time = send.time;
	unsigned long long int address = send.address;

	switch (command)
	{
	case DRAM_COM_ACT:
		fprintf(debug_file, "%lld,ACT,%X,%X,%X,%llX\n",time, group, bank, row, address);
		break;
	case DRAM_COM_PRE:
		fprintf(debug_file, "%lld,PRE,%X,%X,%llX\n",time, group, bank, address);
		break;
	case DRAM_COM_READ:
		fprintf(debug_file, "%lld,RD,%X,%X,%X,%llX\n",time, group, bank, col, address);
		break;
	case DRAM_COM_WRITE:
		fprintf(debug_file, "%lld,WR,%X,%X,%X,%llX\n",time, group, bank, col, address);
		break;
	case DRAM_COM_REF:
		fprintf(debug_file, "%lld,REF\n", time);
		break;
	}
#endif

	return;
}

