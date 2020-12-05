#include "disksupport.h"
#include <stdio.h>

/////// Function Prototypes ///////
char* searchFile(char* address, char* file);
void retrieveFile(char* address, char* address1, char* fp, int sizeof_file);

int main(int argc, char* argv[]) {
  if(argc != 3) {
    printf("Error, incorrect format.\nUse: ./diskget <disk>.ima <file>\n");
    exit(1);
  }
  int fd;
  struct stat sb;
  if((fd = open(argv[1], O_RDWR)) < 0) {
    printf("Error: Disk image was unable to be read. \n");
    exit(1);
  }
  fstat(fd, &sb);
  char* address = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if(address == MAP_FAILED) {
    printf("Error: Memory was failed to be mapped.\n");
    exit(1);
  }
  char* file_was_found = searchFile(address + SECTOR_SIZE * 19, argv[2]);
  if(file_was_found == NULL) {
    munmap(address, sb.st_size);
    printf("Error: File was not found. \n");
    exit(1);
  }
  int sizeof_file = (file_was_found[28] & 0xff) + ((file_was_found[29] & 0xff) << 8) + ((file_was_found[30] & 0xff) << 16) + ((file_was_found[31] & 0xff) << 24);
  FILE* fp = fopen(argv[2], "wb+");
  if(fp == NULL){
    munmap(address, sb.st_size);
		close(fd);
		fclose(fp);
		printf("Error: file failed to open.\n");
		exit(1);
  }
  int fd1 = fileno(fp);
  if(fseek(fp, sizeof_file-1, SEEK_SET) == -1){
    munmap(address, sb.st_size);
		close(fd);
		fclose(fp);
		close(fd1);
		printf("Error: Seek for end of file failed.\n");
		exit(1);
  }
  char* address1 = mmap(NULL, sizeof_file, PROT_WRITE, MAP_SHARED, fd1, 0);
  if(address1 == MAP_FAILED){
    printf("Error: Memory was failed to be mapped.\n");
    exit(1);
  }
  retrieveFile(address, address1, file_was_found, sizeof_file);
  munmap(address, sb.st_size);
  munmap(address1, sizeof_file);
  close(fd);
  close(fd1);
  fclose(fp);
  return 0;
}

char* searchFile(char* address, char* file){
  char* init = address;
  while(address[0] != 0x00) {
    if(address[0] != '.' && address[1] != '.' && address[26] != 0x00 && address[26] != 0x01 && address[11] != 0x0f && (address[11] & 0x08) != 0x08){
      if(address[11] & 0x10){
        char name[21];
        char ext[4];
        int i;
        for(i = 0; i < 8 && address[i] != ' '; i++){
          name[i] = address[i];
        }
        name[i] = '\0';
        for(i = 0; i < 3 && address[i+8] != ' '; i++){
          ext[i] = address[i+8];
        }
        ext[i] = '\0';
        strcat(name, ".");
        strncat(name, ext, strlen(ext));
        if(strncmp(name, file, 25) == 0){
          return address;
        }
      } else {
        char* file_was_found = searchFile(init + (address[26] + 12) * SECTOR_SIZE, file);
        if(file_was_found != NULL) {
          return file_was_found;
        }
      }
    }
    address += 0x20;
  }
  return NULL;
}

void retrieveFile(char* address, char* address1, char* fp, int sizeof_file){
  int i, physical;
  int logical = fp[26] + (fp[27] << 8);
  int bytes = sizeof_file;
  do{
    if(bytes == sizeof_file){
      i = logical;
    } else {
      i = retrieveFAT(address, i);
    }
    physical = (31 + i) * SECTOR_SIZE;
    for(int j = 0; j < SECTOR_SIZE && bytes > 0; j++){
      address1[sizeof_file - bytes] = address[j + physical];
      bytes--;
    }
  }
  while(bytes > 0);
}
