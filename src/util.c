#include "util.h"

void hexdump(uint8_t* addr, uint32_t len)
{
    printf("Dumping %d bytes of memory at 0x%X\n", len, (uint32_t)addr);
    const uint32_t bpl = 16;
    char chars[bpl + 1];

    for (uint32_t i = 0; i < len; i += bpl)
    {
        memset(chars, 0, bpl + 1);

        printf("0x%X:0x%X  ", (uint32_t)(addr + i), (uint32_t)(addr + i + (bpl - 1)));

        for (uint32_t j = 0; j < bpl; j++)
        {
            uint8_t v = addr[i + j];
            printf("%02X ", v);

            if (v >= 32 && v <= 126) { chars[j] = v; } else { chars[j] = '.'; }
        }

        printf("  %s\n", chars);
    }
}

char** strsplit(const char* str, char delim, int* count)
{
    if (str == NULL) { return NULL; }
    if (strlen(str) == 0) { return NULL; }

    int len = strlen(str);
    uint32_t num_delimeters = 0;

    for(int i = 0; i < len - 1; i++)
    {
        if (str[i] == delim) { num_delimeters++; }
    }

    uint32_t arr_size = sizeof(char*) * (num_delimeters + 1);
    char** str_array = malloc(arr_size);
    memset(str_array, 0, arr_size);
    int str_offset = 0;

    int start = 0;
    int end = 0;
    while(end < len)
    {
        while(str[end] != delim && end < len) { end++; }
        char* substr = malloc((end - start + 1) + 1);
        memset(substr, 0, end - start + 1);
        memcpy(substr, str + start, end - start);
        start = end + 1;
        end++;
        str_array[str_offset] = substr;
        str_offset++;
    }

    //return necessary data now
    *count = str_offset;
    return str_array;
}

void freearray(char** ptr, int count)
{
    if (ptr == NULL) { return; }
    for (int i = 0; i < count; i++)
    {
        if (ptr[i] != NULL) { free(ptr[i]); }
    }
    free(ptr);
}