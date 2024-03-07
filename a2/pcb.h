#ifndef PCB_H
#define PCB_H
#include <stdbool.h>

#define PAGE_TABLE_SIZE 20

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
    int PC;
    // int start;
    // int end;
    int job_length_score;
    int page_table[PAGE_TABLE_SIZE];
    FILE *fp;
    char filename[100];
} PCB;

int generatePID();
/*PCB *makePCB(int start, int end);*/
PCB *makePCB_withPageTable(int *page_table, FILE *fp, char *filename);
#endif