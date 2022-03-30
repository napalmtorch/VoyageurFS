#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "util.h"

typedef enum
{
    VFSSTATUS_DEFAULT = 0x00,
    VFSSTATUS_ROOT    = 0xFF,
} VFSSTATUS;

typedef struct
{
    char* name;
    char* path;
    VFSSTATUS   status;
    uint32_t    size;
    uint32_t    sub_dirs;
    uint32_t    sub_files;
} PACKED vfs_directory_t;

typedef struct
{
    char* name;
    char* path;
    VFSSTATUS   status;
    uint32_t    size;
    uint8_t*    data;
} PACKED vfs_file_t;

bool_t          vfs_dir_exists(const char* path);
bool_t          vfs_file_exists(const char* path);
vfs_directory_t vfs_dir_info(const char* path);
vfs_file_t      vfs_file_info(const char* path);
uint32_t        vfs_count_dirs(const char* path);
uint32_t        vfs_count_files(const char* path);
char**          vfs_get_dirs(const char* path, int* count);
char**          vfs_get_files(const char* path, int* count);
char**          vfs_read_lines(const char* path);
char*           vfs_read_text(const char* path);
uint8_t*        vfs_read_bytes(const char* path);
bool_t          vfs_write_lines(const char* path, char** lines, int line_count);
bool_t          vfs_write_text(const char* path, char* text);
bool_t          vfs_write_bytes(const char* path, uint8_t* data, uint32_t size);
bool_t          vfs_create_dir(const char* path);
bool_t          vfs_rename_dir(const char* path, const char* name);
bool_t          vfs_rename_file(const char* path, const char* name);
bool_t          vfs_delete_dir(const char* path, bool_t recursive);
bool_t          vfs_delete_file(const char* path);
bool_t          vfs_copy_dir(const char* dest, const char* src, bool_t recursive);
bool_t          vfs_copy_file(const char* dest, const char* src);
bool_t          vfs_move_dir(const char* dest, const char* src);
bool_t          vfs_move_file(const char* dest, const char* src);

