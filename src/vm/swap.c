#include "swap.h"

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
