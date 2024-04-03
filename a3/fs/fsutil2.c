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
#include <sys/types.h>

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

  char *buffer = malloc(size + 1);
  if (!buffer)
  {
    fclose(source_file);
    return 1;
  }

  fread(buffer, 1, size, source_file);
  fclose(source_file);

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
  size_t bytes_written = file_write(dest_file, buffer, size + 1);
  if (bytes_written != size + 1)
  {
    file_close(dest_file);
    free(buffer);
    return 11;
  }
  file_seek(dest_file, bytes_written - 2);
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

  int i;
  for (i = 0; i < size; i++)
  {
    if (buffer[i] != '\0')
    {
      fputc(buffer[i], dest_file);
    }
  }

  fclose(dest_file);
  free(buffer);

  return 0;
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

static bool is_file_fragmented(struct inode *inode)
{
  if (inode == NULL || inode_length(inode) <= BLOCK_SECTOR_SIZE)
  {
    return false;
  }

  block_sector_t prev_sector = -1;
  off_t length = inode_length(inode);
  off_t offset;
  for (offset = 0; offset < length; offset += BLOCK_SECTOR_SIZE)
  {
    block_sector_t sector = byte_to_sector(inode, offset);
    if (prev_sector != (block_sector_t)-1 && sector > prev_sector + 3)
    {
      return true;
    }
    prev_sector = sector;
  }

  return false;
}

void fragmentation_degree()
{
  size_t fragmentable_files = 0;
  size_t fragmented_files = 0;

  struct file *file_iter = NULL;
  while ((file_iter = next_file(file_iter)) != NULL)
  {
    struct inode *inode = file_get_inode(file_iter);
    if (inode_is_directory(inode) || inode_length(inode) <= BLOCK_SECTOR_SIZE)
    {
      continue;
    }

    fragmentable_files++;
    if (is_file_fragmented(inode))
    {
      fragmented_files++;
    }
  }

  double fragmentation_degree = 0;
  if (fragmentable_files > 0)
  {
    fragmentation_degree = (double)fragmented_files / fragmentable_files;
  }

  printf("Fragmentation Degree: %.2f%%\n", fragmentation_degree * 100);
}

static bool defragment_file(struct inode *inode)
{
  off_t length = inode_length(inode);
  void *buffer = malloc(length);
  if (buffer == NULL)
  {
    return false;
  }

  inode_read_at(inode, buffer, length, 0);

  inode_deallocate(inode);

  struct inode_disk *disk_inode = inode_disk(inode);
  disk_inode->length = 0;
  inode_reserve(disk_inode, length);
  inode_write_at(inode, buffer, length, 0);

  free(buffer);
  return true;
}

int defragment()
{

  struct file *file_iter = NULL;
  while ((file_iter = next_file(file_iter)) != NULL)
  {
    struct inode *inode = file_get_inode(file_iter);
    if (inode_is_directory(inode))
    {
      continue;
    }

    if (is_file_fragmented(inode))
    {
      defragment_file(inode);
    }
  }

  printf("Defragmentation completed.\n");
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