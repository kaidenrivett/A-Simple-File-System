#include "disksupport.h"

char* displayDiskImage(char* address);

int main(int argc, char* argv[]) {
  if(argc != 2){
    printf("Error, incorrect format.\nUse: ./disklist <disk>.ima\n");
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
  printf("\nROOT\n\n==================\n\n");
  displayDiskImage(address + SECTOR_SIZE * 19);
  munmap(address, sb.st_size);
  close(fd);
  return 0;
}

char* displayDiskImage(char* address) {
  char name[21];
  char ext[4];
  char* init = address;
  char* subdirectory = NULL;
  do{
    while(address[0] != 0x00) {
      if(address[26] != 0x00 && address[26] != 0x01 && address[11] != 0x0f && ((address[11] & 0x08) != 0x08)) {
        int sizeof_file = 0;
        char form;
        if((address[11] & 0x10) == 0x10) {
          form = 'D';
          if(address[0] == '.' || address[1] == '.') {
            address += 0x20;
            continue;
          }
        } else {
        form = 'F';
        sizeof_file = (address[28] & 0xff) + ((address[29] & 0xff) << 8) + ((address[30] & 0xff) << 16) + ((address[31] & 0xff) << 24);
      }
      int i;
      for(i = 0; i < 8 && address[i] != ' '; i++){
        name[i] = address[i];
      }
      name[i] = '\0';
      for(i = 0; i < 3; i++){
        ext[i] = address[i+8];
      }
      ext[i] = '\0';
      if(form == 'F'){
        strcat(name, ".");
      }
      strncat(name , ext, 3);
      int year = ((address[17] & 0xfe) >> 1) + 1980;
			int month = ((address[16] & 0xe0) >> 5) + ((address[17] & 0x01) << 3);
			int day = address[16] & 0x1f;
			int hour = (address[15] & 0xf8) >> 3;
			int min = ((address[14] & 0xe0) >> 5) + ((address[15] & 0x07) << 3);
			if((address[11] & 0x02) == 0 && (address[11] & 0x08) == 0) {
        printf("%c %10u %20s    %02d %02d %04d %02d:%02d\n", form, sizeof_file, name, month, day, year, hour, min);
      }
      if((address[11] & 0x10) == 0x10) {
        subdirectory = address;
        address = displayDiskImage(address + 0x20);
        printf("\n");
        for(int i = 0; i < 8; i++) {
          printf("%c", subdirectory[i]);
        }
        printf("\n\n==================\n\n");
        address = displayDiskImage(init + (subdirectory[26] + 12) * SECTOR_SIZE);
      }
  }
    address += 0x20;
  }
  subdirectory = address;
}
  while(subdirectory != NULL && subdirectory[0] != 0x00);
    return subdirectory;
}
