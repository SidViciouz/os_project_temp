#ifndef FRAME_HEADER
#define FRAME_HEADER
/*
#include "page.h"

struct frame_e{
	struct list_elem elem;
	struct thread *t;
	void *kaddr;
	struct spt_e *spte;
};

void init_frame_list(void);

void insert_frame_e(struct list_elem *e);

void free_frame(int swap_slot);
*/

#include <hash.h>
#include "threads/synch.h"
#include "threads/palloc.h"

void frame_init(void);
void* frame_allocate(enum palloc_flags flags,void *upage);

void frame_free(void*);
void frame_remove_entry(void*);

void frame_pin(void* kpage);
void frame_unpin(void* kpage);

#endif
