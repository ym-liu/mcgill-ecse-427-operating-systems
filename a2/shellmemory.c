#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "pcb.h"
#include "shellmemory.h"
#include "ready_queue.h"

#define SHELL_MEM_LENGTH 1000
const int FRAME_STORE_SIZE = 10;					 // size of frame store (# of pages in frame store)
const int FRAME_SIZE = 3;							 // size of a single frame (# of lines in a page)
const int THRESHOLD = FRAME_STORE_SIZE * FRAME_SIZE; // threshold separating frame store from variable store

struct memory_struct
{
	char *var;
	char *value;
};

struct memory_struct shellmemory[SHELL_MEM_LENGTH];

// TODO: function to alloc a new frame
// use shell memory functions

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
	char token = '='; // look for this to find value
	char value[1000]; // stores the extract value
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
	for (i = 0; i < 1000; i++)
	{
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

void variable_store_mem_init()
{
	int i;
	for (i = THRESHOLD; i < 1000; i++)
	{
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in)
{
	int i;
	for (i = THRESHOLD; i < 1000; i++)
	{
		if (strcmp(shellmemory[i].var, var_in) == 0)
		{
			shellmemory[i].value = strdup(value_in);
			return;
		}
	}

	// Value does not exist, need to find a free spot.
	for (i = THRESHOLD; i < 1000; i++)
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
	for (i = 0; i < 1000; i++)
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
int load_file(FILE *fp, int *pStart, int *pEnd, char *filename)
{
	char *line;
	size_t i;
	int error_code = 0;
	bool hasSpaceLeft = false;
	bool flag = true;
	i = 101;
	size_t candidate;
	while (flag)
	{
		flag = false;
		for (i; i < SHELL_MEM_LENGTH; i++)
		{
			if (strcmp(shellmemory[i].var, "none") == 0)
			{
				*pStart = (int)i;
				hasSpaceLeft = true;
				break;
			}
		}
		candidate = i;
		for (i; i < SHELL_MEM_LENGTH; i++)
		{
			if (strcmp(shellmemory[i].var, "none") != 0)
			{
				flag = true;
				break;
			}
		}
	}
	i = candidate;
	// shell memory is full
	if (hasSpaceLeft == 0)
	{
		error_code = 21;
		return error_code;
	}

	for (size_t j = i; j < SHELL_MEM_LENGTH; j++)
	{
		if (feof(fp))
		{
			*pEnd = (int)j - 1;
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
			free(line);
		}
	}

	// no space left to load the entire file into shell memory
	if (!feof(fp))
	{
		error_code = 21;
		// clean up the file in memory
		for (int j = 1; i <= SHELL_MEM_LENGTH; i++)
		{
			shellmemory[j].var = "none";
			shellmemory[j].value = "none";
		}
		return error_code;
	}
	// printShellMemory();
	return error_code;
}

// load file but only load one page
// Load one page of the source code of the file fp into the frame store of shell memory
int load_page(FILE *fp, char *filename, PCB *pPCB, int line_start)
{
	char *line;
	size_t i;
	int error_code = 0;
	bool hasSpaceLeft = false;
	bool flag = true;
	i = 0;
	int line_count = 0;
	size_t candidate;
	while (flag)
	{
		flag = false;
		for (i; i < THRESHOLD; i++)
		{
			if (strcmp(shellmemory[i].var, "none") == 0)
			{
				hasSpaceLeft = true;
				break;
			}
		}
		candidate = i;
		for (i; i < THRESHOLD; i++)
		{
			if (strcmp(shellmemory[i].var, "none") != 0)
			{
				flag = true;
				break;
			}
		}
	}
	i = candidate;
	// shell memory is full
	if (hasSpaceLeft == 0)
	{
		error_code = 21;
		return error_code;
	}

	// create new page
	int index[3] = {-1, -1, -1};
	int valid_bits[3] = {-1, -1, -1};
	int page_index = 0;
	PAGE *page_p = makePAGE(index, valid_bits, -1, filename);
	print_ready_queue();
	// if (page_p == NULL), then what do?

	// add new page to page table
	if (pPCB->page_table == NULL)
		pPCB->page_table = &page_p;
	// else?

	for (size_t j = i; j < THRESHOLD; j++)
	{
		if (feof(fp))
		{
			while ((j - i) % FRAME_SIZE != 0) // fill the rest of the page with invalid
			{
				page_p->index[(j - i) % FRAME_SIZE] = j;
				page_p->valid_bits[(j - i) % FRAME_SIZE] = 0; // 0 = INVALID
				shellmemory[j].var = strdup(filename);
				shellmemory[j].value = strndup("none", 1 * sizeof(char));
				printf("page index of line: %d    ", page_p->index[(j - i) % FRAME_SIZE]);
				printf("valid bit of line: %d", page_p->valid_bits[(j - i) % FRAME_SIZE]);
				printf("\n");
				j++;
			}
			break;
		}
		else
		{
			line = calloc(1, THRESHOLD);
			if (fgets(line, THRESHOLD, fp) == NULL)
				continue;

			if (line_count < line_start)
				continue; // do not read lines before line_start
			line_count++;

			// after loading 3 lines (i.e., one page), break
			if ((j - i) % FRAME_SIZE == 0 && j != i)
				break;

			// add line to page
			page_p->index[(j - i) % FRAME_SIZE] = j;
			page_p->valid_bits[(j - i) % FRAME_SIZE] = 1; // 1 = VALID
			printf("page index of line: %d    ", page_p->index[(j - i) % FRAME_SIZE]);
			printf("valid bit of line: %d", page_p->valid_bits[(j - i) % FRAME_SIZE]);
			printf("\n");

			shellmemory[j].var = strdup(filename);
			shellmemory[j].value = strndup(line, strlen(line));
			free(line);
		}
	}

	// no space left to load the entire file into shell memory
	if (!feof(fp))
	{
		error_code = 21;
		// clean up the file in memory
		for (int j = 1; i <= THRESHOLD; i++)
		{
			shellmemory[j].var = "none";
			shellmemory[j].value = "none";
		}
		return error_code;
	}
	// printShellMemory();
	printf("new page indices: %i, %i, %i\nnew page page_index: %i\nnew page pid: %i\n",
		   (*(pPCB->page_table))->index[0], (*(pPCB->page_table))->index[1], (*(pPCB->page_table))->index[2],
		   (*(pPCB->page_table))->index, (*(pPCB->page_table))->page_pid);
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
	}
}