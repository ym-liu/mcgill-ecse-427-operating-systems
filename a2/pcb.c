#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"

int pid_counter = 1;

int generatePID()
{
    return pid_counter++;
}

// In this implementation, Pid is the same as file ID
PCB *makePCB() // (int start, int end)
{
    PCB *newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = -1;
    // newPCB->start = start;
    // newPCB->end = end;
    // newPCB->job_length_score = 1 + end - start;
    newPCB->priority = false;
    // TODO: add page table: array of pages
    newPCB->page_table = NULL;
    newPCB->num_of_pages = 0;
    return newPCB;
}

PAGE *makePAGE(int index[3], int valid_bits[3], int page_index, char *page_pid)
{
    PAGE *newPAGE = malloc(sizeof(PAGE));
    newPAGE->index[0] = index[0];
    newPAGE->index[1] = index[1];
    newPAGE->index[2] = index[2];
    newPAGE->valid_bits[0] = valid_bits[0];
    newPAGE->valid_bits[1] = valid_bits[1];
    newPAGE->valid_bits[2] = valid_bits[2];
    newPAGE->page_index = page_index;
    newPAGE->page_pid = page_pid;
    return newPAGE;
}