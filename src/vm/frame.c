#include "frame.h"

static struct list frame_list;


void init_frame_list(void)
{
	list_init(&frame_list);
}

void insert_frame_e(struct list_elem* e)
{
	list_push_back(&frame_list,e);
}

void free_frame(void)
{
	//dirty bit에 따라 swap slot에 swap in하는거 추가해야함
	struct list_elem *e = list_pop_back(&frame_list); //list_pop_back에서 lru로 바꿔야함.
	struct frame_e *fe = list_entry(e,struct frame_e,elem);

	palloc_free_page(fe->kaddr);	
}
