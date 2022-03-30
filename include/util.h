#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PACKED __attribute__((packed))

#define FALSE 0
#define TRUE 1

typedef unsigned char bool_t;

void hexdump(uint8_t* addr, uint32_t len);

char** strsplit(const char* str, char delim, int* count);

void freearray(char** ptr, int count);