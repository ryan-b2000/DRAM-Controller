// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-13

//verify_util.c
// Implementation file for utility functions for verify_timing.c

#include "verify_util.h"

#define I_LEN 4

#define ERROR_EOF -1
#define ERROR_INSTRUCTION -2

FILE * open_file(char * filename)
{
	return fopen(filename, "r");
}

FILE * open_fileout(char * filename)
{
	return fopen(filename, "w");
}

int close_file(FILE * file)
{
    return fclose(file);
}

// Read a line from a file of DRAM commands (mem controller output)
int read_output(FILE * file, struct dram_command * com_in)
{
    long long time = 0;
    char instruction[I_LEN];
    int group;
    int bank;
    int row_or_col;
    
    int debug = 0;
    
    if(feof(file))
        return ERROR_EOF;
    //fscanf(file, "%9ld %s %X %X %X", &time, instruction, group, bank, row_or_col); // Might or might not have last argument. Hmmm
    // Or on a refresh, no arguments after instruction
    fscanf(file, "%9lld %s", &time, instruction);
    // STRING COMPARE INSTRUCTION TO THINGS
    if( strcmp(instruction, "REF") == 0) // Refresh
    {
        // set and move on, set rest of stuff to invalid info
        com_in->valid = 1;
        com_in->time = time;
        com_in->instruction = DRAM_COM_REFRESH;
        if(debug)
            printf("Refresh command at time %lld\n", time);

    }
    else if( strcmp(instruction, "PRE") == 0) // actually precharge
    {
        // load in two args for group and bank
        fscanf(file, "%X %X", &group, &bank);
        com_in->valid = 1;
        com_in->time = time;
        com_in->bank = bank;
        com_in->group = group;
        com_in->instruction = DRAM_COM_PRE;
        if(debug)
            printf("Precharge command at time %lld, with group:%X, bank:%X\n", time, group, bank);
    }
    else if( strcmp(instruction, "ACT") == 0 ) // activate
    {
        // load in 3 args
        fscanf(file, "%X %X %X", &group, &bank, &row_or_col);
        // if act than row
        com_in->valid = 1;
        com_in->time = time;
        com_in->bank = bank;
        com_in->group = group;
        com_in->row = row_or_col;
        com_in->instruction = DRAM_COM_ACT;
        if(debug)
            printf("Activate command at time %lld, with group:%X, bank:%X, row:%X\n", time, group, bank, row_or_col);
    }
    else if((strcmp(instruction, "RD") == 0) || (strcmp(instruction, "WR") == 0)) // read or write (we don't care)
    {
        // load in 3 args, last is col
        fscanf(file, "%X %X %X", &group, &bank, &row_or_col);
        com_in->valid = 1;
        com_in->time = time;
        com_in->bank = bank;
        com_in->group = group;
        com_in->col = row_or_col;
        if(strcmp(instruction, "RD") == 0)
        {
            com_in->instruction = DRAM_COM_READ;
            if(debug)
                printf("Read command at time %lld, with group:%X, bank:%X, col:%X\n", time, group, bank, row_or_col);
        }
        else
        {
            com_in->instruction = DRAM_COM_WRITE;
            if(debug)
                printf("Write command at time %lld, with group:%X, bank:%X, col:%X\n", time, group, bank, row_or_col);
        }

    }
    else// none of above
    {
        // ERROR
        com_in->valid = 0;
        // Have main print out some kind of bug
        printf("ERROR! Invalid command at time: %lld\n", time);
        return ERROR_INSTRUCTION;
    }
    // Load everything into struct
    return 0;
}

// Writes a DRAM command to a file
char output_write(FILE * out_file, struct dram_command send)
{
    unsigned char group = send.group;
    unsigned char bank = send.bank;
    unsigned short row = send.row;
    unsigned short col = send.col;
	unsigned char command = send.instruction;
	unsigned long time = send.time;

	switch (command)
	{
	case DRAM_COM_ACT:
		fprintf(out_file, "%9ld ACT %3X %3X %6X\n", time, group, bank, row);
		break;
	case DRAM_COM_PRE:
		fprintf(out_file, "%9ld PRE %3X %3X\n", time, group, bank);
		break;
	case DRAM_COM_READ:
		fprintf(out_file, "%9ld RD %3X %3X %10X\n", time, group, bank, col);
		break;
	case DRAM_COM_WRITE:
		fprintf(out_file, "%9ld WR %3X %3X %10X\n", time, group, bank, col);
		break;
	case DRAM_COM_REFRESH:
		fprintf(out_file, "%9ld REF\n", time);
		break;
	default:
		// Error: invalid command
		return -1;
	}

    return 0;
}

// Increments all times since last command
int increment_time(int add_time, struct command_time * time_block)
{
    if((add_time < 0) || (time_block == NULL))
        return -1;
    
    time_block->activate += add_time;
    time_block->read += add_time;
    time_block->write += add_time;
    time_block->refresh += add_time;
    time_block->precharge += add_time;
    
    return 0;
}

// Resets all times to zero
int reset_time(struct command_time * time_block)
{
    if(time_block == NULL)
        return -1;
    
    time_block->activate = 0;
    time_block->read = 0;
    time_block->write = 0;
    time_block->refresh = 0;
    time_block->precharge = 0;
    
    return 0;
}

// Function that does timing checks on a refresh command and reports errors
int time_check_ref(struct bank_group * group, struct dram_command * current_com, FILE * out_file)
{
    int error_count = 0;
    // Refresh command:
    //  -Check tRFC: REF to REF (any bank)
    if( group[0].time.refresh < tRFC)
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: REFRESH TIME (tRFC) VIOLATED\n");
        fprintf(out_file, "         ERROR: REFRESH TIME (tRFC) VIOLATED\n");
        error_count++;
        fprintf(out_file, "         ERROR: TIME SINCE LAST REFRESH: %d\n", group[current_com->group].bank[current_com->bank].time.refresh);
    }
    // Set banks, bgs to REF:
    for(int g=0;g< NUM_GROUPS;g++)
    {
        group[g].command = DRAM_COM_REFRESH;
        group[g].time.refresh = 0;
        for(int b=0;b < NUM_BANKS;b++)
        {
            group[g].bank[b].status = BANK_REFRESH;
            group[g].bank[b].command = DRAM_COM_REFRESH;
            group[g].bank[b].time.refresh = 0;
        }
    }
    return error_count;
}

// Function that does timing checks on a precharge command and reports errors
int time_check_pre(struct bank_group * group, struct dram_command * current_com, FILE * out_file)
{
    int error_count = 0;
    // Precharge command:
    //  -Check that no current REF (tRFC) (any bank)
    if( group[0].time.refresh < tRFC)
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: REFRESH TIME (tRFC) VIOLATED\n");
        fprintf(out_file, "         ERROR: REFRESH TIME (tRFC) VIOLATED\n");
        fprintf(out_file, "         ERROR: TIME SINCE LAST REFRESH: %d\n", group[current_com->group].bank[current_com->bank].time.refresh);
        error_count++;
    }
    //  -Check tRAS: ACT to PRE (same bank)
    if( group[current_com->group].bank[current_com->bank].time.activate < (2*tRAS))
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: ACT to PRE WAIT (tRAS) VIOLATED\n");
        fprintf(out_file, "         ERROR: ACT to PRE WAIT (tRAS) VIOLATED\n");
        fprintf(out_file, "         ERROR: TIME SINCE LAST ACTIVATE: %d\n", group[current_com->group].bank[current_com->bank].time.activate);
        error_count++;
    }
    //  -Check tWR: WRITE to PRE (same bank)
    if( group[current_com->group].bank[current_com->bank].time.write < (2*tWR))
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: WRITE to PRE WAIT (tWR) VIOLATED\n");
        fprintf(out_file, "         ERROR: WRITE to PRE WAIT (tWR) VIOLATED\n");
        fprintf(out_file, "         ERROR: TIME SINCE LAST WRITE: %d\n", group[current_com->group].bank[current_com->bank].time.write);
        error_count++;
    }
    //  -Check tRTP: RD to PRE (same bank)
    if( group[current_com->group].bank[current_com->bank].time.read < (2*tRTP))
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: READ to PRE WAIT (tRTP) VIOLATED\n");
        fprintf(out_file, "         ERROR: READ to PRE WAIT (tRTP) VIOLATED\n");
        fprintf(out_file, "         ERROR: TIME SINCE LAST READ: %d\n", group[current_com->group].bank[current_com->bank].time.read);
        error_count++;
    }
    // Set bank group, bank to PRE
    group[current_com->group].command = DRAM_COM_PRE;
    group[current_com->group].time.precharge = 0;
    
    group[current_com->group].bank[current_com->bank].status = BANK_PRECHARGE;
    group[current_com->group].bank[current_com->bank].command = DRAM_COM_PRE;
    group[current_com->group].bank[current_com->bank].time.precharge = 0;
    
    return error_count;
}

// Function that does timing checks on an activate command and reports errors
int time_check_act(struct bank_group * group, struct dram_command * current_com, FILE * out_file)
{
    int error_count = 0;
    // Activate command:
    //  -Check that no current REF (tRFC) (any bank)
    if( group[0].time.refresh < tRFC)
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: REFRESH TIME (tRFC) VIOLATED\n");
        fprintf(out_file, "         ERROR: REFRESH TIME (tRFC) VIOLATED\n");
        fprintf(out_file, "         ERROR: TIME SINCE LAST REFRESH: %d\n", group[current_com->group].bank[current_com->bank].time.refresh);
        error_count++;
    }
    //  -Check THAT BANK IS PRECHARGED
    if( group[current_com->group].bank[current_com->bank].status != BANK_PRECHARGE)
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: ACTIVATE WITHOUT PRECHARGE!\n");
        fprintf(out_file, "         ERROR: ACTIVATE WITHOUT PRECHARGE!\n");
        error_count++;
    }
    //  -Check tRP: PRE to ACT (same bank)
    if( group[current_com->group].bank[current_com->bank].time.precharge < (2*tRP))
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: PRE to ACT TIME (tRC) VIOLATED\n");
        fprintf(out_file, "         ERROR: PRE to ACT TIME (tRC) VIOLATED\n");
        fprintf(out_file, "                TIME SINCE LAST PRECHARGE: %d\n", group[current_com->group].bank[current_com->bank].time.precharge);
        error_count++;
    }
    //  -Check tRRD_L: ACT to ACT (same bg)
    if(group[current_com->group].time.activate < (2*tRRD_L))
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: ACT to ACT TIME (tRRD_L) VIOLATED\n");
        fprintf(out_file, "         ERROR: ACT to ACT TIME (tRRD_L) VIOLATED\n");
        fprintf(out_file, "                TIME SINCE LAST ACTIVATE: %d\n", group[current_com->group].bank[current_com->bank].time.activate);
        error_count++;
    }
    //  -Check tRRD_S: ACT to ACT (diff bg)
    for(int g=0;g< NUM_GROUPS;g++)
    {
        if((g != current_com->group) && (group[current_com->group].time.activate < (2*tRRD_L)))
        {
            if(!FILE_DEBUG)
                output_write(out_file, *current_com);
            if(CONSOLE_DEBUG)
                printf("         ERROR: ACT to ACT TIME (tRRD_S) VIOLATED\n");
            fprintf(out_file, "         ERROR: ACT to ACT TIME (tRRD_S) VIOLATED\n");
            fprintf(out_file, "                TIME SINCE LAST ACTIVATE: %d\n", group[current_com->group].bank[current_com->bank].time.activate);
            error_count++;
        }
    }
    // Set bank group, bank to ACT
    group[current_com->group].command = DRAM_COM_ACT;
    group[current_com->group].time.activate = 0;
    
    group[current_com->group].bank[current_com->bank].status = BANK_ACTIVE;
    group[current_com->group].bank[current_com->bank].command = DRAM_COM_ACT;
    group[current_com->group].bank[current_com->bank].time.activate = 0;
    group[current_com->group].bank[current_com->bank].row = current_com->row;
    
    return error_count;
}

// Function that does timing checks on a read command and reports errors
int time_check_read(struct bank_group * group, struct dram_command * current_com, FILE * out_file)
{
    int error_count = 0;
    
    // Read command:
    //  -Check that no current REF (tRFC) (any bank)
    if( group[0].time.refresh < tRFC)
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: REFRESH TIME (tRFC) VIOLATED\n");
        fprintf(out_file, "         ERROR: REFRESH TIME (tRFC) VIOLATED\n");
        fprintf(out_file, "         ERROR: TIME SINCE LAST REFRESH: %d\n", group[current_com->group].bank[current_com->bank].time.refresh);
        error_count++;
    }
    //  -Check THAT BANK IS ACTIVATED TO ROW
    if(group[current_com->group].bank[current_com->bank].status != BANK_ACTIVE)
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: ROW NOT ACTIVATED!\n");
        fprintf(out_file, "         ERROR: ROW NOT ACTIVATED!\n");
        //fprintf(out_file, "         ROW VALUE %X\n", group[current_com->group].bank[current_com->bank].row);
        error_count++;
    }
    //  -Check tRCD: ACT to RD (same bank)
    if( group[current_com->group].bank[current_com->bank].time.activate < (2*tRCD))
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: ACT to READ TIME (tRCD) VIOLATED\n");
        fprintf(out_file, "         ERROR: ACT to READ TIME (tRCD) VIOLATED\n");
        fprintf(out_file, "                TIME SINCE LAST ACTIVATE: %d\n", group[current_com->group].bank[current_com->bank].time.activate);
        error_count++;
    }
    //  -Check tCCD_L: RD to RD (same bg)
    if(group[current_com->group].time.read < (2*tCCD_L))
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: READ to READ TIME (tCCD_L) VIOLATED\n");
        fprintf(out_file, "         ERROR: READ to READ TIME (tCCD_L) VIOLATED\n");
        fprintf(out_file, "                TIME SINCE LAST READ: %d\n", group[current_com->group].bank[current_com->bank].time.read);
        error_count++;
    }
    //  -Check tCCD_S: RD to RD (diff bg)
    for(int g=0;g< NUM_GROUPS;g++)
    {
        if((g != current_com->group) && (group[current_com->group].time.read < (2*tCCD_S)))
        {
            if(!FILE_DEBUG)
                output_write(out_file, *current_com);
            if(CONSOLE_DEBUG)
                printf("         ERROR: READ to READ TIME (tCCD_S) VIOLATED\n");
            fprintf(out_file, "         ERROR: READ to READ TIME (tCCD_S) VIOLATED\n");
            fprintf(out_file, "                TIME SINCE LAST READ: %d\n", group[current_com->group].bank[current_com->bank].time.read);
            error_count++;
        }
    }
    //  -Check tWTR_L: WRITE to RD (same bg)
    if(group[current_com->group].time.write < (2*tWTR_L))
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: WRITE to READ TIME (tCCD_L) VIOLATED\n");
        fprintf(out_file, "         ERROR: WRITE to READ TIME (tCCD_L) VIOLATED\n");
        fprintf(out_file, "                TIME SINCE LAST WRITE: %d\n", group[current_com->group].bank[current_com->bank].time.write);
        error_count++;
    }
    //  -Check tWTR_S: WRITE to RD (diff bg)
    for(int g=0;g< NUM_GROUPS;g++)
    {
        if((g != current_com->group) && (group[current_com->group].time.write < (2*tWTR_S)))
        {
            if(!FILE_DEBUG)
                output_write(out_file, *current_com);
            if(CONSOLE_DEBUG)
                printf("         ERROR: WRITE to READ TIME (tWTR_S) VIOLATED\n");
            fprintf(out_file, "         ERROR: WRITE to READ TIME (tWTR_S) VIOLATED\n");
            fprintf(out_file, "                TIME SINCE LAST WRITE: %d\n", group[current_com->group].bank[current_com->bank].time.write);
        }   
    }
    // Set bank group, bank to RD
    group[current_com->group].command = DRAM_COM_READ;
    group[current_com->group].time.read = 0;
    
    group[current_com->group].bank[current_com->bank].command = DRAM_COM_READ;
    group[current_com->group].bank[current_com->bank].time.read = 0;
    
    return error_count;
}

// Function that does timing checks on a write command and reports errors
int time_check_write(struct bank_group * group, struct dram_command * current_com, FILE * out_file)
{
    int error_count = 0;
    // Check timing on a WRITE command
    //  -Check that no current REF (tRFC) (any bank)
    if( group[0].time.refresh < tRFC)
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: REFRESH TIME (tRFC) VIOLATED\n");
        fprintf(out_file, "         ERROR: REFRESH TIME (tRFC) VIOLATED\n");
        fprintf(out_file, "         ERROR: TIME SINCE LAST REFRESH: %d\n", group[current_com->group].bank[current_com->bank].time.refresh);
        error_count++;
    }
    //  -Check THAT BANK IS ACTIVATED TO ROW
    if(group[current_com->group].bank[current_com->bank].status != BANK_ACTIVE)
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: ROW NOT ACTIVATED!\n");
        fprintf(out_file, "         ERROR: ROW NOT ACTIVATED!\n");
        //fprintf(out_file, "         ROW VALUE %X\n", group[current_com->group].bank[current_com->bank].row);
        error_count++;
    }
    //  -Check tRCD: ACT to WRITE (same bank)
    if( group[current_com->group].bank[current_com->bank].time.activate < (2*tRCD))
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: ACT to WRITE TIME (tRCD) VIOLATED\n");
        fprintf(out_file, "         ERROR: ACT to WRITE TIME (tRCD) VIOLATED\n");
        fprintf(out_file, "                TIME SINCE LAST ACTIVATE: %d\n", group[current_com->group].bank[current_com->bank].time.activate);
        error_count++;
    }
    //  -Check tCCD_L: WRITE/RD to WRITE (same bg)
    if((group[current_com->group].time.write < tCCD_L) || (group[current_com->group].time.read < (2*tCCD_L)))
    {
        if(!FILE_DEBUG)
            output_write(out_file, *current_com);
        if(CONSOLE_DEBUG)
            printf("         ERROR: WRITE/READ to WRITE TIME (tCCD_L) VIOLATED\n");
        fprintf(out_file, "         ERROR: WRITE/READ to WRITE TIME (tCCD_L) VIOLATED\n");
        fprintf(out_file, "                TIME SINCE LAST READ: %d\n", group[current_com->group].bank[current_com->bank].time.read);
        fprintf(out_file, "                TIME SINCE LAST WRITE: %d\n", group[current_com->group].bank[current_com->bank].time.write);
        error_count++;
    }
    //  -Check tCCD_S: WRITE/RD to WRITE (diff bg)
    for(int g=0;g< NUM_GROUPS;g++)
    {
        if((g != current_com->group) && ((group[current_com->group].time.read < (2*tCCD_S)) || (group[current_com->group].time.write < (2*tCCD_S))))
        {
            if(!FILE_DEBUG)
                output_write(out_file, *current_com);
            if(CONSOLE_DEBUG)
                printf("         ERROR: WRITE/READ to WRITE TIME (tCCD_S) VIOLATED\n");
            fprintf(out_file, "         ERROR: WRITE/READ to WRITE TIME (tCCD_S) VIOLATED\n");
            fprintf(out_file, "                TIME SINCE LAST READ: %d\n", group[current_com->group].bank[current_com->bank].time.read);
            fprintf(out_file, "                TIME SINCE LAST WRITE: %d\n", group[current_com->group].bank[current_com->bank].time.write);
            error_count++;
        }
    }
    // Set bank group, bank to WRITE
    group[current_com->group].command = DRAM_COM_WRITE;
    group[current_com->group].time.write = 0;
    
    group[current_com->group].bank[current_com->bank].command = DRAM_COM_WRITE;
    group[current_com->group].bank[current_com->bank].time.write = 0;
    
    return error_count;
}