#include "fsutil2.h"
#include "bitmap.h"
#include "cache.h"
#include "debug.h"
#include "directory.h"
#include "file.h"
#include "filesys.h"
#include "free-map.h"
#include "fsutil.h"
#include "inode.h"
#include "off_t.h"
#include "partition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int copy_in(char *fname)
{
  FILE *source_file = fopen(fname, "rb");
  if (!source_file)
  {
    return 0;
  }

  fseek(source_file, 0, SEEK_END);
  long size = ftell(source_file);
  rewind(source_file);

  char *buffer = malloc(size);
  if (!buffer)
  {
    fclose(source_file);
    return 1;
  }

  size_t bytes_read = fread(buffer, 1, size, source_file);
  fclose(source_file);

  if (bytes_read != size)
  {
    printf("Warning: could only read %zu out of %ld bytes (reached end of file)\n", bytes_read, size);
  }

  if (!filesys_create(fname, size, false))
  {
    free(buffer);
    return 9;
  }

  struct file *dest_file = filesys_open(fname);
  if (!dest_file)
  {
    free(buffer);
    return 10;
  }
  if (file_write(dest_file, buffer, bytes_read) != bytes_read)
  {
    file_close(dest_file);
    free(buffer);
    return 11;
  }
  file_close(dest_file);
  free(buffer);
}

int copy_out(char *fname)
{

  struct file *source_file = filesys_open(fname);
  if (!source_file)
  {
    return 0;
  }

  int size = file_length(source_file);
  if (size < 0)
  {
    file_close(source_file);
    return 10;
  }

  char *buffer = (char *)malloc(size);
  if (!buffer)
  {
    file_close(source_file);
    return 1;
  }

  int bytes_read = file_read(source_file, buffer, size);
  if (bytes_read != size)
  {
    free(buffer);
    file_close(source_file);
    return 12;
  }

  file_close(source_file);

  FILE *dest_file = fopen(fname, "wb");
  if (!dest_file)
  {
    free(buffer);
    return 9;
  }

  size_t bytes_written = fwrite(buffer, sizeof(char), size, dest_file);
  fclose(dest_file);
  free(buffer);

  if (bytes_written != size)
  {
    return 10;
  }
}

void find_file(char *pattern)
{
  struct dir *directory = dir_open_root();
  if (!directory)
  {
    return 10;
  }

  char name[NAME_MAX + 1];
  while (dir_readdir(directory, name))
  {
    struct file *file = filesys_open(name);
    if (!file)
    {
      continue;
    }

    int file_size = file_length(file);
    char *buffer = (char *)malloc((file_size + 1) * sizeof(char)); // Add 1 for null terminator
    if (!buffer)
    {
      printf("Error: Memory allocation failed.\n");
      file_close(file);
      continue;
    }

    int bytes_read = file_read(file, buffer, file_size);
    buffer[bytes_read] = '\0'; // Null-terminate the buffer
    file_close(file);

    if (strstr(buffer, pattern))
    {
      printf("%s\n", name);
    }

    free(buffer);
  }

  dir_close(directory);
}

void fragmentation_degree()
{
  int free_sectors = num_free_sectors();
  int total_sectors = block_size(fs_device);
  int used_sectors = total_sectors - free_sectors;
  printf("Fragmentation degree: %d%%\n", (100 * free_sectors) / used_sectors);
}

int defragment()
{
}

void recover(int flag)
{
  if (flag == 0)
  { // recover deleted inodes

    // TODO
  }
  else if (flag == 1)
  { // recover all non-empty sectors

    // TODO
  }
  else if (flag == 2)
  { // data past end of file.

    // TODO
  }
}