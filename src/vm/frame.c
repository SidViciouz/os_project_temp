#include "frame.h"
#include "swap.h"

static struct list frame_list;


void init_frame_list(void)
{
	list_init(&frame_list);
}

void insert_frame_e(struct list_elem* e)
{
	list_push_back(&frame_list,e);
}

void free_frame(int swap_slot)
{
	//dirty bit에 따라 swap slot에 swap in하는거 추가해야함
	struct list_elem *e = list_pop_back(&frame_list); //list_pop_back에서 lru로 바꿔야함.
	struct frame_e *fe = list_entry(e,struct frame_e,elem);
/*
//	printf("enter\n");
	e = list_begin(&frame_list);
	while(1){
		fe = list_entry(e,struct frame_e,elem);
		if(pagedir_is_accessed(fe->t->pagedir,fe->spte->vaddr)){
			//printf("1\n");
			pagedir_set_accessed(fe->t->pagedir,fe->spte->vaddr,0);	
		}
		
		else{
			//printf("2\n");
			break;
		}
	

		if(list_next(e) == list_end(&frame_list)){
			//printf("3\n");
			e = list_begin(&frame_list);
		}
		
		else{
			//printf("4\n");
			e = list_next(e);
		}
	}
	//printf("%d\n",swap_slot);
	*/
	swap_to_disk(swap_slot,fe->kaddr);
	//swap_to_disk(swap_slot,fe->spte->vaddr);
	fe->spte->swap_slot = swap_slot;
	list_remove(e);

	//pagedir_clear_page(fe->t->pagedir,fe->spte->vaddr);
	palloc_free_page(fe->kaddr);	
}
