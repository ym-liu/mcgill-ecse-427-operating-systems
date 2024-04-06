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
    char *buffer = (char *)malloc((file_size + 1) * sizeof(char));
    if (!buffer)
    {
      file_close(file);
      continue;
    }

    int bytes_read = file_read(file, buffer, file_size);
    buffer[bytes_read] = '\0';
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

  while (true)
  {
    block_sector_t inode_file = dir_readdir_inode(dir, e.name);
    if (inode_file == -1)
    {
      break;
    }
    struct inode *inode = inode_open(inode_file);
    if (inode != NULL && !inode_is_directory(inode) && !inode_is_removed(inode))
    {
      bool is_fragmented = false;
      block_sector_t *inode_sectors = get_inode_data_sectors(inode);

      for (size_t i = 1; i < inode_length(inode) / BLOCK_SECTOR_SIZE; i++)
      {
        if (inode_sectors[i] - inode_sectors[i - 1] > 3)
        {
          is_fragmented = true;
          break;
        }
      }

      if (is_fragmented)
      {
        count++;
      }
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

char *read_file_into_memory(const char *filename)
{
  FILE *file = fopen(filename, "r");
  if (file == NULL)
  {
    fprintf(stderr, "Error: Cannot open file %s\n", filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = (char *)malloc(file_size + 1);
  if (buffer == NULL)
  {
    fprintf(stderr, "Error: Memory allocation failed\n");
    fclose(file);
    return NULL;
  }

  fread(buffer, 1, file_size, file);
  buffer[file_size] = '\0';
  fclose(file);
  return buffer;
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
  free_map_open();
  for (size_t sector = 0; sector < bitmap_size(free_map); sector++)
  {
    if (!bitmap_test(free_map, sector))
    {
      struct inode_disk *buffer = malloc(BLOCK_SECTOR_SIZE);
      buffer_cache_read(sector, buffer);

      if (buffer->magic == INODE_MAGIC)
      {
        struct inode *recovered_inode = inode_open(sector);
        char *recovered_data = malloc(recovered_inode->data.length);
        inode_read_at(recovered_inode, recovered_data, recovered_inode->data.length, 0);

        char filename[NAME_MAX];
        snprintf(filename, sizeof(filename), "recovered0-%d", sector);

        if (filesys_open(filename) == NULL)
        {
          struct dir *dir = dir_open_root();
          if (dir != NULL)
          {
            dir_add(dir, filename, sector, false);
            dir_close(dir);
          }
        }
        struct file *recovered_file = filesys_open(filename);
        file_write_at(recovered_file, recovered_data, recovered_inode->data.length, 0);
      }
    }
  }
}

static void recover_data_blocks()
{
  block_sector_t total_sectors = block_size(fs_device);
  for (block_sector_t i = 4; i < total_sectors - 1; i++)
  {
    char buffer[BLOCK_SECTOR_SIZE] = {0};
    block_read(fs_device, i, buffer);
    bool is_non_zero = false;
    for (int j = 0; j < BLOCK_SECTOR_SIZE; j++)
    {
      if (buffer[j] != 0)
      {
        is_non_zero = true;
        break;
      }
    }

    if (is_non_zero)
    {
      char filename[NAME_MAX];
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

  while (true)
  {
    block_sector_t inode_file = dir_readdir_inode(dir, e.name);
    if (inode_file == -1)
    {
      break;
    }
    struct inode *inode = inode_open(inode_file);

    if (inode == NULL || inode_is_directory(inode) || inode_is_removed(inode))
    {
      continue;
    }
    block_sector_t sector = inode->sector;
    struct inode_disk *buffer = malloc(BLOCK_SECTOR_SIZE);
    buffer_cache_read(sector, buffer);

    if ((buffer->magic == INODE_MAGIC) && (buffer->length % BLOCK_SECTOR_SIZE != 0))
    {

      block_sector_t *data_sectors = get_inode_data_sectors(inode);
      block_sector_t last_block = 0;
      size_t sectors = bytes_to_sectors(inode_length(inode));
      last_block = data_sectors[sectors - 1];

      int modulo = fsutil_size(e.name) % BLOCK_SECTOR_SIZE;
      int left = BLOCK_SECTOR_SIZE - modulo;
      char *block = malloc(sizeof(char) * BLOCK_SECTOR_SIZE);
      char *recovered = malloc(sizeof(char) * BLOCK_SECTOR_SIZE);
      buffer_cache_read(last_block, block);
      memcpy(recovered, block + modulo, left);

      char filename[NAME_MAX];
      snprintf(filename, sizeof(filename), "recovered2-%s.txt", e.name);
      FILE *fp = fopen(filename, "w");
      if (fp != NULL)
      {
        for (int i = 0; i < BLOCK_SECTOR_SIZE; i++)
        {
          if (recovered[i] != '\0')
          {
            fputc(recovered[i], fp);
          }
        }
        fclose(fp);
      }
    }
  }
  dir_close(dir);
}
