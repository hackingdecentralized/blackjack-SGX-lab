
#ifndef _FILES_H
#define _FILES_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


int read_file(const char * path, const char * mode, uint8_t * data, size_t max_data_size, size_t * data_size);
int write_file(const char * path, const char * mode, const uint8_t * data, size_t data_size);

#endif /* _FILES_H */