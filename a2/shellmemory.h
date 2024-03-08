#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

// #define VARMEMSIZE // num of lines in variable store
// #define THRESHOLD  // num of lines in frame store; threshold separating frame store from variable store
#define FRAME_SIZE 3                              // size of a single frame (# of lines in a page)
#define FRAME_STORE_SIZE (THRESHOLD / FRAME_SIZE) // size of frame store (# of pages in frame store)
#define SHELL_MEM_LENGTH (VARMEMSIZE + THRESHOLD)

void mem_init();
void variable_store_mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int load_file_into_frame_store(FILE *fp, char *fileID, int *page_table);
int load_page_into_frame_store(FILE *fp, char *fileID, int *page_table, int page_num);
char *mem_get_value_at_line(int index);
void mem_free_lines_between(int start, int end);
void printShellMemory();
void printFrameStore();
int pick_victim_frame(int *page_table);
void increment_lastused(int line_num);
#endif