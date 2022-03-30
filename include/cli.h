#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include "util.h"

extern char* CLI_DIR;

typedef void (*cli_cmd_exec_t)(char*, char**, int);

typedef struct
{
    const char*    name;
    const char*    help;
    const char*    usage;
    cli_cmd_exec_t exec;
} PACKED cli_cmd_t;

void cli_init();
void cli_register(cli_cmd_t cmd);
void cli_execute(char* input);
char* cli_getline();

void CMD_METHOD_CLS(char* input, char** argv, int argc);
void CMD_METHOD_EXIT(char* input, char** argv, int argc);
void CMD_METHOD_HELP(char* input, char** argv, int argc);
void CMD_METHOD_BLOCKS(char* input, char** argv, int argc);
void CMD_METHOD_ENTRIES(char* input, char** argv, int argc);
void CMD_METHOD_LS(char* input, char** argv, int argc);
void CMD_METHOD_SCRIPT(char* input, char** argv, int argc);

void CMD_METHOD_FORMAT(char* input, char** argv, int argc);
void CMD_METHOD_NEWIMG(char* input, char** argv, int argc);
void CMD_METHOD_SAVEIMG(char* input, char** argv, int argc);
void CMD_METHOD_LOADIMG(char* input, char** argv, int argc);
void CMD_METHOD_UNLOADIMG(char* input, char** argv, int argc);

void CMD_METHOD_EXISTS(char* input, char** argv, int argc);
void CMD_METHOD_MKDIR(char* input, char** argv, int argc);
void CMD_METHOD_INFILE(char* input, char** argv, int argc);
void CMD_METHOD_INDIR(char* input, char** argv, int argc);
void CMD_METHOD_RM(char* input, char** argv, int argc);
void CMD_METHOD_RMDIR(char* input, char** argv, int argc);
void CMD_METHOD_REN(char* input, char** argv, int argc);
void CMD_METHOD_RENDIR(char* input, char** argv, int argc);
void CMD_METHOD_CP(char* input, char** argv, int argc);
void CMD_METHOD_CPDIR(char* input, char** argv, int argc);
void CMD_METHOD_MV(char* input, char** argv, int argc);
void CMD_METHOD_MVDIR(char* input, char** argv, int argc);


static const cli_cmd_t CMD_CLS          = { "CLS", "Clear the screen", "cls", CMD_METHOD_CLS };
static const cli_cmd_t CMD_EXIT         = { "EXIT", "Exit program", "exit", CMD_METHOD_EXIT };
static const cli_cmd_t CMD_HELP         = { "HELP", "Show list of commands", "help [-u : usage, -s : shortened]", CMD_METHOD_HELP };
static const cli_cmd_t CMD_BLOCKS       = { "BLOCKS", "Show list of sector blocks", "blocks", CMD_METHOD_BLOCKS };
static const cli_cmd_t CMD_ENTRIES      = { "ENTRIES", "Show list of file/directory entries", "entries", CMD_METHOD_ENTRIES };
static const cli_cmd_t CMD_LS           = { "LS", "Show contents of specified directory", "dir [path]", CMD_METHOD_LS };
static const cli_cmd_t CMD_SCRIPT       = { "SCRIPT", "Execute script file", "script [path]", CMD_METHOD_SCRIPT };

static const cli_cmd_t CMD_FORMAT       = { "FORMAT", "Formatted current disk image", "format [-q : quick]", CMD_METHOD_FORMAT };
static const cli_cmd_t CMD_NEWIMG       = { "NEWIMG", "Create a new disk image of specified size", "newimg [bytes]", CMD_METHOD_NEWIMG };
static const cli_cmd_t CMD_SAVEIMG      = { "SAVEIMG", "Save the current disk image to specified path", "saveimg [path]", CMD_METHOD_SAVEIMG };
static const cli_cmd_t CMD_LOADIMG      = { "LOADIMG", "Load disk image from specified path", "loadimg [path]", CMD_METHOD_LOADIMG };
static const cli_cmd_t CMD_UNLOADIMG    = { "UNLOADIMG", "Unload the current disk image", "unloadimg", CMD_METHOD_UNLOADIMG };

static const cli_cmd_t CMD_EXISTS       = { "EXISTS", "Check if file or directory exists", "exists [path]", CMD_METHOD_EXISTS };
static const cli_cmd_t CMD_MKDIR        = { "MKDIR", "Create a new directory", "mkdir [path]", CMD_METHOD_MKDIR };
static const cli_cmd_t CMD_INFILE       = { "INFILE", "Copy file from host to specified path", "infile [dest_path] [src_path]", CMD_METHOD_INFILE };
static const cli_cmd_t CMD_INDIR        = { "INDIR", "Copy directory and contents to specified path", "infile [dest_path] [src_path]", CMD_METHOD_INDIR };
static const cli_cmd_t CMD_RM           = { "RM", "Remove specified file", "rm [path]", CMD_METHOD_RM };
static const cli_cmd_t CMD_RMDIR        = { "RMDIR", "Remove a specified directory", "rmdir [path]", CMD_METHOD_RMDIR };
static const cli_cmd_t CMD_REN          = { "REN", "Rename a specified file", "ren [path] [name]", CMD_METHOD_REN };
static const cli_cmd_t CMD_RENDIR       = { "RENDIR", "Rename a specified directory", "rendir [path] [name]", CMD_METHOD_RENDIR };
static const cli_cmd_t CMD_CP           = { "CP", "Copy a file to specified path", "cp [dest_path] [src_path]", CMD_METHOD_CP };
static const cli_cmd_t CMD_CPDIR        = { "CPDIR", "Copy directory to specified path", "cpdir [dest_path] [src_path]", CMD_METHOD_CPDIR };
static const cli_cmd_t CMD_MV           = { "MV", "Move a file to specified path", "cp [dest_path] [src_path]", CMD_METHOD_MV };
static const cli_cmd_t CMD_MVDIR        = { "MVDIR", "Move directory to specified path", "cpdir [dest_path] [src_path]", CMD_METHOD_MVDIR };