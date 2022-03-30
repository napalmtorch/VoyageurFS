#include "ata.h"

uint8_t* ata_data;
uint64_t ata_size;
char*    ata_filename;

void ata_init()
{
    ata_data     = NULL;
    ata_size     = 0;
    ata_filename = NULL;
    printf("Initialized ATA controller\n");
}

void ata_unload()
{
    if (ata_data != NULL) { free(ata_data); ata_data = NULL; }
    ata_size = 0;
    printf("Unloaded disk image '%s'\n", ata_filename);
    if (ata_filename != NULL) { free(ata_filename); ata_filename = NULL; }
}

void ata_load_file(const char* filename)
{
    FILE* fileptr = fopen(filename, "rb");
    if (fileptr == NULL) { printf("Unable to locate disk image '%s'\n", filename); return; }
    fseek(fileptr, 0, SEEK_END);
    size_t size = ftell(fileptr);
    fseek(fileptr, 0, SEEK_SET);
    if (size == 0) { printf("Unable to locate disk image '%s'\n", filename); return; }

    if (ata_data != NULL) { free(ata_data); }    
    ata_data = malloc(size);
    ata_size = size;

    if (ata_filename != NULL) { free(ata_filename); }
    ata_filename = malloc(strlen(filename) + 1);
    strcpy(ata_filename, filename);
    
    fread(ata_data, size, 1, fileptr);
    fclose(fileptr);
    printf("Loaded disk image '%s'\n", ata_filename);
}

void ata_save_file(const char* filename)
{
    if (ata_data == NULL) { printf("No disk image loaded\n"); return; }
    FILE* fileptr = fopen(filename, "wb");
    if (fileptr == NULL) { printf("Unable to save disk image '%s'\n", filename); return; }

    fwrite(ata_data, ata_size, 1, fileptr);
    if (ata_filename != NULL) { free(ata_filename); }
    ata_filename = malloc(strlen(filename) + 1);
    strcpy(ata_filename, filename);
    fclose(fileptr);
    printf("Saved disk image '%s'\n", filename);
}

void ata_create(uint64_t size)
{
    ata_size     = size;
    ata_data     = malloc(size);
    ata_filename = NULL;

    memset(ata_data, 0, ata_size);
    printf("Created disk of size %lld MB\n", size / 1024 / 1024);
}

void ata_read(uint64_t sector, uint32_t count, uint8_t* buffer)
{
    uint8_t* p_start = ata_data + (sector * ATA_SECTOR_SIZE);
    uint32_t len = count * ATA_SECTOR_SIZE;
    memset(buffer, 0, len);
    memcpy(buffer, p_start, len);
}

void ata_write(uint64_t sector, uint32_t count, uint8_t* buffer)
{
    uint8_t* p_start = ata_data + (sector * ATA_SECTOR_SIZE);
    uint32_t len = count * ATA_SECTOR_SIZE;
    memcpy(p_start, buffer, len);
}

void ata_fill(uint64_t sector, uint32_t count, uint8_t value)
{
    uint8_t* data = malloc(ATA_SECTOR_SIZE);
    memset(data, value, ATA_SECTOR_SIZE);
    for (uint64_t i = 0; i < count; i++) { ata_write(sector + i, 1, data); }
    free(data);
}

void ata_filldata(uint64_t sector, uint32_t count, uint8_t* data)
{
    for (uint64_t i = 0; i < count; i++)
    {
        ata_write(sector + i, 1, data + (i * ATA_SECTOR_SIZE));
    }
}

uint32_t ata_get_disk_size() { return ata_size; }

uint8_t* ata_get_data() { return ata_data; }