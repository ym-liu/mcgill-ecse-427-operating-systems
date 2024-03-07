#ifndef KERNEL
#define KERNEL
#include "pcb.h"
int process_initialize(char *filename);
bool handle_page_fault(FILE *fp, char *filename, int *page_table, int page_num);
int schedule_by_policy(char *policy); //, bool mt);
void ready_queue_destory();
#endif