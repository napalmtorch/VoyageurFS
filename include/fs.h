#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"

#define FS_SECTOR_BOOT  0
#define FS_SECTOR_INFO  1
#define FS_SECTOR_BLKS  4

#define FSSTATE_FREE 0
#define FSSTATE_USED 1

#define FSTYPE_NULL 0
#define FSTYPE_DIR  1
#define FSTYPE_FILE 2

typedef struct
{
    uint32_t sector_count;
    uint16_t bytes_per_sector; 
    uint32_t blk_table_start;
    uint32_t blk_table_count;
    uint32_t blk_table_count_max;
    uint32_t blk_table_sector_count;
    uint32_t blk_data_start;
    uint32_t blk_data_sector_count;
    uint32_t blk_data_used;
    uint32_t file_table_start;
    uint32_t file_table_count;
    uint32_t file_table_count_max;
    uint32_t file_table_sector_count;
} PACKED fs_info_t;

typedef struct
{
    uint32_t start;
    uint32_t count;
    uint16_t state;
    uint8_t  padding[6];
} PACKED fs_blkentry_t;

typedef struct
{
    char     name[46];
    uint32_t parent_index;
    uint8_t  status;
    uint8_t  type;
    uint8_t  padding[12];
} PACKED fs_directory_t;

typedef struct
{
    char     name[46];
    uint32_t parent_index;
    uint8_t  status;
    uint8_t  type;
    uint32_t size;
    uint32_t blk_index;
    uint8_t* data;
} PACKED fs_file_t;

void fs_mount();
void fs_format(uint32_t size, bool_t wipe);
void fs_wipe(uint32_t size);

void fs_info_create(uint32_t size);
void fs_info_read();
void fs_info_write();
fs_info_t fs_get_info();

// block table
void            fs_blktable_print();
uint32_t        fs_blktable_sector_from_index(int index);
uint32_t        fs_blktable_offset_from_index(uint32_t sector, int index);
fs_blkentry_t   fs_blktable_read(int index);
void            fs_blktable_write(int index, fs_blkentry_t entry);
fs_blkentry_t   fs_blktable_allocate(uint32_t sectors);
bool_t          fs_blktable_free(fs_blkentry_t entry);
fs_blkentry_t   fs_blktable_nearest(fs_blkentry_t entry);
void            fs_blktable_merge_free();
bool_t          fs_blktable_copy(fs_blkentry_t dest, fs_blkentry_t src);
fs_blkentry_t   fs_blktable_create_entry(uint32_t start, uint32_t count, uint8_t state);
bool_t          fs_blktable_delete_entry(fs_blkentry_t entry);
bool_t          fs_blktable_fill(fs_blkentry_t entry, uint8_t value);
fs_blkentry_t   fs_blktable_at_index(int index);
bool_t          fs_blktable_validate_sector(uint32_t sector);
int             fs_blktable_get_index(fs_blkentry_t entry);
int             fs_blktable_freeindex();

uint32_t        fs_bytes_to_sectors(uint32_t bytes);

// file table
bool_t          fs_root_create(const char* label);
void            fs_filetable_print();
uint32_t        fs_filetable_sector_from_index(int index);
uint32_t        fs_filetable_offset_from_index(uint32_t sector, int index);
fs_directory_t  fs_filetable_read_dir(int index);
fs_file_t       fs_filetable_read_file(int index);
void            fs_filetable_write_dir(int index, fs_directory_t dir);
void            fs_filetable_write_file(int index, fs_file_t file);
fs_directory_t  fs_filetable_create_dir(fs_directory_t dir);
fs_file_t       fs_filetable_create_file(fs_file_t file);
bool_t          fs_filetable_delete_dir(fs_directory_t dir);
bool_t          fs_filetable_delete_file(fs_file_t file);
bool_t          fs_filetable_validate_sector(uint32_t sector);
int             fs_filetable_freeindex();
bool_t          fs_dir_equals(fs_directory_t a, fs_directory_t b);
bool_t          fs_file_equals(fs_file_t a, fs_file_t b);
fs_directory_t  fs_parent_from_path(const char* path);
fs_file_t       fs_get_file_byname(const char* path);
fs_directory_t  fs_get_dir_byname(const char* path);
int             fs_get_file_index(fs_file_t file);
int             fs_get_dir_index(fs_directory_t dir);
char*           fs_get_name_from_path(const char* path);
char*           fs_get_parent_path_from_path(const char* path);
fs_file_t       fs_file_create(const char* path, uint32_t size);
fs_file_t       fs_file_read(const char* path);
bool_t          fs_file_write(const char* path, uint8_t* data, uint32_t len);