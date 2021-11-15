/*
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
}*/

#include "swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include <bitmap.h>

static struct block *swap_block;
static struct bitmap *swap_available;

static const int SECTORS_PER_PAGE = PGSIZE/BLOCK_SECTOR_SIZE;

static int swap_size;

void swap_init(){
	swap_block = block_get_role(BLOCK_SWAP);
	if(swap_block == NULL){
		PANIC("Error: can't initialize swap block");
		NOT_REACHED();
	}
	swap_size = block_size(swap_block)/SECTORS_PER_PAGE;
	swap_available = bitmap_create(swap_size);
	bitmap_set_all(swap_available,true);
}

int swap_out(void *page){

	ASSERT(page >= PHYS_BASE);

	int swap_index = bitmap_scan(swap_available,0,1,true);

	int i;
	for(i=0; i<SECTORS_PER_PAGE; i++)
		block_write(swap_block,swap_index*SECTORS_PER_PAGE+i,page+BLOCK_SECTOR_SIZE*i);
	
	bitmap_set(swap_available,swap_index,false);
	return swap_index;
}

void swap_in(int swap_index,void *page){
	
	ASSERT(page>=PHYS_BASE);

	ASSERT(swap_index < swap_size);

	if(bitmap_test(swap_available,swap_index) == true){
		PANIC("Error, invalid read access to unassinged swap block");
	}

	int i;
	for(i=0; i <SECTORS_PER_PAGE; i++){
		block_read(swap_block,swap_index*SECTORS_PER_PAGE+i,page+BLOCK_SECTOR_SIZE*i);
	}
	bitmap_set(swap_available,swap_index,true);
}
void swap_free(int swap_index){
	
	ASSERT(swap_index < swap_size);
	if(bitmap_test(swap_available,swap_index) == true){
		PANIC("Error, invalid free requeset to unassigned swap block");
	}
	bitmap_set(swap_available,swap_index,true);
}
