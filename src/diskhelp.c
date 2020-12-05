#include "disksupport.h"

//get the FAT entry at index K
int retrieveFAT(char* address, int i){
	int low, high;
	if((i % 2) == 0){
		low = address[SECTOR_SIZE + (3*i/2) + 1] & 0x0f;
		high = address[SECTOR_SIZE + (3*i/2)] & 0xff;
		return (low << 8) + high;
	}
	else{
		low = address[SECTOR_SIZE + (3*i/2)] & 0xf0;
		high = address[SECTOR_SIZE + (3*i/2) + 1] & 0xff;
		return (low >> 4) + (high << 4);
	}
}

//get the total size of the disk image
int sizeOfDisk(char* address){
	int bytes = address[11] + (address[12] << 8);
	int sectors = address[19] + (address[20] << 8);
	return bytes * sectors;
}

//get the free space of the disk image
int freeDisk(char* address){
	int free = 0;
	for(int i = 19; i < address[19] + (address[20] << 8); i++){
		if(retrieveFAT(address, i) == 0x000){
			free++;
		}
	}
	return free * SECTOR_SIZE;
}
