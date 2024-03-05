#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "ready_queue.h"
#include "interpreter.h"

bool active = false;
bool debug = false;
bool in_background = false;

int process_initialize(char *filename)
{
    // LOAD SCRIPTS INTO BACKING STORE
    FILE *script_fp = fopen(filename, "r");
    if (script_fp == NULL)
        return FILE_DOES_NOT_EXIST;

    char script_in_backing_store[100];
    strcpy(script_in_backing_store, "./backing_store/");
    strcat(script_in_backing_store, filename);

    char copy_into_backing_store[100];
    strcpy(copy_into_backing_store, "cp ");
    strcat(copy_into_backing_store, filename);
    strcat(copy_into_backing_store, " ");
    strcat(copy_into_backing_store, script_in_backing_store);
    system(copy_into_backing_store);

    // close original script files
    fclose(script_fp);
    // open files in backing store
    FILE *script_in_backing_store_fp = fopen(script_in_backing_store, "r+");
    if (script_in_backing_store_fp == NULL)
        return FILE_DOES_NOT_EXIST;

    // LOAD SCRIPTS INTO FRAME STORE
    int *start = (int *)malloc(sizeof(int));
    int *end = (int *)malloc(sizeof(int));
    int page_table[PAGE_TABLE_SIZE];
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) // initialize page table with -1's
    {
        page_table[i] = -1;
    }
    int error_code = load_file_into_frame_store(script_in_backing_store_fp, start, end, filename, page_table);
    if (error_code != 0)
    {
        fclose(script_in_backing_store_fp);
        return FILE_ERROR;
    }
    PCB *newPCB = makePCB_withPageTable(*start, *end, page_table);
    printf("PAGE TABLE for %s = [", filename);
    for (int i = 0; i < PAGE_TABLE_SIZE - 1; i++)
    {
        printf("%i, ", newPCB->page_table[i]);
    }
    printf("%i]\n\n", newPCB->page_table[PAGE_TABLE_SIZE - 1]);

    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;

    ready_queue_add_to_tail(node);

    fclose(script_in_backing_store_fp);
    return 0;
}

int shell_process_initialize()
{
    // Note that "You can assume that the # option will only be used in batch mode."
    // So we know that the input is a file, we can directly load the file into ram
    int *start = (int *)malloc(sizeof(int));
    int *end = (int *)malloc(sizeof(int));
    int error_code = 0;
    error_code = load_file(stdin, start, end, "_SHELL");
    if (error_code != 0)
    {
        return error_code;
    }
    PCB *newPCB = makePCB(*start, *end);
    newPCB->priority = true;
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;

    ready_queue_add_to_head(node);

    freopen("/dev/tty", "r", stdin);
    return 0;
}

bool execute_process(QueueNode *node, int quanta)
{
    char *line = NULL;
    PCB *pcb = node->pcb;
    for (int i = 0; i < quanta; i++)
    {
        line = mem_get_value_at_line(pcb->PC++);
        in_background = true;
        if (pcb->priority)
        {
            pcb->priority = false;
        }
        if (pcb->PC > pcb->end || strcmp(line, "none") == 0)
        { // if we reached the end or reached a line that is padding
            if (strcmp(line, "none") != 0)
                parseInput(line);
            terminate_process(node);
            in_background = false;
            return true;
        }
        parseInput(line);
        in_background = false;
    }
    return false;
}

void *scheduler_FCFS()
{
    QueueNode *cur;
    while (true)
    {
        if (is_ready_empty())
        {
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_head();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_SJF()
{
    QueueNode *cur;
    while (true)
    {
        if (is_ready_empty())
        {
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_shortest_job();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_AGING_alternative()
{
    QueueNode *cur;
    while (true)
    {
        if (is_ready_empty())
        {
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_shortest_job();
        ready_queue_decrement_job_length_score();
        if (!execute_process(cur, 1))
        {
            ready_queue_add_to_head(cur);
        }
    }
    return 0;
}

void *scheduler_AGING()
{
    QueueNode *cur;
    int shortest;
    sort_ready_queue();
    while (true)
    {
        if (is_ready_empty())
        {
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_head();
        shortest = ready_queue_get_shortest_job_score();
        if (shortest < cur->pcb->job_length_score)
        {
            ready_queue_promote(shortest);
            ready_queue_add_to_tail(cur);
            cur = ready_queue_pop_head();
        }
        ready_queue_decrement_job_length_score();
        if (!execute_process(cur, 1))
        {
            ready_queue_add_to_head(cur);
        }
    }
    return 0;
}

void *scheduler_RR(void *arg)
{
    int quanta = ((int *)arg)[0];
    QueueNode *cur;
    while (true)
    {
        if (is_ready_empty())
        {
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_head();
        if (!execute_process(cur, quanta))
        {
            ready_queue_add_to_tail(cur);
        }
    }
    return 0;
}

int schedule_by_policy(char *policy)
{ //, bool mt){
    if (strcmp(policy, "FCFS") != 0 && strcmp(policy, "SJF") != 0 &&
        strcmp(policy, "RR") != 0 && strcmp(policy, "AGING") != 0 && strcmp(policy, "RR30") != 0)
    {
        return SCHEDULING_ERROR;
    }
    if (active)
        return 0;
    if (in_background)
        return 0;
    int arg[1];
    if (strcmp("FCFS", policy) == 0)
    {
        scheduler_FCFS();
    }
    else if (strcmp("SJF", policy) == 0)
    {
        scheduler_SJF();
    }
    else if (strcmp("RR", policy) == 0)
    {
        arg[0] = 2;
        scheduler_RR((void *)arg);
    }
    else if (strcmp("AGING", policy) == 0)
    {
        scheduler_AGING();
    }
    else if (strcmp("RR30", policy) == 0)
    {
        arg[0] = 30;
        scheduler_RR((void *)arg);
    }
    return 0;
}