#include "disksupport.h"


int fileCount(char* address);
void diskImageLabel(char* address, char* label);
void osName(char* address, char* os);
void displayDiskImage(char* os, char* label, int size, int free, int files, int fat, int sectors);

int main(int argc, char* argv[]) {
  if(argc != 2) {
    printf("Error, incorrect format.\nUse: ./diskinfo <disk>.ima\n");
    exit(1);
  }

  // mmap is implemented here with reference to the sample code given on connex in the file mmap_test.c
  int fd;
  struct stat sb;
  if((fd = open(argv[1], O_RDONLY)) < 0) {
    printf("Error: Disk image was unable to be read. \n");
    exit(1);
  }
  fstat(fd, &sb);
  char* address = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0); // address points to the starting position of the mapped memory
  if(address == MAP_FAILED){
    printf("Error: Memory was failed to be mapped.\n");
    exit(1);
  }

  char os[16];
  char label[16];
  osName(address, os);
  diskImageLabel(address, label);
  int fat = address[16];
  int sectors = address[22] + (address[23] << 8);
  int files = fileCount(address + SECTOR_SIZE * 19);
  int size = sizeOfDisk(address);
  int free = freeDisk(address);

  displayDiskImage(os, label, size, free, files, fat, sectors);
  // delete the mappings for the specified address range using munmap
  munmap(address, sb.st_size);
  close(fd);
  return 0;
}


int fileCount(char* address){
  int cnt = 0;
  char* init = address;
  while(address[0] != 0x00) {
    if(address[0] != '.' && address[1] != '.' && address[26] != 0x00 && address[26] != 0x01 && address[11] != 0x0f && (address[11] & 0x08) != 0x08) {
      if((address[11] & 0x10) != 0x10) {
        cnt++;
      } else {
        cnt += fileCount(init + (address[26] + 31 - 19) * SECTOR_SIZE);
      }
    }
    address += 0x20;
  }
  return cnt;
}


void diskImageLabel(char* address, char* label) {
  address += SECTOR_SIZE * 19;
  int i;
  while(address[0] != 0x00) {
    if(address[11] == 0x08) {
      for(i = 0; i < 8; i++){
        label[i] = address[i];
      }
      break;
    }
    address += 0x20;
  }
}

void osName(char* address, char* os) {
  for(int i = 0; i < 8; i++){ os[i] = address[i+3]; }
}

void displayDiskImage(char* os, char* label, int size, int free, int files, int fat, int sectors) {
  // displaying information as assignment specifies
  printf("OS Name: %.8s\n", os);
	printf("Label of the disk: %.6s\n", label);
	printf("Total size of the disk: %d bytes\n", size);
	printf("Free size of the disk: %d bytes\n", free);
	printf("\n==============\n");
	printf("The number of files in the disk (including all files in the root directory and files in all subdirectories): %d\n", files);
	printf("\n=============\n");
	printf("Number of FAT copies: %d\n", fat);
	printf("Sectors per FAT: %d\n", sectors);
}
