#include "cli.h"
#include "fs.h"
#include "vfs.h"
#include "ata.h"

char* CLI_DIR = NULL;

cli_cmd_t** cmd_list;
uint32_t   cmd_count;
uint32_t   cmd_count_max;

void cli_init()
{
    cmd_count_max = 256;
    cmd_count     = 0;
    cmd_list = malloc(sizeof(cli_cmd_t*) * cmd_count_max);
    memset(cmd_list, 0, sizeof(cli_cmd_t*) * cmd_count_max);

    cli_register(CMD_CLS);
    cli_register(CMD_EXIT);
    cli_register(CMD_HELP);
    cli_register(CMD_BLOCKS);
    cli_register(CMD_ENTRIES);
    cli_register(CMD_LS);
    cli_register(CMD_SCRIPT);

    cli_register(CMD_FORMAT);
    cli_register(CMD_NEWIMG);
    cli_register(CMD_SAVEIMG);
    cli_register(CMD_LOADIMG);
    cli_register(CMD_UNLOADIMG);

    cli_register(CMD_EXISTS);
    cli_register(CMD_MKDIR);
    cli_register(CMD_INFILE);
    cli_register(CMD_INDIR);
    cli_register(CMD_RM);
    cli_register(CMD_RMDIR);
    cli_register(CMD_REN);
    cli_register(CMD_RENDIR);
    cli_register(CMD_CP);
    cli_register(CMD_CPDIR);
    cli_register(CMD_MV);
    cli_register(CMD_MVDIR);
}

void cli_register(cli_cmd_t cmd)
{
    if (cmd_count >= cmd_count_max) { printf("Maximum amount of registered commands reached"); return; }
    cmd_list[cmd_count] = malloc(sizeof(cli_cmd_t));
    memcpy(cmd_list[cmd_count], &cmd, sizeof(cli_cmd_t));
    cmd_count++;
}

void cli_execute(char* input)
{
    if (input == NULL) { return; }
    if (strlen(input) == 0) { return; }
    if (input[strlen(input) - 1] == '\n') { input[strlen(input) - 1] = 0; }
    if (strlen(input) == 0) { return; }

    int args_count = 0;
    char** args = strsplit(input, ' ', &args_count);
    if (args_count == 0 || args == NULL) { return; }

    char* cmd = malloc(strlen(input) + 1);
    strcpy(cmd, args[0]);
    for (uint32_t i = 0; i < strlen(cmd); i++) { cmd[i] = toupper(cmd[i]); }    

    for (uint32_t i = 0; i < cmd_count_max; i++)
    {
        if (cmd_list[i] == NULL) { continue; }
        if (cmd_list[i]->exec == NULL) { continue; }
        
        if (!strcmp(cmd_list[i]->name, cmd))
        {
            cmd_list[i]->exec(input, args, args_count);
            freearray(args, args_count);
            if (cmd != NULL) { free(cmd); }
            return;
        }
    }

    freearray(args, args_count);
    if (cmd != NULL) { free(cmd); }
    printf("Invalid command\n");
}

char* cli_getline() 
{
    char * line = malloc(256), *linep = line;
    size_t lenmax = 256, len = lenmax;
    int c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;

        if(--len == 0) {
            len = lenmax;
            char * linen = realloc(linep, lenmax *= 2);

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}

void _FS_INFILE(char* dest, char* src)
{
    char* path_dest = dest;
    char* path_src  = src;

    FILE* fileptr = fopen(path_src, "rb");
    if (fileptr == NULL) { printf("Unable to locate file '%s'\n", path_src); return; }
    fseek(fileptr, 0, SEEK_END);
    size_t size = ftell(fileptr);
    fseek(fileptr, 0, SEEK_SET);
    if (size == 0) { printf("Unable to locate file '%s'\n", path_src); return; }

    uint8_t* fdata = malloc(size);
    fread(fdata, size, 1, fileptr);

    bool_t success = vfs_write_bytes(path_dest, fdata, size);

    fclose(fileptr);
    free(fdata);
    if (success) { printf("Copied file '%s' from host to '%s' on disk\n", path_src, path_dest); }
    else { printf("Unable to copy file '%s' from host to '%s' on disk\n", path_src, path_dest); }
}

void CMD_METHOD_CLS(char* input, char** argv, int argc)
{
    system("clear");   
}

void CMD_METHOD_EXIT(char* input, char** argv, int argc)
{
    exit(0);
}

void CMD_METHOD_HELP(char* input, char** argv, int argc)
{
    for (uint32_t i = 0; i < cmd_count; i++)
    {
        char temp[128];
        memset(temp, 0, 128);
        strcpy(temp, cmd_list[i]->name);
        while (strlen(temp) < 24) { strcat(temp, " "); }
        printf("- %s %s\n", temp, cmd_list[i]->help);
        
    }
}

void CMD_METHOD_BLOCKS(char* input, char** argv, int argc)
{
    fs_blktable_print();
}

void CMD_METHOD_ENTRIES(char* input, char** argv, int argc)
{
    fs_filetable_print();
}

void CMD_METHOD_LS(char* input, char** argv, int argc)
{
    char* path = (char*)(input + 3);
    printf("Listing contents of %s\n", path);

    int dirs_count = 0;
    char** dirs = vfs_get_dirs(path, &dirs_count);
    for (int i = 0; i < dirs_count; i++) { printf("- %s\n", dirs[i]); }
    freearray(dirs, dirs_count);

    int files_count = 0;
    char** files = vfs_get_files(path, &files_count);
    for (int i = 0; i < files_count; i++) { printf("%s\n", files[i]); }
    freearray(files, files_count);
}

void CMD_METHOD_SCRIPT(char* input, char** argv, int argc)
{
    char* path = (char*)(input + 7);

    FILE* fileptr = fopen(path, "rb");
    if (fileptr == NULL) { printf("Unable to locate script file '%s'\n", path); return; }
    fseek(fileptr, 0, SEEK_END);
    size_t size = ftell(fileptr);
    fseek(fileptr, 0, SEEK_SET);
    if (size == 0) { printf("Unable to locate script file '%s'\n", path); return; }

    char* fdata = malloc(size + 8);
    memset(fdata, 0, size + 8);
    fread(fdata, size, 1, fileptr);
    fclose(fileptr);

    printf("Loaded script file '%s'\n", path);

    if (fdata == NULL) { return; }
    int lines_count = 0;
    char** lines = strsplit(fdata, '\n', &lines_count);
    if (lines_count == 0 || lines == NULL) { return; }

    for (int i = 0; i < lines_count; i++)
    {
        if (lines[i] == NULL) { continue; }
        if (strlen(lines[i]) == 0) { continue; }
        int args_count = 0;
        char** args = strsplit(lines[i], ' ', &args_count);
        if (args_count == 0 || args == NULL) { continue; }

        char* cmd = malloc(strlen(lines[i]) + 1);
        strcpy(cmd, args[0]);
        for (uint32_t k = 0; k < strlen(cmd); k++) { cmd[k] = toupper(cmd[k]); }    

        bool_t cmd_error = TRUE;
        for (uint32_t j = 0; j < cmd_count_max; j++)
        {
            if (cmd_list[j] == NULL) { continue; }
            if (cmd_list[j]->exec == NULL) { continue; }
            
            if (!strcmp(cmd_list[j]->name, cmd))
            {
                cmd_list[j]->exec(lines[i], args, args_count);
                cmd_error = FALSE;
                break;
            }
        }

        if (args != NULL) { freearray(args, args_count); }
        if (cmd != NULL) { free(cmd); }

        if (cmd_error) { printf("Invalid command in script\n");}
    }

    freearray(lines, lines_count);
    free(fdata);
}

void CMD_METHOD_FORMAT(char* input, char** argv, int argc)
{
    fs_format(ata_get_disk_size(), TRUE);
    fs_mount();
}

void CMD_METHOD_NEWIMG(char* input, char** argv, int argc)
{
    char* size_str = (char*)(input + 7);
    long size = atol(size_str);
    ata_create(size);
}

void CMD_METHOD_SAVEIMG(char* input, char** argv, int argc)
{
    char* path = (char*)(input + 8);
    ata_save_file(path);   
}

void CMD_METHOD_LOADIMG(char* input, char** argv, int argc)
{
    char* path = (char*)(input + 8);
    ata_load_file(path);
    fs_mount();
}

void CMD_METHOD_UNLOADIMG(char* input, char** argv, int argc)
{
    ata_unload();
}

void CMD_METHOD_EXISTS(char* input, char** argv, int argc)
{
    char* path = (char*)(input + 7);
    if (!vfs_file_exists(path))
    {
        if (!vfs_dir_exists(path)) { printf("Unable to locate '%s'\n", path); return; }
    }
    printf("Located '%s'\n", path);
}

void CMD_METHOD_MKDIR(char* input, char** argv, int argc)
{
    char* path = (char*)(input + 6);
    if (!vfs_create_dir(path)) { printf("Unable to create directory '%s'\n", path); }
    else { printf("Created directory '%s'\n", path); }
}

void CMD_METHOD_INFILE(char* input, char** argv, int argc)
{
    if (argc < 3) { printf("Invalid arguments\n"); return; }
    if (!strcmp(argv[1], "-d"))
    {
        if (argc < 4) { printf("Invalid arguments\n"); return; }
        char* dest_dir = argv[2];
        char* src_dir  = argv[3];

        DIR *d;
        struct dirent *dir;
        d = opendir(src_dir);
        if (d) 
        {
            while ((dir = readdir(d)) != NULL) 
            {
                if (dir->d_type == DT_DIR) { continue; }

                uint32_t src_size = strlen(src_dir) + strlen(dir->d_name) + 32;
                uint32_t dest_size = strlen(dest_dir) + strlen(dir->d_name) + 32;
                char* src_full = malloc(src_size);
                char* dest_full = malloc(dest_size);
                memset(src_full, 0, src_size);
                memset(dest_full, 0, dest_size);

                strcat(src_full, src_dir);
                strcat(src_full, "/");

                strcat(dest_full, dest_dir);
                strcat(dest_full, "/");
            
                strcat(src_full, dir->d_name);
                strcat(dest_full, dir->d_name);
                printf("FILE: '%s', '%s'\n", dest_full, src_full);

                _FS_INFILE(dest_full, src_full);

                free(src_full);
                free(dest_full);
            }
            closedir(d);
        }        
    }
    else { _FS_INFILE(argv[1], argv[2]); }
}

void CMD_METHOD_INDIR(char* input, char** argv, int argc)
{
    printf("Importing entire directories at once not yet supported\n");
}

void CMD_METHOD_RM(char* input, char** argv, int argc)
{
    char* path = (char*)(input + 3);
    if (vfs_delete_file(path)) { printf("Deleted file '%s'\n", path); }
    else { printf("Unable to locate file '%s'\n", path); }
}

void CMD_METHOD_RMDIR(char* input, char** argv, int argc)
{
    printf("Deleting entire directories at once not yet supported\n");   
}

void CMD_METHOD_REN(char* input, char** argv, int argc)
{
    char* path = argv[1];
    char* name = argv[2];

    if (vfs_rename_file(path, name)) { printf("Renamed file '%s' to '%s'\n", path, name); }
    else { printf("Unable to rename file '%s' to '%s'\n", path, name); }
}

void CMD_METHOD_RENDIR(char* input, char** argv, int argc)
{
    char* path = argv[1];
    char* name = argv[2];

    if (vfs_rename_dir(path, name)) { printf("Renamed directory '%s' to '%s'\n", path, name); }
    else { printf("Unable to rename directory '%s' to '%s'\n", path, name); }
}

void CMD_METHOD_CP(char* input, char** argv, int argc)
{
    char* path_dest = argv[1];
    char* path_src  = argv[2];

    if (vfs_copy_file(path_dest, path_src)) { printf("Copied file '%s' to '%s'\n", path_src, path_dest); }
    else { printf("Unable to copy file '%s' to '%s'\n", path_src, path_dest); }
}

void CMD_METHOD_CPDIR(char* input, char** argv, int argc)
{
    printf("Copying entire directories not yet implemented\n");
}

void CMD_METHOD_MV(char* input, char** argv, int argc)
{
    char* path_dest = argv[1];
    char* path_src  = argv[2];

    if (!vfs_copy_file(path_dest, path_src)) { printf("Unable to copy file '%s' to '%s'\n", path_src, path_dest); return; }
    if (!vfs_delete_file(path_src)) { printf("Unable to delete source file '%s' while moving to '%s'\n", path_src, path_dest); return; }
    printf("Copied file '%s' to '%s'\n", path_src, path_dest);
}

void CMD_METHOD_MVDIR(char* input, char** argv, int argc)
{
    printf("Moving entire directories not yet implemented\n");
}