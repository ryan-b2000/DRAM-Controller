// Ryan Bentz, Jamie Williams, Ashwini Nayak Kota, Jeff Gragg
// ECE 485/858
// Final Project
// 2018-06-08
// queue.c

/*
	This file contains the function definitions and private data members for the queue.
	The queue is a statically allocated array of memory requests. The array members are then setup
	in a doubly linked circular list.

	Circular list:
	- Head always points to the front of the queue.
	- If head == NULL, the queue is empty
	- Tail points to the next available spot
	- If tail->next == head, the queue is full
*/

#include "queue.h"

struct mem_req * head;				// Points to head of queue or NULL if queue is empty 
struct mem_req * tail;				// Points to the next available spot or head if queue is full 
struct mem_req queue[QUEUE_LEN];	// must be placed here so that it placed in data

// diagnostic information for tracking performance of the queueu
unsigned int enqcount = 0;
unsigned int deqcount = 0;
unsigned int removedhead = 0;
unsigned int removedother = 0;

/* Initialize the head and tail pointers and all the items in the */
void init_queue(void)
{
	// initialize the memory request queue
	head = NULL;	
	tail = queue;	// Set tail to the first empty item 
	int i;

	// Link the nodes together
	for (i = 0; i < QUEUE_LEN - 1; ++i)
		queue[i].next = &queue[i + 1];
	
	queue[QUEUE_LEN - 1].next = &queue[0];
}


/*	ADD AN ITEM TO THE QUEUE
	Return: 1 if success, 0 if queue full
*/
char enqueue_request(struct mem_req * add)
{
	/* Queue is full if tail is at last item if tail->next is head */
	if (tail->next == head)
	{
		return QUEUE_FULL;
	}
		
	// Otherwise tail points to the next open spot
	// fill in the tail with the information from the next request
	tail->group = add->group;
	tail->bank = add->bank;
	tail->col = add->col;
	tail->row = add->row;

	tail->request.address = add->request.address;
	tail->request.time = add->request.time;
	tail->request.type = add->request.type;

	tail->queue_time = 0;
	tail->command.time = 0;
	tail->command.status = DRAM_COM_NONE;

	d_enqueue_check(tail);			// debug check if item was added to the queue or not
	++enqcount;

	/* If this is the first item, set the head of the queue */
	if (head == NULL)
		head = tail;

	/* Increment tail to the next spot */
	tail = tail->next;

	return 1;
}


// Traverse the queue until you find the request to remove
// Remove the request and re-order the queue 
char dequeue_request_select(struct mem_req * remove)
{
	struct mem_req * prev;
	struct mem_req * curr;
	struct mem_req * end;

	// if list is empty do nothing
	if (!head)
		return QUEUE_EMPTY;

#ifdef D_QUEUE_DEQ_REQ_SEL
	printf("Removing %llX\n", remove->request.address);
#endif

	++deqcount;

	// check if the first item is to be removed
	if (head == remove)
	{
#ifdef D_QUEUE_DEQ_REQ_SEL
		printf("Removing at head: %llX\n", head->request.address);
#endif		
		// Remove at the head of the queue
		head = head->next;

		// increment counter for how many we removed
		++removedhead;

		// If this is the last item, set head to NULL
		if (head == tail)
		{
			head = NULL;
			return QUEUE_EMPTY;
		}

		return QUEUE_OPEN;
	}

	// set current and previous to head
	curr = head;
	prev = head;
	
	// set the end of the queue based on if it full or not
	if (tail->next == head)
		end = head;
	else
		end = tail;

	// traverse the list until you find what you want or reached the end
	do
	{
		prev = curr;
		curr = curr->next;
	} while (curr != remove && curr != end);
	
	// if we traversed the list and got to the tail

	if (curr == end)
	{
#ifdef D_QUEUE_DEQ_REQ_SEL
		printf("Remove selected address not found\n");
#endif		
		return 0;
	}
#ifdef D_QUEUE_DEQ_REQ_SEL
	printf("Found %llX\n", curr->request.address);
#endif

	// increment counter to track how many we removed
	++removedother;

	// current is the one you want to remove so set previous to current and current to first to remove
	prev = curr;
	curr = curr->next;

	// if queue has an empty spot and we are at tail, then this was removing the last item in the tail
	// cycle through the LLL and move all the nodes up to fill in the gap
	while (curr != tail)
	{
		prev->group = curr->group;
		prev->bank = curr->bank;
		prev->col = curr->col;
		prev->row = curr->row;
		prev->request.time = curr->request.time;
		prev->request.type = curr->request.type;
		prev->request.address = curr->request.address;
		prev->command.status = curr->command.status;
		prev->command.time = curr->command.time;

		prev = curr;
		curr = curr->next;
	}

	//move the tail up by one
	tail = prev;

	return QUEUE_OPEN;
}


// Checks if any of the memory requests in the queue need the row being accessed by this memory request
// if other requests need the row, then leave the row open, if not we can close the row
char check_request_row_addr(struct mem_req * curr)
{
	struct mem_req * temp;
	unsigned short crow = curr->row;
	char cbank = curr->bank;
	char cgroup = curr->group;
	char tbank;
	char tgroup;
	char trow;

	// if list is empty, we can definitely close the row
	if (!head)
		return CLOSE_ROW;

	temp = head;

	// if the queue is full you it won't actually look at the tail so we
	// do different look ups if the queue is full or not
	if (tail->next == head)
	{
		do
		{
			tbank = temp->bank;				// get the bank, group, and row information for the request
			tgroup = temp->group;			// because we need to be sure of the specific row
			trow = temp->row;
			if (temp != curr)				// look at all the requests except the one 
				if (tgroup == cgroup && tbank == cbank && trow == crow)		// are looking to see if there is a match for
					return LEAVE_ROW_OPEN;
			temp = temp->next;
		} while (temp != head);
	}
	else
	{
		// if queue is open, search to tail since tail points to the next open spot
		while (temp != tail)
		{
			tbank = temp->bank;				// get the bank, group, and row information for the request
			tgroup = temp->group;			// because we need to be sure of the specific row
			trow = temp->row;
			if (temp != curr)				// look at all the requests except the one 
				if (tgroup == cgroup && tbank == cbank && trow == crow)		// are looking to see if there is a match for
					return LEAVE_ROW_OPEN;
			temp = temp->next;
		}
	}
	return CLOSE_ROW;
}


// Returns the status of if the queue is full or not 
char get_queue_status(void)
{
	/* if tail is at the last item */
	if (tail->next == head)
		return QUEUE_FULL;
	
	if (head == NULL)
		return QUEUE_EMPTY;

	return QUEUE_OPEN;
}


// Returns the data from the head item in the mem_req argument
// Returns: 0 if failure, 1 if success
char get_request_info(struct mem_req * current)
{
	
	// if the queue is empty, return 
	if (!head)
		return QUEUE_EMPTY;

	current->request.address = head->request.address;
	current->request.type = head->request.type;
	current->request.time = head->request.time;
	current->group = head->group;
	current->bank = head->bank;
	current->col = head->col;
	current->row = head->row;
	current->command.status = head->command.status;
	current->command.time = head->command.time;

	return QUEUE_OPEN;
}


// Set the head item with the information from the DRAM command logic
// This is used for the closed page policy
void set_request_info(struct mem_req * update)
{
	head->request.address = update->request.address;
	head->request.type = update->request.type;
	head->request.time = update->request.time;
	head->group = update->group;
	head->bank = update->bank;
	head->col = update->col;
	head->row = update->row;
	head->command.status = update->command.status;
	head->command.time = update->command.time;
	head->queue_time = update->queue_time;
}


// Increment the clocks for each item in the queue
void inc_q_clocks(void)
{
	struct mem_req * current;

	// check if the queue is empty
	if (!head)
		return;

	current = head;
	do
	{
		++current->queue_time;			// increment the relative queue time, how long it has been in the queue
		++current->command.time;		// increment the time since the last command was issued
		current = current->next;
	} while (current != tail);

	return;
}


// get the end of the queue for the command scheduler
void get_queue_headptr(struct mem_req ** current)
{
	*current = head;
}


void get_queue_end(struct mem_req ** end)
{
	// if the queue is full set the end to the head so we check the last one
	if (tail->next == head)
		*end = head;
	else
		*end = tail;
}


// this function is for debug printing of diagnostic information
void d_print_stats()
{
#ifdef D_STATS_QUEUE
	printf("Queued: %d, Dequeued: %d\n", enqcount, deqcount);
	printf("Removed at head: %d, Removed at other: %d\n", removedhead, removedother);
#endif
}


// display the queue
void disp_queue(void)
{
	struct mem_req * current = head;

	if (!current)
	{
		printf("List is empty\n");
		return;
	}

	printf("Display queue...\n");
	// scan through the list and display the data nodes
	while (current != tail)
	{
		d_print_memreq(*current);
		current = current->next;
	}
	printf("\n");
}


