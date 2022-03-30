#include "fs.h"
#include "ata.h"

// null structures
fs_blkentry_t  NULL_BLKENTRY = { 0, 0, 0, { 0, 0, 0, 0, 0, 0 } };
fs_directory_t NULL_DIR      = { "", 0, 0, 0, { 0 } };
fs_file_t      NULL_FILE     = { "", 0, 0, 0, 0, 0, NULL };

// file system information
fs_info_t      fs_info;
fs_blkentry_t  fs_blk_mass;
fs_blkentry_t  fs_blk_files;
fs_directory_t fs_rootdir;

// mount file system from disk image
void fs_mount()
{
    fs_info_read();
    fs_blk_mass = fs_blktable_read(0);
    fs_blk_files = fs_blktable_read(1);
    fs_rootdir = fs_filetable_read_dir(0);
    printf("Mounted file system\n");
}

// format disk of specified size to file system
void fs_format(uint32_t size, bool_t wipe)
{
    printf("Fomatting disk...\n");
    if (wipe) { fs_wipe(size); }

    // generate info block
    fs_info_create(size);
    fs_info_read();

    // create mass block entry
    fs_blk_mass.start = fs_info.blk_data_start;
    fs_blk_mass.count = fs_info.blk_data_sector_count;
    fs_blk_mass.state = FSSTATE_FREE;
    memset(fs_blk_mass.padding, 0, sizeof(fs_blk_mass.padding));
    fs_blktable_write(0, fs_blk_mass);
    fs_blk_mass = fs_blktable_read(0);
    printf("Created mass block: START: %d, STATE = 0x%02x, COUNT = %d\n", fs_blk_mass.start, fs_blk_mass.state, fs_blk_mass.count);

    // create files block entry and update info
    fs_info_read();
    fs_info.file_table_count_max = 32768;
    fs_info.file_table_count     = 0;
    fs_info.file_table_sector_count = (fs_info.file_table_count_max * sizeof(fs_file_t)) / ATA_SECTOR_SIZE;
    fs_info_write();
    fs_blk_files = fs_blktable_allocate(fs_info.file_table_sector_count);
    fs_info.file_table_start = fs_blk_files.start;
    fs_info_write();

    // create root directory
    fs_root_create("VOS");

    // finished
    printf("Finished formatting disk\n");

}

// fill disk with zeros
void fs_wipe(uint32_t size)
{
    printf("Started wiping disk...\n");
    uint32_t sectors = size / ATA_SECTOR_SIZE;
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    memset(data, 0, ATA_SECTOR_SIZE);

    for (uint64_t i = 0; i < sectors; i++) { ata_write(i, 1, data); }

    printf("Finished wiping disk\n");
    free(data);
}

fs_info_t fs_get_info()
{
    fs_info_t output;
    memcpy(&output, &fs_info, sizeof(fs_info_t));
    return output;
}

// create new info block
void fs_info_create(uint32_t size)
{
    // disk info
    fs_info.sector_count        = size / ATA_SECTOR_SIZE;
    fs_info.bytes_per_sector    = ATA_SECTOR_SIZE;

    // block table
    fs_info.blk_table_start     = FS_SECTOR_BLKS;
    fs_info.blk_table_count     = 0;
    fs_info.blk_table_count_max = 16384;
    fs_info.blk_table_sector_count = (fs_info.blk_table_count_max * sizeof(fs_blkentry_t)) / ATA_SECTOR_SIZE;

    // block data
    fs_info.blk_data_start = fs_info.blk_table_start + fs_info.blk_table_sector_count + 4;
    fs_info.blk_data_sector_count = fs_info.sector_count - (fs_info.blk_table_sector_count + 8);
    fs_info.blk_data_used = 0;

    // write to disk
    fs_info_write();

    // finished
    printf("Created new info block\n");
}

// read info block from disk
void fs_info_read()
{
    uint8_t* sec = malloc(ATA_SECTOR_SIZE);
    ata_read(FS_SECTOR_INFO, 1, sec);
    memcpy(&fs_info, sec, sizeof(fs_info_t));
    free(sec);
}

// write info block to disk
void fs_info_write()
{
    uint8_t* sec = malloc(ATA_SECTOR_SIZE);
    memset(sec, 0, ATA_SECTOR_SIZE);
    memcpy(sec, &fs_info, sizeof(fs_info_t));
    ata_write(FS_SECTOR_INFO, 1, sec);
    free(sec);
}

void fs_blktable_print()
{
    printf("PRINTING BLOCK TABLE: \n");

    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            fs_blkentry_t* entry = (fs_blkentry_t*)(data + i);
            if (entry->start == 0) { index++; continue; }
            printf("INDEX: 0x%08x START: 0x%08x COUNT: 0x%08x STATE: 0x%02x\n", index, entry->start, entry->count, entry->state);
            index++;
        }
    }

    printf("\n");
    free(data);
}

// get sector from block entry index
uint32_t fs_blktable_sector_from_index(int index)
{
    if (index == 0) { index = 1; }
    uint32_t sec = FS_SECTOR_BLKS;
    uint32_t offset_bytes = (index * sizeof(fs_blkentry_t));
    sec += (offset_bytes / ATA_SECTOR_SIZE);
    return sec;
}

// get sector offset from sector and block entry index
uint32_t fs_blktable_offset_from_index(uint32_t sector, int index)
{
    uint32_t offset_bytes = (index * sizeof(fs_blkentry_t));
    uint32_t val = offset_bytes % 512;
    return val;
}

// read block entry from disk
fs_blkentry_t fs_blktable_read(int index)
{
    fs_info_read();
    if (index < 0 || index >= fs_info.blk_table_count_max) { printf("Invalid index while reading from block table\n"); return NULL_BLKENTRY; }
    uint32_t sector = fs_blktable_sector_from_index(index);
    uint32_t offset = fs_blktable_offset_from_index(sector, index);
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    ata_read(sector, 1, data);
    fs_blkentry_t* entry = (fs_blkentry_t*)(data + offset);
    fs_blkentry_t output = { entry->start, entry->count, entry->state, { 0 } };
    free(data);
    return output;
}

// write block entry to disk 
void fs_blktable_write(int index, fs_blkentry_t entry)
{
    fs_info_read();
    if (index < 0 || index >= fs_info.blk_table_count_max) { printf("Invalid index while reading from block table\n"); return; }
    uint32_t sector = fs_blktable_sector_from_index(index);
    uint32_t offset = fs_blktable_offset_from_index(sector, index);
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    ata_read(sector, 1, data);
    fs_blkentry_t* temp = (fs_blkentry_t*)(data + offset);
    temp->start         = entry.start;
    temp->count         = entry.count;
    temp->state         = entry.state;
    memcpy(temp->padding, entry.padding, sizeof(entry.padding));
    ata_write(sector, 1, data);
    free(data);
}

// allocate new block entry
fs_blkentry_t fs_blktable_allocate(uint32_t sectors)
{
    if (sectors == 0) { return NULL_BLKENTRY; }

    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            fs_blkentry_t* entry = (fs_blkentry_t*)(data + i);
            if (!fs_blktable_validate_sector(entry->start)) { index++; continue; }
            if (entry->count == sectors && entry->state == FSSTATE_FREE)
            {
                fs_blkentry_t output;
                entry->state = FSSTATE_USED;
                memcpy(&output, entry, sizeof(fs_blkentry_t));
                fs_blktable_write(index, output);
                printf("Allocated block: START: 0x%08x, STATE = 0x%02x, COUNT = 0x%08x\n", output.start, output.state, output.count);
                free(data);
                return output;
            }

            index++;
        }
    }

    ata_read(fs_info.blk_table_start, 1, data);
    fs_blkentry_t* mass = (fs_blkentry_t*)data;

    mass->start += sectors;
    mass->count -= sectors;
    mass->state  = FSSTATE_FREE;
    ata_write(fs_info.blk_table_start, 1, data);
    fs_blkentry_t output = fs_blktable_create_entry(mass->start - sectors, sectors, FSSTATE_USED);
    printf("Allocated block: START: 0x%08x, STATE = 0x%02x, COUNT = 0x%08x\n", output.start, output.state, output.count);
    free(data);
    return output;
}

// free existing block entry
bool_t fs_blktable_free(fs_blkentry_t entry)
{
    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            fs_blkentry_t* temp = (fs_blkentry_t*)(data + i);
            if (temp->start == 0) { index++; continue; }
            if (temp->start == entry.start && temp->count == entry.count && temp->state == entry.state)
            {
                temp->state = FSSTATE_FREE;
                ata_write(fs_info.blk_table_start + sec, 1, data);
                printf("Freed block: START: 0x%08x, STATE = 0x%02x, COUNT = 0x%08x\n", temp->start, temp->state, temp->count);
                fs_blktable_merge_free();
                return TRUE;
            }
        }
        index++;
    }

    printf("Unable to free block START: %d, STATE = 0x%02x, COUNT = %d\n", entry.start, entry.state, entry.count);
    return FALSE;
}

fs_blkentry_t fs_blktable_nearest(fs_blkentry_t entry)
{
    if (entry.start == 0) { return NULL_BLKENTRY; }
    if (entry.count == 0) { return NULL_BLKENTRY; }

    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            if (index == 0) { index++; continue; }
            fs_blkentry_t* temp = (fs_blkentry_t*)(data + i);
            if (temp->start == 0 || temp->count == 0) { index++; continue; }

            if ((temp->start + temp->count == entry.start && entry.state == FSSTATE_FREE) ||
                (entry.start - entry.count == temp->start && entry.state == FSSTATE_FREE))
                {
                    fs_blkentry_t output;
                    memcpy(&output, temp, sizeof(fs_blkentry_t));
                    free(data);
                    return output;
                }

            index++;
        }
    }

    return NULL_BLKENTRY;
}

void fs_blktable_merge_free()
{
    fs_info_read();
    fs_blkentry_t mass = fs_blktable_nearest(fs_blktable_at_index(0));
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            fs_blkentry_t* temp = (fs_blkentry_t*)(data + i);
            if (index == 0) { index++; continue; }
            if (temp->start == 0 || temp->count == 0) { index++; continue; }
            
            if (temp->state == FSSTATE_FREE)
            {
                fs_blkentry_t nearest = fs_blktable_nearest(*temp);
                if (nearest.start > 0 && nearest.count > 0 && nearest.start != temp->start && nearest.start != mass.start && nearest.state == FSSTATE_FREE)
                {
                    printf("TEMP: 0x%08x, NEAREST: 0x%08x\n", temp->start, nearest.start);
                    if (temp->start > nearest.start) { temp->start = nearest.start; }
                    temp->count += nearest.count;
                    ata_write(fs_info.blk_table_start + sec, 1, data);
                    if (!fs_blktable_delete_entry(nearest)) { printf("Error deleting entry while merging\n"); return; }
                    ata_read(fs_info.blk_table_start + sec, 1, data);
                }
            }
            index++;
        }
    }
    fs_info_write();

    mass = fs_blktable_at_index(0);

    fs_info_read();
    index = 0;
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            fs_blkentry_t* temp = (fs_blkentry_t*)(data + i);
            if (index == 0) { index++; continue; }
            if (temp->start == 0 || temp->count == 0) { index++; continue; }

            if (temp->start + temp->count == mass.start && temp->state == FSSTATE_FREE)
            {
                printf("MASS: START = 0x%08x, COUNT = 0x%08x, STATE = 0x%02x\n", mass.start, mass.count, mass.state);
                printf("TEMP: START = 0x%08x, COUNT = 0x%08x, STATE = 0x%02x\n", temp->start, temp->count, temp->state);
                mass.start = temp->start;
                mass.count += temp->count;
                mass.state = FSSTATE_FREE;
                fs_blktable_write(0, mass);
                ata_read(fs_info.blk_table_start + sec, 1, data);
                memset(temp, 0, sizeof(fs_blkentry_t));
                ata_write(fs_info.blk_table_start + sec, 1, data);
                fs_info_write();
                break;
            }
            index++;
        }
    }

    free(data);
    fs_info_write();
}

bool_t fs_blktable_copy(fs_blkentry_t dest, fs_blkentry_t src)
{
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    for (uint32_t sec = 0; sec < src.count; sec++)
    {
        ata_read(src.start + sec, 1, data);
        ata_write(dest.start + sec, 1, data);
    }
    free(data);

    return TRUE;
}

// create new block entry in table
fs_blkentry_t fs_blktable_create_entry(uint32_t start, uint32_t count, uint8_t state)
{
    int i = fs_blktable_freeindex();
    if (i < 0 || i >= fs_info.blk_table_count_max) { printf("Maximum amount of block entries reached\n"); return NULL_BLKENTRY; }
    fs_blkentry_t entry;
    entry.start = start;
    entry.count = count;
    entry.state = state;
    memset(entry.padding, 0, sizeof(entry.padding));
    fs_blktable_write(i, entry);
    fs_info.blk_table_count++;
    fs_info_write();
    //printf("Created block: START: %d, STATE = 0x%02x, COUNT = %d\n", entry.start, entry.state, entry.count);
    return entry;
}

// delete existing block entry in table
bool_t fs_blktable_delete_entry(fs_blkentry_t entry)
{
    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            fs_blkentry_t* temp = (fs_blkentry_t*)(data + i);
            if (temp->start == 0 || temp->count == 0) { continue; }

            if (temp->start == entry.start && temp->count == entry.count && temp->state == entry.state)
            {
                printf("Delete block: START: 0x%08x, STATE = 0x%02x, COUNT = 0x%08x\n", entry.start, entry.state, entry.count);
                memset(temp, 0, sizeof(fs_blkentry_t));
                ata_write(fs_info.blk_table_start + sec, 1, data);
                fs_info.blk_table_count--;
                fs_info_write();
                return TRUE;
            }
        }
    }

    printf("Unable to delete block\n");
    return FALSE;
}

bool_t fs_blktable_fill(fs_blkentry_t entry, uint8_t value)
{
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            fs_blkentry_t* temp = (fs_blkentry_t*)(data + i);
            if (temp->start == 0 || temp->count == 0) { continue; }

            if (temp->start == entry.start && temp->count == entry.count && temp->state == entry.state)
            {
                ata_fill(temp->start, temp->count, value);
                free(data);
                return TRUE;
            }
        }
    }

    printf("Unable to fill block entry\n");
    free(data);
    return FALSE;
}

fs_blkentry_t fs_blktable_at_index(int index)
{
    if (index < 0 || index >+ fs_info.blk_table_count_max) { return NULL_BLKENTRY; }

    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int temp_index = 0;
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            fs_blkentry_t* temp = (fs_blkentry_t*)(data + i);
            if (temp_index == index) 
            { 
                fs_blkentry_t output;
                memcpy(&output, temp, sizeof(fs_blkentry_t));
                free(data);
                return output; 
            }
            temp_index++;
        }
    }

    printf("Unable to get block entry at index %d\n", index);
    free(data);
    return NULL_BLKENTRY;
}

// validate that sector is within block table boundaries
bool_t fs_blktable_validate_sector(uint32_t sector)
{
    if (sector < fs_info.blk_table_start || sector >= fs_info.blk_table_sector_count) { return FALSE; }
    return TRUE;
}

// get index of specified block entry
int fs_blktable_get_index(fs_blkentry_t entry)
{
    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            fs_blkentry_t* temp = (fs_blkentry_t*)(data + i);
            if (temp->start == 0 || temp->count == 0) { continue; }
            if (temp->start == entry.start && temp->count == entry.count && temp->state == entry.state) { free(data); return index; }
            index++;
        }
    }
    free(data);
    return -1;
}

// get next available block entry index in table
int fs_blktable_freeindex()
{
    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.blk_table_sector_count; sec++)
    {
        ata_read(fs_info.blk_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_blkentry_t))
        {
            fs_blkentry_t* entry = (fs_blkentry_t*)(data + i);
            if (entry->start == 0 && entry->count == 0 && entry->state == 0) { free(data); return index; }
            index++;
        }
    }
    free(data); 
    return -1;
}

// create new root directory
bool_t fs_root_create(const char* label)
{
    strcpy(fs_rootdir.name, label);
    fs_rootdir.status = 0xFF;
    fs_rootdir.parent_index = UINT32_MAX;
    fs_rootdir.type = FSTYPE_DIR;
    memset(fs_rootdir.padding, 0, sizeof(fs_rootdir.padding));
    fs_filetable_write_dir(0, fs_rootdir);

    fs_directory_t rfd = fs_filetable_read_dir(0);
    printf("Created root: NAME = %s, PARENT = 0x%08x, TYPE = 0x%02x, STATUS = 0x%02x\n", rfd.name, rfd.parent_index, rfd.type, rfd.status);
    return TRUE;
}

void fs_filetable_print()
{
    printf("------ FILE TABLE ----------------------------\n");

    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.file_table_sector_count; sec++)
    {
        ata_read(fs_info.file_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_directory_t))
        {
            fs_directory_t* entry = (fs_directory_t*)(data + i);

            if (entry->type != FSTYPE_NULL)
            {
                printf("INDEX: 0x%08x PARENT: 0x%08x TYPE: 0x%02x STATUS: 0x%02x NAME: %s\n", index, entry->parent_index, entry->type, entry->status, entry->name);
            }
            index++;
        }
    }

    printf("\n");
    free(data); 
}

// convert file index to file entry sector
uint32_t fs_filetable_sector_from_index(int index)
{
    if (index == 0) { index = 1; }
    fs_info_read();
    uint32_t sec = fs_info.file_table_start;
    uint32_t offset_bytes = (index * sizeof(fs_file_t));
    sec += (offset_bytes / ATA_SECTOR_SIZE);
    return sec;
}

// convert file index and sector to file entry sector offset
uint32_t fs_filetable_offset_from_index(uint32_t sector, int index)
{
    uint32_t offset_bytes = (index * sizeof(fs_file_t));
    uint32_t val = offset_bytes % 512;
    return val;
}

// read directory from disk at index in table
fs_directory_t fs_filetable_read_dir(int index)
{
    fs_info_read();
    if (index < 0 || index >= fs_info.file_table_count_max) { printf("Invalid index while reading directory entry\n"); return NULL_DIR; }
    uint32_t sector = fs_filetable_sector_from_index(index);
    uint32_t offset = fs_filetable_offset_from_index(sector, index);
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    ata_read(sector, 1, data);
    fs_directory_t* entry = (fs_directory_t*)(data + offset);
    fs_directory_t output;
    memcpy(&output, entry, sizeof(fs_directory_t));
    free(data);
    return output;
}

// read file form disk at index in table
fs_file_t fs_filetable_read_file(int index)
{
    fs_info_read();
    if (index < 0 || index >= fs_info.file_table_count_max) { printf("Invalid index while reading file entry\n"); return NULL_FILE; }
    uint32_t sector = fs_filetable_sector_from_index(index);
    uint32_t offset = fs_filetable_offset_from_index(sector, index);
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    ata_read(sector, 1, data);
    fs_file_t* entry = (fs_file_t*)(data + offset);
    fs_file_t output;
    memcpy(&output, entry, sizeof(fs_file_t));
    free(data); 
    return output;
}

// write directory to disk at index in table
void fs_filetable_write_dir(int index, fs_directory_t dir)
{
    fs_info_read();
    if (index < 0 || index >= fs_info.file_table_count_max) { printf("Invalid index while writing directory entry\n"); return; }
    uint32_t sector = fs_filetable_sector_from_index(index);
    uint32_t offset = fs_filetable_offset_from_index(sector, index);
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    ata_read(sector, 1, data);
    fs_directory_t* temp = (fs_directory_t*)(data + offset);
    memcpy(temp, &dir, sizeof(fs_directory_t));
    ata_write(sector, 1, data);
    free(data);
}

// write file to disk at index in table
void fs_filetable_write_file(int index, fs_file_t file)
{
    fs_info_read();
    if (index < 0 || index >= fs_info.file_table_count_max) { printf("Invalid index while writing file entry: %d\n", index); return; }
    uint32_t sector = fs_filetable_sector_from_index(index);
    uint32_t offset = fs_filetable_offset_from_index(sector, index);
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    ata_read(sector, 1, data);
    fs_file_t* temp = (fs_file_t*)(data + offset);
    memcpy(temp, &file, sizeof(fs_file_t));
    ata_write(sector, 1, data);
    free(data); 
}

// create new directory entry in table
fs_directory_t fs_filetable_create_dir(fs_directory_t dir)
{
    int i = fs_filetable_freeindex();
    if (i < 0 || i >= fs_info.file_table_count_max) { printf("Invalid index while creating directory entry\n"); return NULL_DIR; }
    fs_filetable_write_dir(i, dir);
    fs_info.file_table_count++;
    fs_info_write();
    printf("Created directory: INDEX = 0x%08x, NAME = %s, PARENT = 0x%08x, TYPE = 0x%02x, STATUS = 0x%02x\n", i, dir.name, dir.parent_index, dir.type, dir.status);
    return dir;
}

// create new file entry in table
fs_file_t fs_filetable_create_file(fs_file_t file)
{
    int i = fs_filetable_freeindex();
    if (i < 0 || i >= fs_info.file_table_count_max) { printf("Invalid index while creating file entry\n"); return NULL_FILE; }
    fs_filetable_write_file(i, file);
    fs_info.file_table_count++;
    fs_info_write();
    printf("Created file: NAME = %s, PARENT = 0x%08x, TYPE = 0x%02x, STATUS = 0x%02x, BLK = 0x%08x, SIZE = %d\n", file.name, file.parent_index, file.type, file.status, file.blk_index, file.size);
    return file;
}

// delete existing directory entry in table
bool_t fs_filetable_delete_dir(fs_directory_t dir)
{
    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.file_table_sector_count; sec++)
    {
        ata_read(fs_info.file_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_directory_t))
        {
            fs_directory_t* entry = (fs_directory_t*)(data + i);

            if (fs_dir_equals(dir, *entry))
            {
                printf("Deleted directory: NAME = %s, PARENT = 0x%08x, TYPE = 0x%02x, STATUS = 0x%02x\n", dir.name, dir.parent_index, dir.type, dir.status);
                memset(entry, 0, sizeof(fs_directory_t));
                ata_write(fs_info.file_table_start + sec, 1, data);
                free(data); 
                return TRUE;
            }
            index++;
        }
    }

    printf("Unable to delete directory\n");
    free(data); 
    return FALSE;
}

// delete existing file entry in table
bool_t fs_filetable_delete_file(fs_file_t file)
{
    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.file_table_sector_count; sec++)
    {
        ata_read(fs_info.file_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_file_t))
        {
            fs_file_t* entry = (fs_file_t*)(data + i);

            if (fs_file_equals(file, *entry))
            {
                printf("Deleted file: NAME = %s, PARENT = 0x%08x, TYPE = 0x%02x, STATUS = 0x%02x, SIZE = %d\n", file.name, file.parent_index, file.type, file.status, file.size);
                memset(entry, 0, sizeof(fs_file_t));
                ata_write(fs_info.file_table_start + sec, 1, data);
                free(data); 
                return TRUE;
            }

            index++;
        }
    }

    printf("Unable to delete directory\n");
    free(data); 
    return FALSE;
}

// validate that sector is within file table bounds
bool_t fs_filetable_validate_sector(uint32_t sector)
{
    if (sector < fs_info.file_table_start || sector >= fs_info.file_table_start + fs_info.file_table_sector_count) { return FALSE; }
    return TRUE;
}

// get next available index in file table
int fs_filetable_freeindex()
{
    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.file_table_sector_count; sec++)
    {
        ata_read(fs_info.file_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_file_t))
        {
            fs_file_t* entry = (fs_file_t*)(data + i);
            if (entry->type == FSTYPE_NULL) {free(data);  return index; }
            index++;
        }
    }

    free(data); 
    return -1;
}

// check if 2 directories are equal
bool_t fs_dir_equals(fs_directory_t a, fs_directory_t b)
{
    if (strcmp(a.name, b.name)) { return FALSE; }
    if (a.parent_index != b.parent_index) { return FALSE; }
    if (a.status != b.status) { return FALSE; }
    if (a.type != b.type) { return FALSE; }
    return TRUE;
}

// check if 2 files are equal
bool_t fs_file_equals(fs_file_t a, fs_file_t b)
{
    if (strcmp(a.name, b.name)) { return FALSE; }
    if (a.parent_index != b.parent_index) { return FALSE; }
    if (a.status != b.status) { return FALSE; }
    if (a.type != b.type) { return FALSE; }
    if (a.blk_index != b.blk_index) { return FALSE; }
    if (a.size != b.size) { return FALSE; }
    return TRUE;
}

// get parent directory from path - returns empty if unable to locate
fs_directory_t fs_parent_from_path(const char* path)
{
    if (path == NULL) { return NULL_DIR; }
    if (strlen(path) == 0) { return NULL_DIR; }

    int    args_count = 0;
    char** args = strsplit(path, '/', &args_count);

    if (args_count <= 2 && path[0] == '/') { freearray(args, args_count); return fs_rootdir; }

    if (args_count > 1)
    {
        fs_info_read();
        int32_t index = 0;
        uint32_t p = 0;
        fs_directory_t dir_out = NULL_DIR;

        for (uint32_t arg = 0; arg < args_count - 1; arg++)
        {
            dir_out = NULL_DIR;
            index = 0;

            if (args[arg] != NULL && strlen(args[arg]) > 0)
            {
                uint8_t* data = malloc(ATA_SECTOR_SIZE);
                for (uint32_t sec = 0; sec < fs_info.file_table_sector_count; sec++)
                {
                    ata_read(fs_info.file_table_start + sec, 1, data);

                    for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_directory_t))
                    {
                        fs_directory_t* dir = (fs_directory_t*)(data + i);

                        if (dir->type == FSTYPE_DIR && dir->parent_index == (uint32_t)p && !strcmp(dir->name, args[arg]))
                        {
                            p = index;
                            dir_out = *dir;
                        }

                        index++;
                    }
                }
                free(data);
            }
        }

        if (dir_out.type == FSTYPE_DIR) { freearray(args, args_count); return dir_out; }
    }

    printf("Unable to locate parent of %s\n", path);
    freearray(args, args_count);
    return NULL_DIR;
}

// return file by path - returns empty if unable to locate
fs_file_t fs_get_file_byname(const char* path)
{
    if (path == NULL) { return NULL_FILE; }
    if (strlen(path) == 0) { return NULL_FILE; }

    fs_info_read();

    fs_directory_t parent = fs_parent_from_path(path);
    if (parent.type != FSTYPE_DIR) { printf("Parent was null while getting file by name\n"); return NULL_FILE; }

    int args_count = 0;
    char** args = strsplit(path, '/', &args_count);

    if (args_count == 0) { freearray(args, args_count); printf("Args was null while getting file by name\n"); return NULL_FILE; }

    char* filename;
    int xx = args_count - 1;
    while (args[xx] != NULL)
    {
        if (args[xx] != NULL && strlen(args[xx]) > 0) { filename = args[xx]; break; }
        if (xx == 0) { break; }
        xx--;
    }
    if (strlen(filename) == 0 || filename == NULL) { freearray(args, args_count); printf("Unable to get name while getting file by name\n"); return NULL_FILE; }

    uint32_t parent_index = fs_get_dir_index(parent);

    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.file_table_sector_count; sec++)
    {
        ata_read(fs_info.file_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_file_t))
        {
            fs_file_t* entry = (fs_file_t*)(data + i);
            if (entry->type == FSTYPE_FILE && entry->parent_index == parent_index && !strcmp(entry->name, filename))
            {
                fs_file_t output = *entry;
                freearray(args, args_count);
                free(data);
                return output;
            }
            index++;
        }
    }

    freearray(args, args_count);
    free(data);
    return NULL_FILE;
}

// return directory by path - returns empty if unable to locate;
fs_directory_t fs_get_dir_byname(const char* path)
{
    if (path == NULL) { return NULL_DIR; }
    if (strlen(path) == 0) { return NULL_DIR; }

    fs_info_read();

    if (!strcmp(path, "/"))
    {
        fs_directory_t output;
        memcpy(&output, &fs_rootdir, sizeof(fs_directory_t));
        return output;
    }

    fs_directory_t parent = fs_parent_from_path(path);
    if (parent.type != FSTYPE_DIR) { printf("Parent was null while getting directory by name\n"); return NULL_DIR; }

    int args_count = 0;
    char** args = strsplit(path, '/', &args_count);

    if (args_count == 0) { freearray(args, args_count); printf("Args was null while getting directory by name\n"); return NULL_DIR; }

    char* dirname;
    int xx = args_count - 1;
    while (args[xx] != NULL)
    {
        if (args[xx] != NULL && strlen(args[xx]) > 0) { dirname = args[xx]; break; }
        if (xx == 0) { break; }
        xx--;
    }
    if (strlen(dirname) == 0 || dirname == NULL) { printf("Unable to get name while getting directory by name\n"); return NULL_DIR; }

    uint32_t parent_index = fs_get_dir_index(parent);

    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.file_table_sector_count; sec++)
    {
        ata_read(fs_info.file_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_directory_t))
        {
            fs_directory_t* entry = (fs_directory_t*)(data + i);
            if (entry->type == FSTYPE_DIR && entry->parent_index == parent_index && !strcmp(entry->name, dirname))
            {
                fs_directory_t output;
                memcpy(&output, entry, sizeof(fs_directory_t));
                freearray(args, args_count);
                free(data);
                return output;
            }
            index++;
        }
    }

    freearray(args, args_count);
    free(data);
    return NULL_DIR;
}

// get index of specified file entry
int fs_get_file_index(fs_file_t file)
{
    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.file_table_sector_count; sec++)
    {
        ata_read(fs_info.file_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_file_t))
        {
            fs_file_t* entry = (fs_file_t*)(data + i);
            if (fs_file_equals(*entry, file)) { free(data); return index; }
            index++;
        }
    }
    free(data);
    return -1;
}

// get index of specified directory entry
int fs_get_dir_index(fs_directory_t dir)
{
    fs_info_read();
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    int index = 0;
    for (uint32_t sec = 0; sec < fs_info.file_table_sector_count; sec++)
    {
        ata_read(fs_info.file_table_start + sec, 1, data);

        for (uint32_t i = 0; i < ATA_SECTOR_SIZE; i += sizeof(fs_directory_t))
        {
            fs_directory_t* entry = (fs_directory_t*)(data + i);
            if (fs_dir_equals(*entry, dir)) { free(data); return index; }
            index++;
        }
    }
    free(data);
    return -1;
}

int ceilnum(float num) 
{
    int inum = (int)num;
    if (num == (float)inum) { return inum; }
    return inum + 1;
}

uint32_t fs_bytes_to_sectors(uint32_t bytes)
{
    return (uint32_t)ceilnum((float)bytes / (float)ATA_SECTOR_SIZE);
}

char* fs_get_name_from_path(const char* path)
{
    if (path == NULL) { return NULL; }
    if (strlen(path) == 0) { return NULL; }

    if (!strcmp(path, "/"))
    {
        char* rootname = malloc(strlen(fs_rootdir.name) + 1);
        memset(rootname, 0, strlen(fs_rootdir.name));
        strcpy(rootname, fs_rootdir.name);
        return rootname;
    }

    int args_count = 0;
    char** args = strsplit(path, '/', &args_count);

    if (args_count == 0) { freearray(args, args_count); printf("Args was null while getting name from path\n"); return NULL; }

    if (args_count == 2 && path[0] == '/')
    {
        char* rootname = malloc(strlen(args[1]) + 1);
        memset(rootname, 0, strlen(args[1]) + 1);
        strcpy(rootname, args[1]);
        freearray(args, args_count);
        return rootname;
    }

    char* filename;
    int xx = args_count - 1;
    while (args[xx] != NULL)
    {
        if (args[xx] != NULL && strlen(args[xx]) > 0) { filename = args[xx]; break; }
        if (xx == 0) { break; }
        xx--;
    }

    char* output = malloc(strlen(filename) + 1);
    memset(output, 0, strlen(filename));
    strcpy(output, filename);
    freearray(args, args_count);
    return output;
}

char* fs_get_parent_path_from_path(const char* path)
{

    if (path == NULL) { return NULL; }
    if (strlen(path) == 0) { return NULL; }

    char* output = malloc(strlen(path + 16));
    memset(output, 0, strlen(path + 16));

    if (!strcmp(path, "/")) { free(output); return NULL; }

    int args_count = 0;
    char** args = strsplit(path, '/', &args_count);   

    if (args_count <= 2 && path[0] == '/') { freearray(args, args_count); strcpy(output, "/"); return output; }

    strcat(output, "/");
    for (int i = 0; i < args_count - 1; i++)
    {
        if (strlen(args[i]) == 0) { continue; }
        strcat(output, args[i]);
        strcat(output, "/");
    }
    
    freearray(args, args_count);
    return output;
}

fs_file_t fs_file_create(const char* path, uint32_t size)
{
    if (size == 0) { printf("Cannot create blank file\n"); return NULL_FILE; }

    fs_directory_t parent = fs_parent_from_path(path);
    if (parent.type != FSTYPE_DIR) { printf("Unable to locate parent while creating file\n"); return NULL_FILE; }

    uint32_t sectors = fs_bytes_to_sectors(size);
    fs_blkentry_t blk = fs_blktable_allocate(sectors);

    // set properties and create file
    fs_file_t file;
    file.parent_index = fs_get_dir_index(parent);
    file.type         = FSTYPE_FILE;
    file.status       = 0x00;
    file.size         = size;
    file.blk_index    = fs_blktable_get_index(blk);
    char* name = fs_get_name_from_path(path);
    strcpy(file.name, name);
    free(name);

    fs_blktable_fill(blk, 0x00);

    fs_file_t new_file = fs_filetable_create_file(file);
    return new_file;
}

fs_file_t fs_file_read(const char* path)
{
    if (path == NULL) { printf("Path was null while trying to read file %s\n", path); return NULL_FILE; }
    if (strlen(path) == 0) { printf("Path was empty while trying to read file %s\n", path); return NULL_FILE; }

    fs_file_t file = fs_get_file_byname(path);
    if (file.type != FSTYPE_FILE) { printf("Unable to locate file %s", path); return NULL_FILE; }

    fs_blkentry_t blk = fs_blktable_read(file.blk_index);

    uint8_t* data = malloc(blk.count * ATA_SECTOR_SIZE);
    for (uint32_t i = 0; i < blk.count; i++) { ata_read(blk.start + i, 1, data + (i * ATA_SECTOR_SIZE)); }
    file.data = data;
    return file;
}

bool_t fs_file_write(const char* path, uint8_t* data, uint32_t len)
{
    if (path == NULL) { printf("Path was null while trying to write file %s\n", path); return FALSE; }
    if (strlen(path) == 0) { printf("Path was empty while trying to write file %s\n", path); return FALSE; }

    fs_file_t tryload = fs_get_file_byname(path);
    if (tryload.type != FSTYPE_FILE) 
    { 
        printf("File %s does not exist and will be created\n", path);
        fs_file_t new_file = fs_file_create(path, len);
        if (new_file.type != FSTYPE_FILE) { printf("Unable to create new file %s\n", path); return FALSE; }
        fs_blkentry_t blk = fs_blktable_read(new_file.blk_index);

        uint8_t* secdata = malloc(ATA_SECTOR_SIZE);
        for (uint32_t i = 0; i < blk.count; i++)
        {
            memset(secdata, 0, ATA_SECTOR_SIZE);
            for (uint32_t j = 0; j < ATA_SECTOR_SIZE; j++)
            {
                uint32_t offset = (i * ATA_SECTOR_SIZE) + j;
                if (offset < len) { secdata[j] = data[offset]; } else { break; }
            }       
            ata_write(blk.start + i, 1, secdata);
        }

        printf("Written file %s to disk, size = %d\n", path, new_file.size);
        free(secdata);
        return TRUE;
    }
    else 
    { 
        printf("File %s exists\n", path); 
        int findex = fs_get_file_index(tryload);
        fs_blkentry_t blk = fs_blktable_read(tryload.blk_index);
        fs_blktable_free(blk);
        blk = fs_blktable_allocate(fs_bytes_to_sectors(len));
        tryload.blk_index = fs_blktable_get_index(blk);
        tryload.size = len;
        fs_filetable_write_file(findex, tryload);

        uint8_t* secdata = malloc(ATA_SECTOR_SIZE);
        for (uint32_t i = 0; i < blk.count; i++)
        {
            memset(secdata, 0, ATA_SECTOR_SIZE);
            for (uint32_t j = 0; j < ATA_SECTOR_SIZE; j++)
            {
                uint32_t offset = (i * ATA_SECTOR_SIZE) + j;
                if (offset < len) { secdata[j] = data[offset]; } else { break; }
            }       
            ata_write(blk.start + i, 1, secdata);
        }  

        printf("Written file %s to disk, size = %d\n", path, tryload.size);
        return TRUE;      
    }



    return TRUE;
}