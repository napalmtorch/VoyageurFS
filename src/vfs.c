#include "vfs.h"
#include "fs.h"
#include "ata.h"

vfs_directory_t VFS_NULL_DIR  = { "", "", 0, 0, 0, 0 };
vfs_file_t      VFS_NULL_FILE = { "", "", 0, 0, 0 };

bool_t vfs_dir_exists(const char* path)
{
    fs_directory_t dir = fs_get_dir_byname(path);
    if (dir.type != FSTYPE_DIR) { return FALSE; }
    return TRUE;
}

bool_t vfs_file_exists(const char* path)
{
    fs_file_t dir = fs_get_file_byname(path);
    if (dir.type != FSTYPE_FILE) { return FALSE; }
    return TRUE;
}

vfs_directory_t vfs_dir_info(const char* path)
{
    fs_directory_t dir = fs_get_dir_byname(path);
    if (dir.type != FSTYPE_DIR) { return VFS_NULL_DIR; }
    char* name = fs_get_name_from_path(path);
    char* dir_name = fs_get_parent_path_from_path(path);
    if (name == NULL) { return VFS_NULL_DIR; }

    vfs_directory_t out_dir;
    out_dir.name = malloc(strlen(name) + 1);
    strcpy(out_dir.name, name);
    
    // copy name and path
    if (dir_name != NULL)
    {
        out_dir.path = malloc(strlen(dir_name) + 1);
        strcpy(out_dir.path, dir_name);
    }
    else { out_dir.path = malloc(1); out_dir.path[0] = 0; }

    out_dir.status = (VFSSTATUS)dir.status;

    // not yet implemented
    out_dir.size = 0;
    out_dir.sub_dirs = 0;
    out_dir.sub_files = 0;

    return out_dir;
}

vfs_file_t vfs_file_info(const char* path)
{
    fs_file_t file = fs_get_file_byname(path);
    if (file.type != FSTYPE_FILE) { return VFS_NULL_FILE; }
    char* name = fs_get_name_from_path(path);
    char* dir_name = fs_get_parent_path_from_path(path);
    if (name == NULL) { return VFS_NULL_FILE; }

    vfs_file_t out_file;
    out_file.name = malloc(strlen(name) + 1);
    strcpy(out_file.name, name);
    
    // copy name and path
    if (dir_name != NULL)
    {
        out_file.path = malloc(strlen(dir_name) + 1);
        strcpy(out_file.path, dir_name);
    }
    else { out_file.path = malloc(1); out_file.path[0] = 0; }

    out_file.status = (VFSSTATUS)file.status;
    out_file.size = file.size;
    out_file.data = NULL;
    return out_file;
}

uint32_t vfs_count_dirs(const char* path)
{
    if (!vfs_dir_exists(path)) { printf("Unable to count directories in '%s'\n", path); return 0; }

    fs_directory_t dir = fs_get_dir_byname(path);
    if (dir.type != FSTYPE_DIR) { printf("Error while counting directories in '%s'\n", path); return 0; }
    int index = fs_get_dir_index(dir);

    uint32_t dirs_count = 0;
    uint8_t* secdata = malloc(ATA_SECTOR_SIZE);
    for (uint32_t sec = 0; sec < fs_get_info().file_table_sector_count; sec++)
    {
        ata_read(fs_get_info().file_table_start + sec, 1, secdata);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_directory_t))
        {
            fs_directory_t* tempdir = (fs_directory_t*)(secdata + i);
            if (tempdir->type == FSTYPE_DIR && tempdir->parent_index == index) { dirs_count++; }
        }
    }
    free(secdata);
    return dirs_count;
}

uint32_t vfs_count_files(const char* path)
{
    if (!vfs_dir_exists(path)) { printf("Unable to count files in '%s'\n", path); return 0; }

    fs_directory_t dir = fs_get_dir_byname(path);
    if (dir.type != FSTYPE_DIR) { printf("Error while counting files in '%s'\n", path); return 0; }
    int index = fs_get_dir_index(dir);

    uint32_t dirs_count = 0;
    uint8_t* secdata = malloc(ATA_SECTOR_SIZE);
    for (uint32_t sec = 0; sec < fs_get_info().file_table_sector_count; sec++)
    {
        ata_read(fs_get_info().file_table_start + sec, 1, secdata);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_file_t))
        {
            fs_file_t* tempfile = (fs_file_t*)(secdata + i);
            if (tempfile->type == FSTYPE_FILE && tempfile->parent_index == index) { dirs_count++; }
        }
    }
    free(secdata);
    return dirs_count;
}

char** vfs_get_dirs(const char* path, int* count)
{
    if (!vfs_dir_exists(path)) { printf("Unable to locate directory '%s'\n", path); return NULL; }

    fs_directory_t dir = fs_get_dir_byname(path);
    if (dir.type != FSTYPE_DIR) { printf("Error while locating directory '%s'\n", path); return NULL; }
    int index = fs_get_dir_index(dir);

    uint32_t dir_count = vfs_count_dirs(path);
    char** output = (char**)malloc(sizeof(char*) * dir_count);
    int output_index = 0;

    uint8_t* secdata = malloc(ATA_SECTOR_SIZE);
    for (uint32_t sec = 0; sec < fs_get_info().file_table_sector_count; sec++)
    {
        ata_read(fs_get_info().file_table_start + sec, 1, secdata);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_directory_t))
        {
            fs_directory_t* tempdir = (fs_directory_t*)(secdata + i);
            if (tempdir->type == FSTYPE_DIR && tempdir->parent_index == index)
            {
                char* dirname = malloc(strlen(tempdir->name) + 1);
                strcpy(dirname, tempdir->name);
                output[output_index] = dirname;
                output_index++;
            }
        }
    }
    free(secdata);
    *count = output_index;
    return output;
}

char** vfs_get_files(const char* path, int* count)
{
    if (!vfs_dir_exists(path)) { printf("Unable to locate directory '%s'\n", path); return NULL; }

    fs_directory_t dir = fs_get_dir_byname(path);
    if (dir.type != FSTYPE_DIR) { printf("Error while locating directory '%s'\n", path); return NULL; }
    int index = fs_get_dir_index(dir);

    uint32_t file_count = vfs_count_files(path);
    char** output = (char**)malloc(sizeof(char*) * file_count);
    int output_index = 0;

    uint8_t* secdata = malloc(ATA_SECTOR_SIZE);
    for (uint32_t sec = 0; sec < fs_get_info().file_table_sector_count; sec++)
    {
        ata_read(fs_get_info().file_table_start + sec, 1, secdata);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_directory_t))
        {
            fs_directory_t* tempfile = (fs_directory_t*)(secdata + i);
            if (tempfile->type == FSTYPE_FILE && tempfile->parent_index == index)
            {
                char* fname = malloc(strlen(tempfile->name) + 1);
                strcpy(fname, tempfile->name);
                output[output_index] = fname;
                output_index++;
            }
        }
    }
    free(secdata);
    *count = output_index;
    return output;
}

char** vfs_read_lines(const char* path)
{
    return NULL;
}

char* vfs_read_text(const char* path)
{
    fs_file_t file = fs_file_read(path);
    if (file.type != FSTYPE_FILE) { return NULL; }
    return (char*)file.data;
}

uint8_t* vfs_read_bytes(const char* path)
{
    fs_file_t file = fs_file_read(path);
    if (file.type != FSTYPE_FILE) { return NULL; }
    return file.data;
}

bool_t vfs_write_lines(const char* path, char** lines, int line_count)
{
    return FALSE;
}

bool_t vfs_write_text(const char* path, char* text)
{
    return fs_file_write(path, (uint8_t*)text, strlen(text));
}

bool_t vfs_write_bytes(const char* path, uint8_t* data, uint32_t size)
{
    return fs_file_write(path, data, size);
}

bool_t vfs_create_dir(const char* path)
{
    fs_directory_t parent = fs_parent_from_path(path);
    if (parent.type != FSTYPE_DIR) { return FALSE; }

    fs_directory_t new_dir;
    char* name = fs_get_name_from_path(path);
    strcpy(new_dir.name, name);
    free(name);
    new_dir.parent_index = fs_get_dir_index(parent);
    new_dir.status = 0x00;
    new_dir.type   = FSTYPE_DIR;
    memset(new_dir.padding, 0, sizeof(new_dir.padding));
    fs_directory_t created = fs_filetable_create_dir(new_dir);
    if (created.type != FSTYPE_DIR) { return FALSE; }
    return TRUE;
    
}

bool_t vfs_rename_dir(const char* path, const char* name)
{
    fs_directory_t dir = fs_get_dir_byname(path);
    if (dir.type != FSTYPE_DIR) { return FALSE; }
    int index = fs_get_dir_index(dir);

    strcpy(dir.name, name);
    fs_filetable_write_dir(index, dir);
    return TRUE;
}

bool_t vfs_rename_file(const char* path, const char* name)
{
    fs_file_t file = fs_get_file_byname(path);
    if (file.type != FSTYPE_FILE) { return FALSE; }
    int index = fs_get_file_index(file);

    strcpy(file.name, name);
    fs_filetable_write_file(index, file);
    return TRUE;
}

bool_t vfs_delete_dir(const char* path, bool_t recursive)
{
    fs_directory_t dir = fs_get_dir_byname(path);
    if (dir.type != FSTYPE_DIR) { return FALSE; }

    if (!recursive) { fs_filetable_delete_dir(dir); return TRUE; }
    else
    {
        printf("Recursive delete not yet implemented\n");
        return FALSE;
    }
}

bool_t vfs_delete_file(const char* path)
{
    fs_file_t file = fs_get_file_byname(path);
    if (file.type != FSTYPE_FILE) { return FALSE; }

    if (!fs_blktable_free(fs_blktable_at_index(file.blk_index))) { return FALSE; }
    return fs_filetable_delete_file(file);
}

bool_t vfs_copy_dir(const char* dest, const char* src, bool_t recursive)
{
    if (recursive) { printf("Recursive copy not yet implemented\n"); return FALSE; }

    fs_directory_t dir_src = fs_get_dir_byname(src);
    if (dir_src.type != FSTYPE_DIR) { return FALSE; }

    fs_directory_t dir_dest_parent = fs_parent_from_path(dest);
    if (dir_dest_parent.type != FSTYPE_DIR) { return FALSE; }

    char* name = fs_get_name_from_path(dest);
    fs_directory_t dir_dest;
    strcpy(dir_dest.name, name);
    dir_dest.parent_index = fs_get_dir_index(dir_dest_parent);
    dir_dest.status = 0x00;
    dir_dest.type = FSTYPE_DIR;
    return fs_filetable_create_dir(dir_dest).type == FSTYPE_DIR;
}

bool_t vfs_copy_file(const char* dest, const char* src)
{
    fs_file_t file_src = fs_file_read(src);
    if (file_src.type != FSTYPE_FILE) { return FALSE; }

    fs_directory_t file_dest_parent = fs_parent_from_path(dest);
    if (file_dest_parent.type != FSTYPE_DIR) { return FALSE; }

    char* name = fs_get_name_from_path(dest);
    fs_file_t file_dest;
    strcpy(file_dest.name, name);
    file_dest.parent_index = fs_get_dir_index(file_dest_parent);
    file_dest.status = 0x00;
    file_dest.type = FSTYPE_FILE;
    file_dest.size = file_src.size;
    
    fs_blkentry_t new_blk = fs_blktable_allocate(fs_bytes_to_sectors(file_dest.size));
    fs_blktable_copy(new_blk, fs_blktable_at_index(file_src.blk_index));

    file_dest.blk_index = fs_blktable_get_index(new_blk);
    return fs_filetable_create_file(file_dest).type == FSTYPE_FILE;
}

bool_t vfs_move_dir(const char* dest, const char* src)
{
    return FALSE;
}

bool_t vfs_move_file(const char* dest, const char* src)
{
    return FALSE;
}
