#ifndef PCB_H
#define PCB_H
#include <stdbool.h>

/*
 * Struct:  PAGE
 * --------------------
 * index:
 * valid_bits:
 * page_index:
 * page_pid:
 */
typedef struct PAGE PAGE;
struct PAGE
{
    int index[3];      // array of 3, bc 3 lines per page (FRAME_SIZE = 3)
    int valid_bits[3]; // array of 3, bc 3 lines per page (FRAME_SIZE = 3)
    int page_index;
    char *page_pid;
};

/*
 * Struct:  PCB
 * --------------------
 * pid: process(task) id
 * PC: program counter, stores the index of line that the task is executing
 * start: the first line in shell memory that belongs to this task
 * end: the last line in shell memory that belongs to this task
 * job_length_score: for EXEC AGING use only, stores the job length score
 */
typedef struct
{
    bool priority;
    int pid;
    int PC; // which line the PC is on
    // int PC_page; // which page the PC is on
    // int start;
    // int end;
    int job_length_score;
    PAGE **page_table;
    int num_of_pages;
} PCB;

int generatePID();
PCB *makePCB();
PAGE *makePAGE(int index[3], int valid_bits[3], int page_index, char *page_pid);
#endif