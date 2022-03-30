#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util.h"

#define ATA_SECTOR_SIZE 512

void ata_init();
void ata_load_file(const char* filename);
void ata_save_file(const char* filename);
void ata_create(uint64_t size);
void ata_unload();

void ata_read(uint64_t sector, uint32_t count, uint8_t* buffer);
void ata_write(uint64_t sector, uint32_t count, uint8_t* buffer);
void ata_fill(uint64_t sector, uint32_t count, uint8_t value);
void ata_filldata(uint64_t sector, uint32_t count, uint8_t* data);

uint32_t ata_get_disk_size();
uint8_t* ata_get_data();
