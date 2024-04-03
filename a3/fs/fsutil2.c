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

int count_fragmentable_files()
{
  int count = 0;
  struct dir *dir = dir_open_root();
  struct dir_entry e;

  while (dir_readdir(dir, e.name))
  {
    struct inode *inode = inode_open(e.inode_sector);
    if (inode != NULL && !inode_is_directory(inode) && !inode_is_removed(inode))
    {
      off_t file_size = inode_length(inode);
      off_t last_block_size = file_size % BLOCK_SECTOR_SIZE;
      if (last_block_size > 0 && last_block_size < BLOCK_SECTOR_SIZE)
      {
        count++;
      }
      inode_close(inode);
    }
  }
  dir_close(dir);
  return count;
}

int count_fragmented_files()
{
  int count = 0;
  struct dir *dir = dir_open_root();
  struct dir_entry e;

  while (dir_readdir(dir, e.name))
  {
    struct inode *inode = inode_open(e.inode_sector);
    if (inode != NULL && !inode_is_directory(inode) && !inode_is_removed(inode))
    {
      bool is_fragmented = false;
      block_sector_t previous_sector = (block_sector_t)-1;
      off_t file_length = inode_length(inode);
      off_t num_sectors = bytes_to_sectors(file_length);

      for (off_t i = 0; i < num_sectors; ++i)
      {
        block_sector_t current_sector = byte_to_sector(inode, i * BLOCK_SECTOR_SIZE);
        printf(current_sector);
        if (current_sector == (block_sector_t)-1)
        {
          is_fragmented = true;
          break;
        }

        if (previous_sector != (block_sector_t)-1 &&
            (current_sector > previous_sector + 3 || current_sector < previous_sector - 3))
        {
          is_fragmented = true;
          break;
        }
        previous_sector = current_sector;
      }

      if (is_fragmented)
      {
        count++;
      }

      inode_close(inode);
    }
  }

  dir_close(dir);
  return count;
}

void fragmentation_degree(void)
{
  int num_fragmentable_files = count_fragmentable_files();
  int num_fragmented_files = count_fragmented_files();

  float degree_of_fragmentation = 0.0;
  if (num_fragmentable_files > 0)
  {
    degree_of_fragmentation = (float)num_fragmented_files / num_fragmentable_files;
  }

  printf("Num fragmentable files: %d\n", num_fragmentable_files);
  printf("Num fragmented files: %d\n", num_fragmented_files);
  printf("Fragmentation pct: %.6f\n", degree_of_fragmentation);
}

int defragment()
{
}

static void recover_deleted_files();
static void recover_data_blocks();
static void find_hidden_data_in_files();

void recover(int flag)
{
  switch (flag)
  {
  case 0:
    recover_deleted_files();
    break;
  case 1:
    recover_data_blocks();
    break;
  case 2:
    find_hidden_data_in_files();
    break;
  default:
    printf("Invalid flag for recovery.\n");
    break;
  }
}

static void recover_deleted_files()
{
  struct bitmap *used_sectors = bitmap_create(block_size(fs_device));
  free_map_open();

  for (size_t i = 0; i < bitmap_size(used_sectors); i++)
  {
    if (!bitmap_test(used_sectors, i))
    {
      struct inode *inode = inode_open(i);
      if (inode != NULL && inode_is_removed(inode))
      {

        char filename[16];
        snprintf(filename, sizeof(filename), "recovered0-%d", i);

        struct dir *root_dir = dir_open_root();
        dir_add(root_dir, filename, i, false);
        dir_close(root_dir);

        inode->removed = false;
        inode_close(inode);
      }
    }
  }

  bitmap_destroy(used_sectors);
}

static void recover_data_blocks()
{
  block_sector_t total_sectors = block_size(fs_device);
  for (block_sector_t i = 4; i < total_sectors; i++)
  {
    char buffer[BLOCK_SECTOR_SIZE] = {0};
    block_read(fs_device, i, buffer);
    bool is_non_zero = false;
    for (unsigned j = 0; j < BLOCK_SECTOR_SIZE; j++)
    {
      if (buffer[j] != 0)
      {
        is_non_zero = true;
        break;
      }
    }
    if (is_non_zero)
    {
      char filename[25];
      snprintf(filename, sizeof(filename), "recovered1-%d.txt", i);
      FILE *fp = fopen(filename, "w");
      if (fp != NULL)
      {
        size_t data_size = 0;
        while (data_size < BLOCK_SECTOR_SIZE && buffer[data_size] != '\0')
        {
          data_size++;
        }
        fwrite(buffer, 1, data_size, fp);
        fclose(fp);
      }
    }
  }
}
static void find_hidden_data_in_files()
{
  struct dir *dir = dir_open_root();
  struct dir_entry e;

  while (dir_readdir(dir, e.name))
  {
    struct inode *inode = inode_open(e.inode_sector);
    if (inode == NULL || inode_is_directory(inode) || inode_is_removed(inode))
    {
      continue;
    }

    off_t length = inode_length(inode);
    off_t last_block_bytes = length % BLOCK_SECTOR_SIZE;
    if (last_block_bytes > 0 && last_block_bytes < BLOCK_SECTOR_SIZE)
    {
      char buffer[BLOCK_SECTOR_SIZE];
      block_sector_t last_block_sector = bytes_to_sectors(length - 1);
      block_read(fs_device, last_block_sector, buffer);

      bool has_non_null = false;
      for (int i = 0; i < last_block_bytes; ++i)
      {
        if (buffer[i] != '\0')
        {
          has_non_null = true;
          break;
        }
      }

      if (has_non_null)
      {
        char filename[25];
        snprintf(filename, sizeof(filename), "recovered2-%s.txt", e.name);
        FILE *fp = fopen(filename, "w");
        if (fp != NULL)
        {
          fwrite(buffer, 1, last_block_bytes, fp);
          fclose(fp);
        }
      }
    }
    inode_close(inode);
  }
  dir_close(dir);
}