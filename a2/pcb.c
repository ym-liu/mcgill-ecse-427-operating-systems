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
    newPCB->page_table[0] = -1;
    // TODO: add page table: array of pages PAGE ** page_table;
    // TODO: int num_of_pages;
    return newPCB;
}

PCB *makePCB_withPageTable(int start, int end, int *page_table)
{
    PCB *newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = start;
    newPCB->start = start;
    newPCB->end = end;
    newPCB->job_length_score = 1 + end - start;
    newPCB->priority = false;

    int i = 0;
    for (int i = 0; i < PAGE_TABLE_SIZE; i++)
        newPCB->page_table[i] = page_table[i];

    return newPCB;
}