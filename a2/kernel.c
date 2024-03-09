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
    // printf("-------- INITIALIZE_PROCESS IS CALLED --------\n");

    // LOAD SCRIPTS INTO BACKING STORE
    FILE *script_fp = fopen(filename, "r");
    if (script_fp == NULL)
        return FILE_DOES_NOT_EXIST;

    char script_in_backing_store[100];
    strcpy(script_in_backing_store, "./backing_store/");
    strcat(script_in_backing_store, filename);

    FILE *script_in_backing_store_fp = fopen(script_in_backing_store, "r");
    if (script_in_backing_store_fp != NULL)
    { // if file exists, rename in backing store
        int i = 0;
        while (script_in_backing_store_fp != NULL)
        {
            i++;
            char str_i[5];
            sprintf(str_i, "%i", i);

            strcpy(script_in_backing_store, "./backing_store/");
            strcat(script_in_backing_store, filename);
            strcat(script_in_backing_store, str_i);

            script_in_backing_store_fp = fopen(script_in_backing_store, "r");
        }
    }
    // copy into backing store
    char copy_into_backing_store[100];
    strcpy(copy_into_backing_store, "cp ");
    strcat(copy_into_backing_store, filename);
    strcat(copy_into_backing_store, " ");
    strcat(copy_into_backing_store, script_in_backing_store);
    system(copy_into_backing_store);

    // close original script files
    fclose(script_fp);
    // open files in backing store
    script_in_backing_store_fp = fopen(script_in_backing_store, "r");
    if (script_in_backing_store_fp == NULL)
        return FILE_DOES_NOT_EXIST;

    // LOAD SCRIPTS INTO FRAME STORE
    int page_table[PAGE_TABLE_SIZE];
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) // initialize page table with -1's
    {
        page_table[i] = -1;
    }
    // load first two pages into frame store
    int error_code = load_page_into_frame_store(script_in_backing_store_fp, filename, page_table, 0);
    /*if (error_code != 0)
    {
        fclose(script_in_backing_store_fp);
        return FILE_ERROR;
    }*/
    error_code = load_page_into_frame_store(script_in_backing_store_fp, filename, page_table, 1);
    /*if (error_code != 0)
    {
        fclose(script_in_backing_store_fp);
        return FILE_ERROR;
    }*/

    PCB *newPCB = makePCB_withPageTable(page_table, script_in_backing_store_fp, filename);

    /*// for debugging purposes
    printf("PAGE TABLE for %s = [", filename);
    for (int i = 0; i < PAGE_TABLE_SIZE - 1; i++)
    {
        printf("%i, ", newPCB->page_table[i]);
    }
    printf("%i]\n\n", newPCB->page_table[PAGE_TABLE_SIZE - 1]);*/

    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;

    ready_queue_add_to_tail(node);

    return 0;
}

bool execute_process(QueueNode *node, int quanta)
{
    // printf("-------- EXECUTE_PROCESS IS CALLED --------\n");

    char *line = NULL;
    PCB *pcb = node->pcb;
    /*// for debugging purposes
    printf("PAGE TABLE for %s = [", node->pcb->filename);
    for (int i = 0; i < PAGE_TABLE_SIZE - 1; i++)
    {
        printf("%i, ", node->pcb->page_table[i]);
    }
    printf("%i]\n\n", node->pcb->page_table[PAGE_TABLE_SIZE - 1]);*/
    // printf("PC for %s is currently at %i\n\n", pcb->filename, pcb->PC);

    // find where PC is in page table
    int page_num = 0;
    while (pcb->PC != pcb->page_table[page_num] * FRAME_SIZE) // go page-by-page
    {                                                         // if we find PC, exit while-loop
        // iterate through all the lines in the page
        int line_num = 0;
        for (line_num; line_num < FRAME_SIZE; line_num++) // go line-by-line
        {
            if (pcb->PC == (pcb->page_table[page_num] * FRAME_SIZE) + line_num && pcb->page_table[page_num] != -1)
                break;
        }

        // if we find PC, exit while-loop; if we did not find PC, go to next page
        if (pcb->PC == (pcb->page_table[page_num] * FRAME_SIZE) + line_num && pcb->page_table[page_num] != -1)
            break;
        else
            page_num++;

        // if we reach the end of the page table, do not execute anything
        // TODO: possibly happens when P2 page is evicted due to P1 page fault
        if (page_num >= PAGE_TABLE_SIZE)
        {
            printf("Could not locate program counter in page table for %s\n", pcb->filename);
            terminate_process(node);
            in_background = false;
            return true;
        }
    }

    // execute lines starting at PC
    for (int i = 0; i < quanta; i++)
    {
        // get line at PC
        // if line is on next page, get line and update PC to next page
        if (pcb->PC % FRAME_SIZE == 0 && pcb->PC != pcb->page_table[page_num] * FRAME_SIZE)
        {
            page_num++;
            // ensure there is, in fact, a next page
            if (page_num < PAGE_TABLE_SIZE && (pcb->page_table[page_num] >= 0 && pcb->page_table[page_num] < FRAME_STORE_SIZE))
            {
                pcb->PC = pcb->page_table[page_num] * FRAME_SIZE; // update PC to next page
                line = mem_get_value_at_line(pcb->PC);            // get line
            }
            // if there is no next page in the page table, handle page fault
            else if (page_num < PAGE_TABLE_SIZE && (pcb->page_table[page_num] == -1)) //&& !feof(pcb->fp)
            {
                // if there is no next page in the backing store, terminate process
                if (!handle_page_fault(pcb->fp, pcb->filename, pcb->page_table, page_num))
                {
                    terminate_process(node);
                    in_background = false;
                    return true;
                }

                pcb->PC = pcb->page_table[page_num] * FRAME_SIZE; // update PC to next page
                line = mem_get_value_at_line(pcb->PC);            // get line
            }
            // if we reached the end
            else
            {
                terminate_process(node);
                in_background = false;
                return true;
            }
        }
        // if line is on same page, get line
        else
            line = mem_get_value_at_line(pcb->PC);

        // increment PC
        // if PC++ is on next page, increment PC to next page
        if (pcb->PC % FRAME_SIZE == 2)
        {
            if (page_num < PAGE_TABLE_SIZE - 1)
            {
                page_num++; // next page
                if (pcb->page_table[page_num] == -1)
                {
                    if (!handle_page_fault(pcb->fp, pcb->filename, pcb->page_table, page_num))
                    {
                        terminate_process(node);
                        in_background = false;
                        return true;
                    }
                }
                pcb->PC = pcb->page_table[page_num] * FRAME_SIZE; // update PC to next page
            }
            else
                printf("Page table not large enough to search frame# %i\n", page_num + 1);
        } // else, same page, so increment PC by 1
        else
            pcb->PC++;

        // execute line
        in_background = true;
        if (pcb->priority)
        {
            pcb->priority = false;
        }
        if (strcmp(line, "none") == 0)
        { // if we reached a line that is padding
            terminate_process(node);
            in_background = false;
            return true;
        }
        parseInput(line);
        increment_lastused(pcb->PC - 1);
        in_background = false;
    }

    return false;
}

bool handle_page_fault(FILE *fp, char *filename, int *page_table, int page_num)
{
    // printf("-------- HANDLE_PAGE_FAULT_IS_CALLED --------\n");

    bool isInShellMemory = false;

    // INTERRUPT CURRENT PROCESS,
    // PLACE IT AT THE BACK OF THE READY QUEUE
    QueueNode *current_node = ready_queue_pop_head();
    if (current_node != NULL)
        ready_queue_add_to_tail(current_node);

    // BRING MISSING PAGE FROM BACKING STORE INTO FRAME STORE
    // UPDATE PAGE TABLE
    // TODO: check if missing page is in shell memory
    int error_code = load_page_into_frame_store(fp, filename, page_table, page_num);
    if (error_code == 0)
        isInShellMemory = true;

    /*// for debugging purposes
    printf("PAGE TABLE for %s after page fault = [", filename);
    for (int i = 0; i < PAGE_TABLE_SIZE - 1; i++)
    {
        printf("%i, ", page_table[i]);
    }
    printf("%i]\n\n", page_table[PAGE_TABLE_SIZE - 1]);*/

    return isInShellMemory;
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

/*void *scheduler_SJF()
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
}*/

/*void *scheduler_AGING_alternative()
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
}*/

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
        if (cur == NULL)
            return 0;

        if (!execute_process(cur, quanta))
        {
            ready_queue_add_to_tail(cur);
        }
    }
    return 0;
}

int schedule_by_policy(char *policy)
{ //, bool mt){
    if (strcmp(policy, "FCFS") != 0 /*&& strcmp(policy, "SJF") != 0*/ &&
        strcmp(policy, "RR") != 0 /*&& strcmp(policy, "AGING") != 0*/ && strcmp(policy, "RR30") != 0)
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
    /*else if (strcmp("SJF", policy) == 0)
    {
        scheduler_SJF();
    }*/
    else if (strcmp("RR", policy) == 0)
    {
        arg[0] = 2;
        scheduler_RR((void *)arg);
    }
    /*else if (strcmp("AGING", policy) == 0)
    {
        scheduler_AGING();
    }*/
    else if (strcmp("RR30", policy) == 0)
    {
        arg[0] = 30;
        scheduler_RR((void *)arg);
    }
    return 0;
}