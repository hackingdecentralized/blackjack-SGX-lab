#include <stdlib.h>
#include "files.h"

int read_file(const char * path, const char * mode, uint8_t * data, size_t max_data_size, size_t * data_size) {
    FILE *fptr;
    fptr = fopen(path, mode); 
    *data_size = fread((char *) data, 1, max_data_size, fptr);
    fclose(fptr); 
    return 0;
}


int write_file(const char * path, const char * mode, const uint8_t * data, size_t data_size) {
    FILE *fptr;
    fptr = fopen(path, mode); 
    int ret = fwrite(data, 1, data_size, fptr);
    fclose(fptr);
    return 0;
}
