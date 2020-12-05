#include "disksupport.h"

char* directorySearch(char* address, char* directory);
void sendFile(char* address, char* address1, char* fp, char* file, int sizeof_file);
void diskUpdate(char* address, char* file, int sizeof_file, int logical);
int FATFree(char* address);
void FATSet(char* address, int i, int value);

int main(int argc, char* argv[]) {
  if(argc != 3){
    printf("Error, incorrect format.\nUse: ./diskput <disk>.ima [directory path]<file>\n");
    exit(1);
  }
  int path_exists = 0;
  int i;
  for(i = 0; i < strlen(argv[2]); i++){
    if(argv[2][i] == '/'){
      path_exists = 1;
      break;
    }
  }
  char* file;
  char* subdirectory = NULL;
  char* tokens[256];
  int cnt = 1;
  if(path_exists == 1) {
    char* token = strtok(argv[2], "/");
    while(token != NULL){
      tokens[cnt++] = token;
      token = strtok(NULL, "/");
    }
    tokens[i] = NULL;
    subdirectory = strdup(tokens[cnt-2]);
    file = strdup(tokens[cnt-1]);
  } else {
    file = argv[2];
  }
  int fd;
  struct stat sb;
  if((fd = open(argv[1], O_RDWR)) < 0) {
    printf("Error: Disk image was unable to be read. \n");
    exit(1);
  }
  fstat(fd, &sb);
  char* address = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(address == MAP_FAILED) {
    printf("Error: Memory was failed to be mapped.\n");
    exit(1);
  }
  int fd1;
	struct stat sb1;
	if((fd1 = open(file, O_RDWR)) < 0){
		printf("File not found.\n");
		exit(1);
	}
	fstat(fd1, &sb1);
	char* address1 = mmap(NULL, sb1.st_size, PROT_READ, MAP_SHARED, fd1, 0);
	if(address1 == MAP_FAILED){
		printf("Error: Memory was failed to be mapped.\n");
		exit(1);
	}
	if(sb1.st_size >= freeDisk(address)){
		printf("Not enough free space in the disk image.\n");
		exit(1);
	}
  char* subdirectory_address;
  if(path_exists == 1){
    subdirectory_address = directorySearch(address + SECTOR_SIZE * 19, subdirectory);
    if(subdirectory_address == NULL){
      printf("Error: The directory was not found.\n");
      exit(1);
    }
  } else {
    subdirectory_address = address;
  }
  sendFile(address, address1, subdirectory_address, file, sb1.st_size);
  munmap(address, sb.st_size);
  munmap(address1, sb1.st_size);
  close(fd);
  close(fd1);
  return 0;
}

void sendFile(char* address, char* address1, char* fp, char* file, int sizeof_file){
  int current = FATFree(address);
  int bytes = sizeof_file;
  int physical;
  if(address != fp){
    diskUpdate(address + (fp[26] + 31) * SECTOR_SIZE, file, sizeof_file, current);
  } else {
    diskUpdate(address + SECTOR_SIZE * 19, file, sizeof_file, current);
  }
  do{
    physical = (current + 31) * SECTOR_SIZE;
    int i;
    for(i = 0; i < SECTOR_SIZE; i++){
      if(bytes <= 0) {
        FATSet(address, current, 0xfff);
        break;
      }
      address[i + physical] = address1[sizeof_file - bytes];
      bytes--;
    }
    FATSet(address, current, 0x77);
    int next = FATFree(address);
    FATSet(address, current, next);
    retrieveFAT(address, current);
    current = next;
  } while(bytes > 0);
}

void diskUpdate(char* address, char* file, int sizeof_file, int logical){
  while(address[0] != 0x00){
    address += 0x20;
  }
  int i;
  int j = -1;
  for(i = 0; i < 8; i++) {
    if(file[i] == '.'){
      j = i; break;
    }
    if(j == -1) {
      address[i] = file[i];
    } else {
      address[i] = ' ';
    }
  }
  for(i = 0; i < 3; i++){
    address[i+8] = file[i+j+1];
  }
  address[11] = 0x00;
  time_t t = time(NULL);
	struct tm* now = localtime(&t);
	int year = now->tm_year + 1900;
	int month = (now->tm_mon + 1);
	int day = now->tm_mday;
	int hour = now->tm_hour;
	int minute = now->tm_min;
	address[14] = 0x00;
	address[15] = 0x00;
	address[16] = 0x00;
	address[17] = 0x00;
	address[17] |= (year - 1980) << 1;
	address[17] |= (month - ((address[16] & 0xe0) >> 5)) >> 3;
	address[16] |= (month - (((address[17] & 0x01)) << 3)) << 5;
	address[16] |= (day & 0x1f);
	address[15] |= (hour << 3) & 0xf8;
	address[15] |= (minute - ((address[14] & 0xe0) >> 5)) >> 3;
	address[14] |= (minute - ((address[15] & 0x07) << 3)) << 5;
	//set the first logical cluster
	address[26] = (logical - (address[27] << 8)) & 0xff;
	address[27] = (logical - address[26]) >> 8;
	//set the file size
	address[28] = sizeof_file & 0x000000ff;
	address[29] = (sizeof_file & 0x0000ff00) >> 8;
	address[30] = (sizeof_file & 0x00ff0000) >> 16;
	address[31] = (sizeof_file & 0xff000000) >> 24;
}

void FATSet(char* address, int i, int val){
  if(i%2 == 0){
		address[SECTOR_SIZE + (3*i/2) + 1] = (val >> 8) & 0x0f;
		address[SECTOR_SIZE + (3*i/2)] = val & 0xff;s
	}
	else{
		address[SECTOR_SIZE + (3*i/2)] = (val << 4) & 0xf0;
		address[SECTOR_SIZE + (3*i/2) + 1] = (val >> 4) & 0xff;
	}
}

int FATFree(char* address){
  int i = 2;
  char* current = address + (i + 30) * SECTOR_SIZE;
  while(retrieveFAT(address, i) != 0x00 || current[0] != 0x00){
    i++;
    current = address + (i + 31) * SECTOR_SIZE;
  }
  return i;
}

char* directorySearch(char* address, char* directory){
  char* init = address;
  char* file_was_found = NULL;
  char* subdirectory = NULL;
  char name[21];
  while(address[0] != 0x00){
    if(address[26] != 0x00 && address[26] != 0x01 && address[11] != 0x0f && (address[11] & 0x08) != 0x08){
      int i;
			for(i = 0; i < 8 && address[i] != ' '; i++){
				name[i] = address[i];
			}
      name[i] = '\0';
      if(!strncmp(name, directory, 25)){
        return address;
      }
      if((address[11] & 0x10) == 0x10){
        subdirectory = address;
        address += 0x20;
        file_was_found = directorySearch(address, directory);
        if(file_was_found != NULL){
          return file_was_found;
        } else {
          file_was_found = directorySearch(init + (subdirectory[26] + 12) * SECTOR_SIZE, directory);
        }
      }
  }
  address += 0x20;
}
return NULL;
}
