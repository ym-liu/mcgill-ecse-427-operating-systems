#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "shellmemory.h"
#include "pcb.h"

int pid_counter = 1;
// const int FRAME_SIZE = 3;

int generatePID()
{
    return pid_counter++;
}

// In this implementation, Pid is the same as file ID
/*PCB *makePCB(int start, int end)
{
    PCB *newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = start;
    newPCB->start = start;
    newPCB->end = end;
    newPCB->job_length_score = 1 + end - start;
    newPCB->priority = false;
    newPCB->page_table[0] = -1;
    // TODO: add page table: array of pages PAGE ** page_table;
    // TODO: int num_of_pages;
    return newPCB;
}*/

PCB *makePCB_withPageTable(int *page_table)
{
    PCB *newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->job_length_score = 0; // note to self: https://edstem.org/us/courses/52582/discussion/4371690?comment=10269431
    newPCB->priority = false;

    for (int i = 0; i < PAGE_TABLE_SIZE; i++)
        newPCB->page_table[i] = page_table[i];

    newPCB->PC = (newPCB->page_table[0]) * FRAME_SIZE;

    return newPCB;
}