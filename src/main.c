#include <stdlib.h>
#include <stdio.h>
#include "ata.h"
#include "fs.h"
#include "vfs.h"
#include "cli.h"
#include "tests.h"

int main(int argc, char** argv)
{
    CLI_DIR = malloc(4096);
    strcpy(CLI_DIR, argv[0]);
    printf("CLI: '%s'\n", CLI_DIR);

    printf("Voyageur File System\n");
    printf("Version 0.1\n");

    ata_init();
    cli_init();

    if (argc == 2)
    {
        char* script_cmd = malloc(strlen(argv[1]) + 128);
        strcpy(script_cmd, "script ");
        strcat(script_cmd, argv[1]);
        cli_execute(script_cmd);
    }

    printf("> ");

    while (TRUE)
    {
        uint32_t buffer_size = 1024;
        char* buffer = malloc(buffer_size);
        getline(&buffer, &buffer_size, stdin);
        cli_execute(buffer);
        printf("> ");
        free(buffer);
    }
    
    return 0;
}