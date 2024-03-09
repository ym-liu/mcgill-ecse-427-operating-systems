#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "pcb.h"
#include "ready_queue.h"
#include "shellmemory.h"

int timer = 0;

struct memory_struct
{
	char *var;
	char *value;
	int last_used; // to keep track of Least-Recently Used frames (for replacement policy)
};

struct memory_struct shellmemory[SHELL_MEM_LENGTH];

// Helper functions
int match(char *model, char *var)
{
	int i, len = strlen(var), matchCount = 0;
	for (i = 0; i < len; i++)
		if (*(model + i) == *(var + i))
			matchCount++;
	if (matchCount == len)
		return 1;
	else
		return 0;
}

char *extract(char *model)
{
	char token = '=';			  // look for this to find value
	char value[SHELL_MEM_LENGTH]; // stores the extract value
	int i, j, len = strlen(model);
	for (i = 0; i < len && *(model + i) != token; i++)
		; // loop till we get there
	// extract the value
	for (i = i + 1, j = 0; i < len; i++, j++)
		value[j] = *(model + i);
	value[j] = '\0';
	return strdup(value);
}

// Shell memory functions

void mem_init()
{
	int i;
	for (i = 0; i < SHELL_MEM_LENGTH; i++)
	{
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
		shellmemory[i].last_used = timer;
	}
}

void variable_store_mem_init()
{
	int i;
	for (i = THRESHOLD; i < SHELL_MEM_LENGTH; i++)
	{
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in)
{
	int i;
	for (i = THRESHOLD; i < SHELL_MEM_LENGTH; i++)
	{
		if (strcmp(shellmemory[i].var, var_in) == 0)
		{
			shellmemory[i].value = strdup(value_in);
			return;
		}
	}

	// Value does not exist, need to find a free spot.
	for (i = THRESHOLD; i < SHELL_MEM_LENGTH; i++)
	{
		if (strcmp(shellmemory[i].var, "none") == 0)
		{
			shellmemory[i].var = strdup(var_in);
			shellmemory[i].value = strdup(value_in);
			return;
		}
	}

	return;
}

// get value based on input key
char *mem_get_value(char *var_in)
{
	int i;
	for (i = THRESHOLD; i < SHELL_MEM_LENGTH; i++)
	{
		if (strcmp(shellmemory[i].var, var_in) == 0)
		{
			return strdup(shellmemory[i].value);
		}
	}
	return NULL;
}

void printShellMemory()
{
	int count_empty = 0;
	for (int i = 0; i < SHELL_MEM_LENGTH; i++)
	{
		if (strcmp(shellmemory[i].var, "none") == 0)
		{
			count_empty++;
		}
		else
		{
			printf("\nline %d: key: %s\t\tvalue: %s\n", i, shellmemory[i].var, shellmemory[i].value);
		}
	}
	printf("\n\t%d lines in total, %d lines in use, %d lines free\n\n", SHELL_MEM_LENGTH, SHELL_MEM_LENGTH - count_empty, count_empty);
}

void printFrameStore()
{
	for (int i = 0; i < THRESHOLD; i++)
	{
		printf("\nline %d: key: %s\t\tvalue: %s\t\tlast used: %i\n", i, shellmemory[i].var, shellmemory[i].value, shellmemory[i].last_used);
	}
	printf("\n\n");
}

/*
 * Function:  addFileToMem
 * 	Added in A2
 * --------------------
 * Load the source code of the file fp into the shell memory:
 * 		Loading format - var stores fileID, value stores a line
 *		Note that the first 100 lines are for set command, the rests are for run and exec command
 *
 *  pStart: This function will store the first line of the loaded file
 * 			in shell memory in here
 *	pEnd: This function will store the last line of the loaded file
			in shell memory in here
 *  fileID: Input that need to provide when calling the function,
			stores the ID of the file
 *
 * returns: error code, 21: no space left
 */

int load_page_into_frame_store(FILE *fp, char *filename, int *page_table, int page_num)
{
	char *line;
	size_t i;
	int error_code = 21;
	bool hasSpaceLeft = false;
	bool hasLoadedFullPage = false;
	bool handlesPageFault = false;
	i = 0;

	// find first available hole
	for (i; i < THRESHOLD; i++)
	{
		if (strcmp(shellmemory[i].var, "none") == 0)
		{
			hasSpaceLeft = true;
			break;
		}
	}

	// if shell memory is full, handle page fault
	if (!(feof(fp)) && hasSpaceLeft == 0)
	{
		// pick victim frame to evict and evict it
		i = pick_victim_frame(page_table) * FRAME_SIZE;
		handlesPageFault = true;
		// and from here, we can bring missing page from backing store into frame store
	}

	// load page into shell memory
	for (size_t j = i; j <= THRESHOLD; j++)
	{
		// if we reached eof, then break
		if (feof(fp))
		{
			// printf("WHY BREAK: reached eof\n");
			// pad the rest of the page
			while ((j - i) % FRAME_SIZE != 0)
			{
				shellmemory[j].var = strdup(filename);
				shellmemory[j].value = strndup("none", 4 * sizeof(char));
				shellmemory[j].last_used = 0;
				j++;
			}

			break;
		}
		// if we reached threshold, then break
		else if (j == THRESHOLD)
		{
			// printf("WHY BREAK: reached threshold\n");
			// if we finished loading a full page
			if (((j - i) % FRAME_SIZE == 0 && j != i))
				hasLoadedFullPage = true;

			break;
		}
		// if we finished loading a full page, then break
		else if ((j - i) % FRAME_SIZE == 0 && j != i)
		{
			// printf("WHY BREAK: finished loading a full page.  ");
			// printf("i: %i, j: %i\n", i, j);
			hasLoadedFullPage = true;
			break;
		}

		else
		{
			line = calloc(1, SHELL_MEM_LENGTH);
			if (fgets(line, SHELL_MEM_LENGTH, fp) == NULL)
			{
				continue;
			}
			shellmemory[j].var = strdup(filename);
			shellmemory[j].value = strndup(line, strlen(line));
			shellmemory[j].last_used = timer;
			error_code = 0;

			if ((j - i) % FRAME_SIZE == 0)
			{
				// store frame# in page table
				if (page_num < PAGE_TABLE_SIZE) // ensure page table large enough
				{
					page_table[page_num] = j / FRAME_SIZE;
				}
				else
					printf("Page table not large enough to record frame# %i\n", j / FRAME_SIZE);
			}

			/*printf("LINE STORED: %s", line);
			if (feof(fp))
				printf("\n");*/

			free(line);
		}
	}

	// no space left to load the entire page into shell memory
	/*if (!feof(fp) && !hasLoadedFullPage) // does this even happen?
	{									   // only if THRESHOLD % FRAME_SIZE != 0
		error_code = 21;
		// clean up the file in memory
		for (int j = 1; i <= THRESHOLD; i++)
		{
			shellmemory[j].var = "none";
			shellmemory[j].value = "none";
		}
		return error_code;
	}*/
	// printShellMemory();
	//  printFrameStore();
	return error_code;
}

char *mem_get_value_at_line(int index)
{
	if (index < 0 || index > SHELL_MEM_LENGTH)
		return NULL;
	return shellmemory[index].value;
}

void mem_free_lines_between(int start, int end)
{
	for (int i = start; i <= end && i < SHELL_MEM_LENGTH; i++)
	{
		if (shellmemory[i].var != NULL)
		{
			free(shellmemory[i].var);
		}
		if (shellmemory[i].value != NULL)
		{
			free(shellmemory[i].value);
		}
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
		shellmemory[i].last_used = 0;
	}
}

// page_table: used to check if victim frame is from one's own page table
// returns victim frame
int pick_victim_frame(int *page_table)
{
	/*// PICK RANDOM FRAME
	srand(time(NULL));
	int victim_frame = rand() % FRAME_STORE_SIZE; // some number between 0 and (FRAME_STORE_SIZE-1)*/

	// PICK LRU FRAME
	int victim_frame = 0;
	for (int frame_num = 0; frame_num < FRAME_STORE_SIZE; frame_num++)
	{
		if (shellmemory[frame_num * FRAME_SIZE].last_used < shellmemory[victim_frame * FRAME_SIZE].last_used)
		{
			victim_frame = frame_num;
		}
	}

	// UPDATE PAGE TABLE
	// check if victim frame is from self
	for (int i = 0; i < PAGE_TABLE_SIZE; i++)
	{
		if (page_table[i] == victim_frame)
			page_table[i] = -1;
	}

	// find victim frame in ready_queue
	QueueNode *current = ready_queue_peek_head();
	bool frameNumFound = false;
	while (current != NULL)
	{
		// iterate through the current node's page table to find victim_frame
		for (int i = 0; i < PAGE_TABLE_SIZE; i++)
		{
			// if we found victim frame
			if (current->pcb->page_table[i] == victim_frame)
			{
				frameNumFound = true;
				// update victim frame's PCB's page table
				current->pcb->page_table[i] = -1;

				break;
			}
		}
		if (frameNumFound)
			break;
		current = ready_queue_peek_next(current);
	}

	if (!frameNumFound)
	{ // ...then what?
	}
	/*// for debugging purposes
	printf("PAGE TABLE for %s after eviction = [", current->pcb->filename);
	for (int k = 0; k < PAGE_TABLE_SIZE - 1; k++)
	{
		printf("%i, ", current->pcb->page_table[k]);
	}
	printf("%i]\n\n", current->pcb->page_table[PAGE_TABLE_SIZE - 1]);*/

	// PRINT PAGE FAULT
	printf("Page fault! Victim page contents:\n");
	for (int j = victim_frame * FRAME_SIZE; j < (victim_frame + 1) * FRAME_SIZE; j++)
	{
		if (strcmp("none", shellmemory[j].value) == 0)
		{
			printf("\n");
			break;
		}
		printf("%s", shellmemory[j].value);
	}
	printf("End of victim page contents.\n");

	// EVICT FRAME
	for (int i = victim_frame * FRAME_SIZE; i < (victim_frame + 1) * FRAME_SIZE; i++)
	{
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
		shellmemory[i].last_used = 0;
	}

	return victim_frame;
};

// increments timer by 1 and sets shellmemory[i].last_used to timer
void increment_lastused(int line_num)
{
	timer++;
	if (line_num >= 0 && line_num < THRESHOLD)
		shellmemory[line_num].last_used = timer;
}