#include "swap.h"
#include "devices/block.h"

static struct bitmap *swap_bitmap;

void init_swap_bitmap()
{
	swap_bitmap = bitmap_create(1024);
}

int find_swap_slot()
{
	if(bitmap_count(swap_bitmap,0,1023,0) == 0)
		return -1;
	
	for(int i=0; i<1024; i++){
		if(bitmap_count(swap_bitmap,i,1,0) == 1)
			return i;
	}
}

int swap_to_disk(int swap_slot,void* kaddr){
	
	struct block* b = block_get_role(BLOCK_SWAP);
	
	for(int i=0; i<8; i++)
		block_write(b,swap_slot*8+i,kaddr +i*BLOCK_SECTOR_SIZE);	
	bitmap_set(swap_bitmap,swap_slot,1);
	
}

void swap_to_addr(int swap_slot,void * kaddr){
	
	struct block* b = block_get_role(BLOCK_SWAP);

	for(int i=0; i<8; i++)
		block_read(b,swap_slot*8+i,kaddr+i*BLOCK_SECTOR_SIZE);	
	bitmap_set(swap_bitmap,swap_slot,0);
}
