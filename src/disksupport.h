#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SECTOR_SIZE 512

int retrieveFAT(char* address, int i);
int sizeOfDisk(char* address);
int freeDisk(char* address);
