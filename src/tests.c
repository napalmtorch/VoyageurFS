#include "tests.h"
#include "fs.h"
#include "vfs.h"

#define FSTEST_DIRS_COUNT 9
const char* fstest_dirs[] = { "/sys/", "/sys/resources/", "/sys/resources/fonts/", "/sys/bin/", "/sys/lib/", 
                           "/users/", "/users/root/", "/users/root/documents/", "/users/root/pictures/" };

#define FSTEST_FILES_COUNT 6
const char* fstest_files[] = { "/sys/resources/fonts/testdoc.txt", "/users/fuckmeintheass.asm", "/users/root/documents/balls.c", "/penisbreath.cs", "/sys/wtf.java", "/sys/lib/YUCK.S" };

void fstest_fail(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    printf("TEST FAILED -  ");
    vprintf(msg, args);
    printf("\n");
    va_end(args);
}

void fstest_ok(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    printf("TEST SUCCESS - ");
    vprintf(msg, args);
    printf("\n");
    va_end(args);
}

void fstest_done(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    printf("TEST DONE -    ");
    vprintf(msg, args);
    printf("\n");
    va_end(args);
}

void fstest_run_all()
{
    fstest_dirs_create();
    fstest_files_create();

    fstest_files_rename();

    fstest_files_delete();
    fstest_dirs_delete();

    fstest_done("COMPLETED ALL TESTS SUCCESSFULLY");
}

void fstest_dirs_create()
{
    // create
    for (int i = 0; i < FSTEST_DIRS_COUNT; i++)
    {
        if (!vfs_create_dir(fstest_dirs[i])) { fstest_fail("Unable to create directory '%s'", fstest_dirs[i]); return; }
        else { fstest_ok("Created directory '%s'", fstest_dirs[i]); }
    }

    // finished test successfully
    fstest_done("CREATE DIRECTORIES");
}


void fstest_files_create()
{
    uint8_t* dummy_data = malloc(4096);

    for (int i = 0; i < FSTEST_FILES_COUNT; i++)
    {
        memset(dummy_data, 'X', 4096);
        if (!vfs_write_text(fstest_files[i], (char*)dummy_data)) { fstest_fail("Unable to create file '%s'", fstest_files[i]); return; }
        else { fstest_ok("Created file '%s'", fstest_files[i]); }
    }
    
    // finished test successfully
    free(dummy_data);
    fstest_done("CREATED FILES");
}

void fstest_dirs_delete()
{
    // until rercursive is implemented, directories must be delete in opposite order of creation
    // otherwise it will be unable to locate the parent of other directories

    // delete
    for (int i = FSTEST_DIRS_COUNT - 1; i >= 0; i--)
    {
        if (!vfs_delete_dir(fstest_dirs[i], FALSE)) { fstest_fail("Unable to delete directory '%s'", fstest_dirs[i]); return; }
        else { fstest_ok("Deleted directory '%s'", fstest_dirs[i]); }
    }

    // finished test successfully
    fstest_done("DELETED DIRECTORIES");
}

void fstest_files_delete()
{
    for (int i = 0; i < FSTEST_FILES_COUNT; i++)
    {
        if (!vfs_delete_file(fstest_files[i])) { fstest_fail("Unable to delete file '%s'", fstest_files[i]); return; }
        else { fstest_ok("Deleted file '%s'", fstest_files[i]); }
    }

    fstest_ok("DELETED FILES");
}

void fstest_dirs_rename()
{
    const char* new_names[FSTEST_DIRS_COUNT] = { "stuff", "retarded", "porn", "NIPPLES", "whatthefuck", "w11sucks", "urmum", "BALLSACK", "ANUS3000" };

    for (int i = FSTEST_DIRS_COUNT - 1; i >= 0; i--)
    {
        if (!vfs_rename_dir(fstest_dirs[i], new_names[i])) { fstest_fail("Unable to rename directory '%s' to '%s'", fstest_dirs[i], new_names[i]); return; }   
        else { fstest_ok("Renamed directory '%s' to '%s'", fstest_dirs[i], new_names[i]); }
    }
    
    fstest_done("RENAMED DIRECTORIES");
}

void fstest_files_rename()
{
    // this must be ran alone, because the values in fstest_dirs will no longer be valid afterwards

    const char* new_names[FSTEST_FILES_COUNT] = { "stupid.cs", "you_dump.asm", "hellNO.rtf", "PENISES.jpg", "lolbruh.bmp", "blue_waffle.png" };

    for (int i = 0; i < FSTEST_FILES_COUNT; i++)
    {
        if (!vfs_rename_file(fstest_files[i], new_names[i])) { fstest_fail("Unable to rename file '%s' to '%s", fstest_files[i], new_names[i]); return; }
        else { fstest_ok("Renamed file '%s' to '%s'", fstest_files[i], new_names[i]); }
    }

    fstest_done("RENAMED FILES");
}