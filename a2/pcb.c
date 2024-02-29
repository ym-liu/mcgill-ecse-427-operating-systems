#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"

int pid_counter = 1;
// const int FRAME_SIZE = 3;

int generatePID()
{
    return pid_counter++;
}

// In this implementation, Pid is the same as file ID
PCB *makePCB(int start, int end)
{
    PCB *newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = start;
    newPCB->start = start;
    newPCB->end = end;
    newPCB->job_length_score = 1 + end - start;
    newPCB->priority = false;
    // TODO: add page table: array of pages PAGE ** page_table;
    // TODO: int num_of_pages;
    return newPCB;
}