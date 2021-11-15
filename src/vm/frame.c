/*
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
	
	swap_to_disk(swap_slot,fe->kaddr);
	//swap_to_disk(swap_slot,fe->spte->vaddr);
	fe->spte->swap_slot = swap_slot;
	list_remove(e);

	//pagedir_clear_page(fe->t->pagedir,fe->spte->vaddr);
	palloc_free_page(fe->kaddr);	
}*/

#include <hash.h>
#include <list.h>
#include <stdio.h>

#include "vm/frame.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"

static struct lock frame_lock;
static struct hash frame_map;

static struct list frame_list;
static struct list_elem *clock_ptr;

static unsigned frame_hash_func(const struct hash_elem *elem,void *aux);
static bool frame_less_func(const struct hash_elem*,const struct hash_elem *,void *aux);

struct frame_table_entry{
	void *kpage;

	struct hash_elem helem;
	struct list_elem lelem;

	void *upage;
	struct thread *t;

	bool pinned;
};

static struct frame_table_entry* pick_frame_to_evict(int* pagedir);
static void frame_do_free(void *kpage,bool free_page);

void frame_init(){
	lock_init(&frame_lock);
	hash_init(&frame_map,frame_hash_func,frame_less_func,NULL);
	list_init(&frame_list);
	clock_ptr = NULL;
}

void * frame_allocate(enum palloc_flags flags, void* upage){
	lock_acquire(&frame_lock);

	void *frame_page = palloc_get_page(PAL_USER|flags);
	if(frame_page == NULL){
		struct frame_table_entry *f_evicted = pick_frame_to_evict(thread_current()->pagedir);
	
		ASSERT(f_evicted != NULL && f_evicted->t != NULL);
	
		ASSERT(f_evicted->t->pagedir != (void*)0xcccccccc);
		pagedir_clear_page(f_evicted->t->pagedir,f_evicted->upage);

		bool is_dirty = false;
		is_dirty = is_dirty || pagedir_is_dirty(f_evicted->t->pagedir, f_evicted->upage);
		is_dirty = is_dirty || pagedir_is_dirty(f_evicted->t->pagedir, f_evicted->kpage);
	
		int swap_index = swap_out(f_evicted->kpage);
		supt_set_swap(f_evicted->t->supt,f_evicted->upage, swap_index);
		supt_set_dirty(f_evicted->t->supt,f_evicted->upage,is_dirty);
		frame_do_free(f_evicted->kpage,true);

		frame_page = palloc_get_page(PAL_USER | flags);
		ASSERT(frame_page != NULL);
	}
	struct frame_table_entry *frame = malloc(sizeof(struct frame_table_entry));
	if(frame == NULL){
		lock_release(&frame_lock);
		return NULL;
	}

	frame->t = thread_current();
	frame->upage = upage;
	frame->kpage = frame_page;
	frame->pinned = true;

	hash_insert(&frame_map,&frame->helem);
	list_push_back(&frame_list,&frame->lelem);

	lock_release(&frame_lock);
	return frame_page;
}

void frame_free(void *kpage){
	lock_acquire(&frame_lock);
	frame_do_free(kpage,true);
	lock_release(&frame_lock);
}

void frame_remove_entry(void* kpage){
	lock_acquire(&frame_lock);
	frame_do_free(kpage,false);
	lock_release(&frame_lock);
}

void frame_do_free(void *kpage,bool free_page){
	ASSERT(lock_held_by_current_thread(&frame_lock) == true);
	ASSERT(is_kernel_vaddr(kpage));
	ASSERT(pg_ofs(kpage) == 0);
	
	struct frame_table_entry f_tmp;
	f_tmp.kpage = kpage;

	struct hash_elem *h = hash_find(&frame_map,&(f_tmp.helem));
	if(h == NULL){
		PANIC("The page to be freed is not stored in the table");
	}

	struct frame_table_entry *f;
	f = hash_entry(h,struct frame_table_entry,helem);

	hash_delete(&frame_map,&f->helem);
	list_remove(&f->lelem);

	if(free_page)
		palloc_free_page(kpage);
	free(f);
}

struct frame_table_entry* clock_frame_next(void);
struct frame_table_entry* pick_frame_to_evict(int *pagedir)
{
	int n = hash_size(&frame_map);
	if(n == 0) PANIC("Frame table is empty, can't happen - there is a leak somewhere");

	int it;
	for(it = 0; it <= n + n; it++){
		struct frame_table_entry *e = clock_frame_next();
		if(e->pinned) continue;
		else if(pagedir_is_accessed(pagedir,e->upage)){
			pagedir_set_accessed(pagedir,e->upage,false);
			continue;
			}
		return e;
	}

	PANIC("Can't evict any frame");
}
struct frame_table_entry* clock_frame_next(void)
{
	if(list_empty(&frame_list))
		PANIC("Frame table is empty, can't happen");
	if(clock_ptr == NULL || clock_ptr == list_end(&frame_list))
		clock_ptr = list_begin(&frame_list);
	else
		clock_ptr = list_next(clock_ptr);

	struct frame_table_entry *e = list_entry(clock_ptr,struct frame_table_entry,lelem);
	return e;
}
static void frame_set_pinned(void *kpage, bool new_value){
	lock_acquire(&frame_lock);

	struct frame_table_entry f_tmp;
	f_tmp.kpage = kpage;
	struct hash_elem *h = hash_find(&frame_map,&(f_tmp.helem));
	if(h == NULL){
		PANIC("The frame to be pinned/unpinned does not exist");
	}
	struct frame_table_entry *f;
	f = hash_entry(h,struct frame_table_entry,helem);
	f->pinned = new_value;

	lock_release(&frame_lock);
}
void frame_unpin(void *kpage){
	frame_set_pinned(kpage,false);
}
void frame_pin(void *kpage){
	frame_set_pinned(kpage,true);
}

static unsigned frame_hash_func(const struct hash_elem *elem, void *aux UNUSED)
{
	struct frame_table_entry *entry = hash_entry(elem,struct frame_table_entry, helem);
	return hash_bytes(&entry->kpage,sizeof(entry->kpage));
}
static bool frame_less_func(const struct hash_elem *a,const struct hash_elem *b,void *aux UNUSED){
	struct frame_table_entry *a_entry = hash_entry(a,struct frame_table_entry,helem);
	struct frame_table_entry *b_entry = hash_entry(b,struct frame_table_entry,helem);

	return a_entry->kpage < b_entry->kpage;
}
