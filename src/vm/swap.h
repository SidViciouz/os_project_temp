#ifndef SWAP_HEADER
#define SWAP_HEADER

/*
int find_swap_slot(); //return -1 if no swapslot, return n if there is swapslot to use with index n.

int swap_to_disk(int swap_slot,void* kaddr);
void swap_to_addr(int swap_slot,void * kaddr);*/

void swap_init();
int swap_out(void *page);
void swap_int(int swap_index,void *page);
void swap_free(int swap_index);

#endif
