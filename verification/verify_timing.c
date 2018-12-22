// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-13

// verify_timing.c

/*
 Code to verify memory controller output meets timing requirements
*/

// Contains utility functions for verification code
#include "verify_util.h" 

// UTILITY FUNCTIONS IN VERIFY_UTIL
/*
int read_output(FILE * file, struct dram_command * com_in);
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
*/

// MAIN FUNCTION 
int main(int argc, char * argv[])
{
	static FILE * in_file;				    // input file
	static FILE * out_file;					// ouput file

	struct dram_command current_com;	    // Working command
	struct dram_command last_com;           // Last command
    long long time_diff;                    // Difference in time from last command
    
    struct bank_group group[NUM_GROUPS];    // Bank Groups (to store statuses)
    
    // Set initial conditions of both held commands
    current_com.valid = 0;
    current_com.time = 0;
    last_com.valid = 0;
    last_com.time = 0;
    
    // Set error variable
    int error_count = 0;
    
    // Set command counts
    int ref_count = 0;
    int pre_count = 0;
    int act_count = 0;
    int read_count = 0;
    int write_count = 0;
    
    // Status variables
	int f_status = 0;				// 1 means error in input file (possible EOF)

    // Initialize input/output files
    char * filename;
    char * fileoutname;
	/* check the input parameters */
	if (argc > 1)
        filename = argv[1]; 
    else
        filename = "output.txt";
    if(argc > 2)
        fileoutname = argv[2];
    else
        fileoutname = "verify_log.txt";
    
    // Open input/output files
    in_file = open_file(filename);
    out_file = open_fileout(fileoutname);
    if (in_file <= 0 || out_file <= 0)
	{
		printf("Unable to open file.\n");
		return -1;
	}
    
    // Initialize banks and bank groups
    //      All banks come precharged
    //      Reset timing variables to neutral number
    for(int g=0;g< NUM_GROUPS;g++)
    {
        group[g].command = DRAM_COM_PRE;
        reset_time(&(group[g].time));
        increment_time(MAX_TIME, &(group[g].time));
        for(int b=0;b < NUM_BANKS;b++)
        {
            group[g].bank[b].status = BANK_PRECHARGE;
            group[g].bank[b].command = DRAM_COM_PRE;
            reset_time(&(group[g].bank[b].time));
            increment_time(MAX_TIME, &(group[g].bank[b].time));
        }
    }
    
    // Read through file and check timing
    while(f_status == 0) // While file is valid
    {
        last_com = current_com; // Save the last command for reference
        
        f_status = read_output(in_file, &current_com); // Read output to command struct
        if(FILE_DEBUG)
            output_write(out_file, current_com);
        
        // If a valid command, check timing
        if(f_status == 0)
        {
            // If time has passed, increment clock and timings for all groups
            if(last_com.valid != 0)
            {
                time_diff = current_com.time - last_com.time;
                if(time_diff < 0)
                    time_diff = (~0 - last_com.time) + current_com.time;
                for(int g=0;g< NUM_GROUPS;g++)
                {
                    increment_time(time_diff, &(group[g].time));
                    for(int b=0;b < NUM_BANKS;b++)
                    {
                        increment_time(time_diff, &(group[g].bank[b].time));
                    }
                }
            }
            
            // Check timing depending on command
            switch(current_com.instruction)
            {
                case DRAM_COM_REFRESH:
                    // Check timing on a REFRESH command
                    error_count+= time_check_ref(group, &current_com, out_file);
                    
                    ref_count++; // Increment command counter
                    break;
                    
                case DRAM_COM_PRE:
                    // Check timing on a PRECHARGE command
                    error_count+= time_check_pre(group, &current_com, out_file);
                    
                    pre_count++; // Increment command counter
                    break;
                    
                case DRAM_COM_ACT:
                    // Check timing on an ACTIVATE command
                    error_count+= time_check_act(group, &current_com, out_file);

                    act_count++; // Increment command counter
                    break;
                    
                case DRAM_COM_READ:
                    // Check timing on a READ command
                    error_count+= time_check_read(group, &current_com, out_file);
                    
                    read_count++; // Increment command counter
                    break;
                    
                case DRAM_COM_WRITE:
                    // Check timing on a WRITE command
                    error_count += time_check_write(group, &current_com, out_file);
                    
                    write_count++; // Increment command counter
                    break;    
            } // End of switch statement
            
        } // If statement: if a valid command

    } // End of while loop (while file is not empty)
    
    // Calculate summary data
    int total_count = ref_count + pre_count + act_count + read_count + write_count;
    int access_count = read_count + write_count;
    float access_to_total = ((float)access_count)/((float)total_count);
    float access_to_act = ((float)access_count)/((float)act_count);
   
    // Print out final error information
    printf("\n\n***********\n");
    printf("ERROR REPORT: %d timing errors found in input file!\n\n", error_count);
    
    printf("Total commands: %d\n", total_count);
    printf("Total READ/WRITE commands: %d\n\n", access_count);
    
    printf("Ratio of READ/WRITE commands to total commands: %f\n", access_to_total);
    printf("Ratio of READ/WRITE commands to ACTIVATE commands: %f\n\n\n", access_to_act);
    
    // Close input and output files
    close_file(in_file);
    close_file(out_file);
return 0;
}

