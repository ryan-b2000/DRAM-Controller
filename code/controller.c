// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-08
// controller.c

/*	This file handles the code for the algorithmic functions of the controller
*/

#include "controller.h"

/* PRIVATE DATA MEMBERS */
// this is the instance of our memory controller
struct mem_control controller;		


// this is the list of commands that could be sent on any given clock cycle
// list is generated each clock cycle and then used to determine which command to send
// list is a linear linked list
struct dram_command command_list[QUEUE_LEN];


/* FUNCTION DEFINTIONS */

// initialize the memory controller. Set the clocks and group and bank statuses
void init_controller(void)
{
	unsigned char g, b;

	// initialize the clock
	controller.clock.count = 0;
	controller.clock.old = 0;
	controller.clock.mult = CPU_CLOCK_SPEED / DIMM_CLOCK_SPEED;

	// initialize stats
	controller.stats.lowtime = 0;
	controller.stats.hightime = 5555;	// something artificially high to get it going

	// initialize bank statuses
	for (g = 0; g < NUM_GROUPS; ++g)
	{
		controller.group[g].time.activate = (tRRD_L * controller.clock.mult) + 1;			// assume enough time has passed since last ACT so first one isn't hung up
		controller.group[g].time.read = 0;
		controller.group[g].time.refresh = 0;
		controller.group[g].time.write = 0;
		controller.group[g].time.precharge = 0;	

		for (b = 0; b < NUM_BANKS_PER_GROUP; ++b)
		{
			controller.group[g].bank[b].status = BANK_IDLE;
			controller.group[g].bank[b].time.activate = 0;
			controller.group[g].bank[b].time.read = 0;
			controller.group[g].bank[b].time.refresh = 0;
			controller.group[g].bank[b].time.read = 0;
			controller.group[g].bank[b].time.write = 0;
			controller.group[g].bank[b].time.precharge = (tRP * controller.clock.mult) + 1;	// assume all banks start off as precharged
		}
	}
}


// Increment the master clock, bank clocks, and queue clocks
void inc_clock(struct mem_req * current)
{
	unsigned long long count = controller.clock.count;
	
	// if the queue is empty, advance the clock to the next memory request
	// otherwise increment the clock
	if (get_queue_status() == QUEUE_EMPTY)
	{
		count = current->request.time;
		d_print_clock_jump(count);
	}
	else
	{
		controller.clock.old = count;
		++count;
	}

	// increment all the bank and group clocks
	inc_clock_bankgroup();

	// increment the clocks for all items in the queue
	inc_q_clocks();

	// check for overflow
	if (count < controller.clock.old)
	{
		++controller.clock.overflow;
		controller.clock.count = 0;
	}
	
	// reassign the new count value
	controller.clock.count = count;

	// debug print
	d_print_clock(controller.clock.count);
}


// this function increments all the relative clocks for the banks and groups
void inc_clock_bankgroup(void)
{
	unsigned char g, b;
	
	// go through all the banks and update their relative clocks
	for (g = 0; g < NUM_GROUPS; ++g)
	{
		// increment relative bank group clocks for each command
		++controller.group[g].time.activate;
		++controller.group[g].time.read;
		++controller.group[g].time.write;
		++controller.group[g].time.precharge;
		++controller.group[g].time.refresh;

		// increment relative clocks for each bank
		for (b = 0; b < NUM_BANKS_PER_GROUP; ++b)
		{
			++controller.group[g].bank[b].time.activate;
			++controller.group[g].bank[b].time.read;
			++controller.group[g].bank[b].time.write;
			++controller.group[g].bank[b].time.precharge;
			++controller.group[g].bank[b].time.refresh;
		}
	}
}


// Return the master clock
unsigned long long get_clock(void)
{
	return controller.clock.count;
}


// checks if any banks are done precharging and moves their state to the closed state (ready for next access)
void close_bank_status(void)
{
	unsigned char cmult = controller.clock.mult;
	unsigned char g, b;
	unsigned char status;
	unsigned int time;

	// go through all the banks and update their relative clocks
	for (g = 0; g < NUM_GROUPS; ++g)
	{
		for (b = 0; b < NUM_BANKS_PER_GROUP; ++b)
		{
			// if the bank is in the precharge state, check to see if it is done precharging
			status = controller.group[g].bank[b].status;
			time = controller.group[g].bank[b].time.precharge;
			if (status == BANK_PRECHARGE && time >= (tRP * cmult))
			{
				controller.group[g].bank[b].status = BANK_IDLE;
			}		
		}
	}
	return;
}


// We need to get the info of the bank that the memory request wants to access so we can
// figure out if we are able to send a command or not
// Return bank with the information from the memory controller
void get_bank_info(struct mem_req * curr, struct bank_info * bank)
{
	unsigned char b = curr->bank;
	unsigned char g = curr->group;

	bank->command = controller.group[g].bank[b].command;
	bank->status = controller.group[g].bank[b].status;
	bank->time = controller.group[g].bank[b].time;
	bank->row = controller.group[g].bank[b].row;
}


// On every clock tick, go through the queued list of memory requests and determine any commands that can
// be sent on the next clock tick
char command_scheduler(struct dram_command * comsend)
{
	char c_status = DO_NOT_SEND;	// command status: is there a new command to be issued?
	struct mem_req * curr;			// the request currently at the head of the queue
	struct mem_req * end;
	struct bank_info bank;			// temporary storage of the bank information


	// if the queue is empty, exit
	if (get_queue_status() == QUEUE_EMPTY)
		return DO_NOT_SEND;

	// get the queue head and tail pointers for traversing the circular list of memory requests to send
	get_queue_headptr(&curr);
	get_queue_end(&end);	// get the end of the queue

	// PROCESS NEXT COMMAND
	// go through each memory request and determine what the next command is
	do
	{
		// get the bank and group information
		get_bank_info(curr, &bank);

		// look at the last command sent for this request and compare it to the last command sent to the desired bank/group
		switch (curr->command.status)
		{
		// no command has been issued, we want to issue an ACTIVATE command
		case DRAM_COM_NONE:
			c_status = compute_command_none(&curr, comsend, bank);
			break;
			
		// ACTIVATE command has been issued for this memory request, we want to issue the READ/WRITE
		case DRAM_COM_ACT:
			c_status = compute_command_activate(&curr, comsend, bank);
			break;

		// if last command was READ/WRITE, the memory request can be removed from the queue
		// remove the request after the precharge has been issued.
		case DRAM_COM_READ:
		case DRAM_COM_WRITE:
			c_status = compute_command_readwrite(&curr, comsend, bank);
			break;
		}

		// go to the next request in the queue
		curr = curr->next;
	
	// keep looking until we find a command to send or reached the end of the queue
	} while (curr != end && c_status == DO_NOT_SEND);

	return c_status;
}


// If the memory request has not sent a command yet, this function will determine if it is possible to send a command or not
char compute_command_none(struct mem_req ** curr, struct dram_command * comsend, struct bank_info bank)
{
	int cmult = controller.clock.mult;		// CPU-DRAM clock multiplier

	// if the bank is closed, then it is definitely done precharging and we can issue a new ACTIVATE
	if (bank.status == BANK_IDLE)
	{
		// the bank is not done precharging, come back and try again later
		if (bank.time.precharge < (tRP * cmult))
			return DO_NOT_SEND;

		// make sure we wait the read-cyle time before issuing the next ACT
		if (bank.time.activate < (tRC * cmult))
			return DO_NOT_SEND;

		// this bank is closed, we want to issue an activate, check against last activate for the group
		if (check_tRRD((*curr)) == DO_NOT_SEND)
			return DO_NOT_SEND;

		update_request_command((*curr), DRAM_COM_ACT);		// update memory request status and time
		update_bank_status((*curr), DRAM_COM_ACT);			// update the bank and group statuses
		set_command_info(comsend, (*curr));				// build the outgoing command
		if (bank.command == DRAM_COM_PRE)
			d_update_stats(STAT_PAGE_MISS, 0);
		return OK_TO_SEND;
	}
	else if (bank.status == BANK_ACTIVE)
	{
		// if the bank is open to a different row, some other request issued an ACTIVATE
		// Rows don't match so that request will close the row when it is done
		if (bank.row != (*curr)->row)
			return DO_NOT_SEND;

		// if last command to the bank was ACTIVATE and rows match
		// see if we can skip ahead and do a READ/WRITE for this command
		if (bank.command == DRAM_COM_ACT)
		{
			// the desired bank already has an activate issued, some other request is also using the bank
			// check that you meet Row to Column Delay between ACT and READ/WRITE		
			if (bank.time.activate < (tRCD * cmult))
				return DO_NOT_SEND;

			// if you want to do a READ, you have to check CCD
			if (check_tCCD((*curr)) == DO_NOT_SEND)
				return DO_NOT_SEND;

			// Page Hit: issue the READ or WRITE
			if ((*curr)->request.type == I_READ || (*curr)->request.type == I_FETCH)
			{
				// if you want to do a READ, you have to check WTR
				if (check_tWTR((*curr)) == DO_NOT_SEND)
					return DO_NOT_SEND;
				
				update_request_command((*curr), DRAM_COM_READ);		// update memory request status and time
				update_bank_status((*curr), DRAM_COM_READ);		// update the bank and group statuses
				set_command_info(comsend, (*curr));				// build the outgoing command
				d_update_stats(STAT_PAGE_HIT, 0);
				return OK_TO_SEND;
			}
			else
			{
				if (check_read_write_conflict() == DO_NOT_SEND)
					return DO_NOT_SEND;

				update_request_command((*curr), DRAM_COM_WRITE);		// update memory request status and time
				update_bank_status((*curr), DRAM_COM_WRITE);			// update the bank and group statuses
				set_command_info(comsend, (*curr));					// build the outgoing command
				d_update_stats(STAT_PAGE_HIT, 0);
				return OK_TO_SEND;
			}
		}
		// if the last command issued was a READ command we could do a READ command
		// if it is the same row as the one we want to read (by default this should be the case)
		else if (bank.command == DRAM_COM_READ || bank.command == DRAM_COM_WRITE)
		{
			// if we want to do a READ-READ or WRITE-WRITE, we need to check the time since the last R/W to that group
			if (check_tCCD(*curr) == DO_NOT_SEND)
				return DO_NOT_SEND;

			// if doing a read, we can schedule the READ operation
			if ((*curr)->request.type == I_READ || (*curr)->request.type == I_FETCH)
			{
				// if you want to do a READ, you have to check WTR
				if (check_tWTR((*curr)) == DO_NOT_SEND)
					return DO_NOT_SEND;

				update_request_command((*curr), DRAM_COM_READ);		// update memory request status and time
				update_bank_status((*curr), DRAM_COM_READ);		// update the bank and group statuses
				set_command_info(comsend, (*curr));				// build the outgoing command
				d_update_stats(STAT_PAGE_HIT, 0);
				return OK_TO_SEND;
			}
			else
			{
				update_request_command((*curr), DRAM_COM_WRITE);		// update memory request status and time
				update_bank_status((*curr), DRAM_COM_WRITE);			// update the bank and group statuses
				set_command_info(comsend, (*curr));					// build the outgoing command
				d_update_stats(STAT_PAGE_HIT, 0);
				return OK_TO_SEND;
			}
		}
	}
	return DO_NOT_SEND;
}


// If the memory request has sent an ACTIVATE command, this function will determine if it is possible to send a command or not
char compute_command_activate(struct mem_req ** curr, struct dram_command * comsend, struct bank_info bank)
{
	int cmult = controller.clock.mult;	// CPU-DRAM clock multiplier

	// if the bank is open to a different row, then we need to wait for that request to close the bank
	if (bank.row != (*curr)->row)
		return DO_NOT_SEND;

	// we want to READ/WRITE the (*curr)ently active row, we might be able to issue the command now
	// if the last bank command was ACTIVATE and it is ready to recieve READ/WRITE command
	if (bank.time.activate < (tRCD * cmult))
		return DO_NOT_SEND;

	// if we want to do a READ-READ or WRITE-WRITE, we need to check the time since the last R/W to that group
	if (check_tCCD(*curr) == DO_NOT_SEND)
		return DO_NOT_SEND;

	// if we want to do a READ, we have to check against time since last WRITE
	if ((*curr)->request.type == I_FETCH || (*curr)->request.type == I_READ)
		if (check_tWTR((*curr)) == DO_NOT_SEND)
			return DO_NOT_SEND;


	// check all the group commands to see when the last read or write was issued and wait tBurst to do the next READ/WRITE
	if (check_read_write_conflict() == DO_NOT_SEND)
		return DO_NOT_SEND;

	// we want to do a READ and the last bank command was a READ to a different row
	if (bank.command == DRAM_COM_READ && bank.row != (*curr)->row)
		return DO_NOT_SEND;

	// for now, do nothing
	// we want to do a read to the same bank but a READ has already been issued to a different row
	// bank needs to be closed and precharged
	// the row will get closed when the command that issued the read completes
	// this one can have the bank

	if ((*curr)->request.type == I_READ || (*curr)->request.type == I_FETCH)
	{
		update_request_command((*curr), DRAM_COM_READ);		// update memory request status and time
		update_bank_status((*curr), DRAM_COM_READ);		// update the bank and group statuses
		set_command_info(comsend, (*curr));				// build the outgoing command
		return OK_TO_SEND;
	}
	else
	{
		update_request_command((*curr), DRAM_COM_WRITE);		// update memory request status and time
		update_bank_status((*curr), DRAM_COM_WRITE);			// update the bank and group statuses
		set_command_info(comsend, (*curr));					// build the outgoing command
		return OK_TO_SEND;
	}
	return DO_NOT_SEND;
}


// If the memory request has sent a READ/WRITE, then all that is left is to either send a PRECHARGE or remove the request from the queue
char compute_command_readwrite(struct mem_req ** curr, struct dram_command * comsend, struct bank_info bank)
{
	int cmult = controller.clock.mult;	// CPU-DRAM clock multiplier
	
	// check that no other requests need the row
	if (check_request_row_addr((*curr)) == LEAVE_ROW_OPEN)
	{
		// some other request wants the row so remove the request and move on
		d_update_stats(STAT_TIME, (*curr)->queue_time);
		dequeue_request_select((*curr));
		return DO_NOT_SEND;
	}

	// if you want to issue a PRE command, have to make sure we meet Read To Precharge Delay
	if (bank.time.read < (tRTP * cmult))
		return DO_NOT_SEND;

	// we want to issue a PRE command, check the time since ACTIVATE has been met
	if (bank.time.activate < (tRAS * cmult))
		return DO_NOT_SEND;

	// per the instructions and clarification from Mark there is no hidden precharge
	// so you have to wait until the data is finished transfering before issuing precharge
	//if ((bank.time.read < tCL + tBURST) || (bank.time.write < tCWL + tBURST))
	if ((*curr)->command.status == DRAM_COM_READ && (((*curr)->command.time) < (tCL + tBURST) * cmult))
		return DO_NOT_SEND;

	if ((*curr)->command.status == DRAM_COM_WRITE && (((*curr)->command.time) < (tCWL + tBURST) * cmult))
		return DO_NOT_SEND;

	// issue the PRE command
	update_request_command((*curr), DRAM_COM_PRE);			// update memory request status and time
	update_bank_status((*curr), DRAM_COM_PRE);				// update the bank and group statuses
	set_command_info(comsend, (*curr));					// build the outgoing command

	// command information is passed to comsend. Once the PRE command has been issued, we can remove the memory request
	d_update_stats(STAT_TIME, (*curr)->queue_time);
	dequeue_request_select((*curr));
	return OK_TO_SEND;
}


// Copy the information from the CPU request in the queue to the outgoing DRAM command
void set_command_info(struct dram_command * command, struct mem_req * current)
{
	command->status = current->command.status;
	// Need to account for the CPU/DRAM clock disparity by using the relative time in the queue x multiplier.
	// For the first instruction, if we send it the same time as reading it, we should use the DRAM time since it will
	// be the same as the CPU (request recevied) time. After that, we need to account for the relative time difference
	command->time = get_clock();
	
	command->group = current->group;
	command->bank = current->bank;
	command->row = current->row;
	command->col = current->col;

	command->address = current->request.address;
}


// Update the memory request command status and reset the relative time
// We use the command status to track what has been done for the memory request
void update_request_command(struct mem_req * request, char command)
{
	request->command.status = command;		// update the command
	request->command.time = 0;				// reset the relative timer for the command
}


// any time you want to do a READ you have to check the last time a WRITE was issued
// this is an additive latency issue where the DRAM might reorganize the commands but
// since we aren't doing additive latency, we must adhere to these timing parameters
char check_tWTR(struct mem_req * current)
{
	unsigned char g;
	unsigned char cmult = controller.clock.mult;
	unsigned char group = current->group;
	unsigned long long time;

	// check ACTIVATE time of your bank group
	time = controller.group[group].time.write;
	if (time < (tWTR_L * cmult))
		return DO_NOT_SEND;

	// check ACTIVATE for the other bank groups
	for (g = 0; g < NUM_GROUPS; ++g)
	{
		// the time is less than any of the other ACTIVATE times, do not send
		time = controller.group[g].time.write;
		if (group != g)
			if (time < (tWTR_S * cmult))
				return DO_NOT_SEND;
	}
	return OK_TO_SEND;
}


// if we want to issue an ACTIVATE you have to check the time of the last ACT to your bank group
// and the longest time between ACTIVATES to the other bank groups
char check_tRRD(struct mem_req * current)
{
	unsigned char g;
	unsigned char cmult = controller.clock.mult;
	unsigned char group = current->group;
	unsigned long long time;

	// check ACTIVATE time of your bank group
	time = controller.group[group].time.activate;
	if (time < (tRRD_L * cmult))
		return DO_NOT_SEND;

	// check ACTIVATE for the other bank groups
	for (g = 0; g < NUM_GROUPS; ++g)
	{
		// the time is less than any of the other ACTIVATE times, do not send
		time = controller.group[g].time.activate;
		if (group != g)
			if (time < (tRRD_S * cmult))
				return DO_NOT_SEND;
	}
	return OK_TO_SEND;
}


// if you want to send a READ/WRITE you have to check the READ/WRITE
// in other bank groups and your bank group
char check_tCCD(struct mem_req * current)
{
	unsigned char cmult = controller.clock.mult;
	unsigned char group = current->group;
	unsigned char g;
	unsigned long long time;

	// check against your bank group
	time = controller.group[group].time.read;
	if (time < (tCCD_L * cmult))
		return DO_NOT_SEND;
	time = controller.group[group].time.write;
	if (time < (tCCD_L * cmult))
		return DO_NOT_SEND;

	// check against other bank groups
	for (g = 0; g < NUM_GROUPS; ++g)
	{
		// the time is less than any of the other READs, do not send
		time = controller.group[g].time.read;
		if (group != g)
			if (time < (tCCD_S * cmult))
				return DO_NOT_SEND;
		// the time is less than any of the other READs, do not send
		time = controller.group[g].time.write;
		if (group != g)
			if (time < (tCCD_S * cmult))
				return DO_NOT_SEND;
	}
	return OK_TO_SEND;
}


// this checks the conflict between if data is coming out of the DRAM, we can't issue a WRITE request 
char check_read_write_conflict()
{
	unsigned long long time;
	unsigned char cmult = controller.clock.mult;
	unsigned char g;

	// if doing a WRITE to the already open row, have to wait for the data transfer, 
	// otherwise you'll schedule conflicts
	// we did a READ or WRITE in the past, data will be coming out in tCL or tCWL
	for (g = 0; g < NUM_GROUPS; ++g)
	{
		// check that you aren't conflicting with a READ
		time = controller.group[g].time.read;
		if ((time >= (tCL * cmult)) && (time <= ((tCL + tBURST) * cmult)))
			return DO_NOT_SEND;

		// check that you aren't conflicting with a WRITE
		time = controller.group[g].time.write;
		if ((time >= (tCWL * cmult)) && (time <= ((tCWL + tBURST) * cmult)))
			return DO_NOT_SEND;
	}
	return OK_TO_SEND;
}


// When you are issuing a new command, you need to update the bank and group status and timings
// So that the relative timings stay consistent and we can do comparisons of how long since a command
// Was issued to that bank or group.
// We keep track of the relative time for each command because we need to check all commands and not 
// just what the most recent command was
void update_bank_status(struct mem_req * curr, char command)
{
	unsigned char bank = curr->bank;
	unsigned char group = curr->group;

	// update the bank information
	controller.group[group].bank[bank].command = command;
	// reset the relative timer for whatever command was just issued
	switch (command)
	{
		case DRAM_COM_ACT:		
			// set the currently open row and reset the relative timer
			controller.group[group].bank[bank].row = curr->row;
			controller.group[group].bank[bank].time.activate = 0;		
			if (controller.group[group].bank[bank].status == BANK_IDLE)
				controller.group[group].bank[bank].status = BANK_ACTIVE;
			break;
		case DRAM_COM_READ:		
			controller.group[group].bank[bank].time.read = 0;			
			break;
		case DRAM_COM_WRITE:	
			controller.group[group].bank[bank].time.write = 0;		
			break;
		case DRAM_COM_PRE:		
			controller.group[group].bank[bank].time.precharge = 0;
			controller.group[group].bank[bank].status = BANK_PRECHARGE;
			break;
	}

	// update the group information
	controller.group[group].command = command;
	// reset the relative timer for whatever command is being issued
	switch (command)
	{
		case DRAM_COM_ACT:		
			controller.group[group].time.activate = 0;		
			break;
		case DRAM_COM_READ:		
			controller.group[group].time.read = 0;			
			break;
		case DRAM_COM_WRITE:	
			controller.group[group].time.write = 0;			
			break;
		case DRAM_COM_PRE:		
			controller.group[group].time.precharge = 0;		
			break;
	}
	
	return;
}


// prints the controller statistics information
void d_print_controller_stats()
{
#ifdef D_STATS_CONTROL
	printf("Controller performance information:\n");
	printf("Longest service time: %d\n", controller.stats.lowtime);
	printf("Fastest service time: %d\n", controller.stats.hightime);
	printf("Page hits: %d, Page misses: %d\n", controller.stats.pagehit, controller.stats.pagemiss);
#endif
}


// updates the controller statistics
void d_update_stats(char stat, unsigned int time)
{
#ifdef D_STATS_CONTROL
	if (stat == STAT_TIME)
	{
		if (time > controller.stats.lowtime)
			controller.stats.lowtime = time;

		if (time < controller.stats.hightime)
			controller.stats.hightime = time;
	}
	else
	{
		if (stat == STAT_PAGE_HIT)
			++controller.stats.pagehit;
		else
			++controller.stats.pagemiss;
	}
	
#endif
}
