#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "shellmemory.h"
#include "pcb.h"

int pid_counter = 1;

int generatePID()
{
    return pid_counter++;
}

// In this implementation, Pid is the same as file ID
PCB *makePCB_withPageTable(int *page_table, FILE *fp, char *filename)
{
    PCB *newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->job_length_score = 0; // note to self: https://edstem.org/us/courses/52582/discussion/4371690?comment=10269431
    newPCB->priority = false;
    newPCB->fp = fp;
    strcpy(newPCB->filename, filename);

    for (int i = 0; i < PAGE_TABLE_SIZE; i++)
        newPCB->page_table[i] = page_table[i];

    newPCB->PC = (newPCB->page_table[0]) * FRAME_SIZE;

    return newPCB;
}