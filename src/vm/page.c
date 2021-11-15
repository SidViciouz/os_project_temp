/*
#include "page.h"

unsigned hash_value(const struct hash_elem* e,void *aux)
{
	const struct spt_e *entry = hash_entry(e,struct spt_e,elem);

	return hash_bytes(&entry->vaddr,sizeof(entry->vaddr));
}

bool hash_compare(const struct hash_elem *a,const struct hash_elem *b,void *aux)
{
	const struct spt_e *e1 = hash_entry(a,struct spt_e,elem);
	const struct spt_e *e2 = hash_entry(b,struct spt_e,elem);

	return e1->vaddr < e2->vaddr;
}
void add_spte(void* upage,size_t page_read_bytes,size_t page_zero_bytes,bool writable,struct file* file,size_t ofs){
	struct spt_e *spte = (struct spte *)malloc(sizeof(struct spt_e)); //insert spte
	spte->vaddr = upage;
	spte->page_read_bytes = page_read_bytes;
	spte->page_zero_bytes = page_zero_bytes;
 	spte->writable = writable;
 	spte->file = file;
 	spte->ofs = ofs;
	spte->swap_slot = -1;
	hash_insert(&thread_current()->spt,&(spte->elem));
}*/

#include <hash.h>
#include <string.h>

#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "filesys/file.h"

static unsigned spte_hash_func(const struct hash_elem *elem, void *aux);
static bool spte_less_func(const struct hash_elem *,const struct hash_elem*, void* aux);
static void spte_destroy_func(struct hash_elem *elem, void *aux);

struct supplemental_page_table* supt_create(void){
	struct supplemental_page_table *supt =
		(struct supplemental_page_table*)malloc(sizeof(struct supplemental_page_table));
	hash_init(&supt->page_map,spte_hash_func,spte_less_func,NULL);
	return supt;
}
void supt_destroy(struct supplemental_page_table *supt)
{
	ASSERT(supt != NULL);
	hash_destroy(&supt->page_map,spte_destroy_func);
	free(supt);
}

bool supt_install_frame(struct supplemental_page_table *supt, void *upage, void *kpage){
	struct supplemental_page_table_entry *spte;
	spte = (struct supplemental_page_table_entry *)malloc(sizeof(struct supplemental_page_table_entry));

	spte->upage = upage;
	spte->kpage = kpage;
	spte->status = ON_FRAME;
	spte->dirty = false;
	spte->swap_index = -1;

	struct hash_elem *prev_elem;
	prev_elem = hash_insert(&supt->page_map,&spte->elem);
	if(prev_elem == NULL){
		return true;
	}
	else{
		free(spte);
		return false;
	}
}
bool supt_install_zeropage(struct supplemental_page_table *supt, void* upage){	
	struct supplemental_page_table_entry *spte;
	spte = (struct supplemental_page_table_entry *)malloc(sizeof(struct supplemental_page_table_entry));
	spte->upage = upage;
	spte->kpage = NULL;
	spte->status = ALL_ZERO;
	spte->dirty = false;

	struct hash_elem *prev_elem;
	prev_elem = hash_insert(&supt->page_map,&spte->elem);
	if(prev_elem == NULL) return true;

	PANIC("Duplicated SUPT entry for zeropage");
	return false;
}
bool supt_set_swap(struct supplemental_page_table *supt,void *page,int swap_index){
	struct supplemental_page_table_entry *spte;
	spte = supt_lookup(supt,page);
	if(spte == NULL) return false;

	spte->status = ON_SWAP;
	spte->kpage = NULL;
	spte->swap_index = swap_index;
	return true;
}
bool supt_install_filesys(struct supplemental_page_table *supt,void *upage,
		struct file* file,off_t offset,int read_bytes,int zero_bytes,bool writable)
{
	struct supplemental_page_table_entry *spte;

	spte = (struct supplemental_page_table_entry *)malloc(sizeof(struct supplemental_page_table_entry));

	spte->upage = upage;
	spte->kpage = NULL;
	spte->status = FROM_FILESYS;
	spte->dirty = false;
	spte->file = file;
	spte->file_offset = offset;
	spte->read_bytes = read_bytes;
	spte->zero_bytes = zero_bytes;
	spte->writable = writable;
	
	struct hash_elem *prev_elem;
	prev_elem = hash_insert(&supt->page_map,&spte->elem);
	if(prev_elem == NULL) return true;

	PANIC("Duplicated SUPT entry for filesys-page");
	return false;
}

struct supplemental_page_table_entry *supt_lookup(struct supplemental_page_table *supt,void *page){
	struct supplemental_page_table_entry spte_temp;
	spte_temp.upage = page;

	struct hash_elem *elem = hash_find(&supt->page_map,&spte_temp.elem);
	if(elem == NULL) return NULL;
	return hash_entry(elem,struct supplemental_page_table_entry,elem);
}

bool supt_has_entry(struct supplemental_page_table *supt,void *page){
	struct supplemental_page_table_entry *spte = supt_lookup(supt,page);
	if(spte == NULL) return false;

	return true;
}
bool supt_set_dirty(struct supplemental_page_table *supt,void *page,bool value){
	struct supplemental_page_table_entry *spte = supt_lookup(supt,page);
	if(spte == NULL) PANIC("set dirty - the request page doesn't exist");

	spte->dirty = spte->dirty || value;
	return true;
}

static bool load_page_from_filesys(struct supplemental_page_table_entry *spte,void *kpage);

bool load_page(struct supplemental_page_table *supt,int *pagedir, void *upage){
	struct supplemental_page_table_entry *spte;
	spte = supt_lookup(supt,upage);
	if(spte == NULL)
		return false;

	if(spte->status == ON_FRAME)
		return true;

	void *frame_page = frame_allocate(PAL_USER,upage);
	if(frame_page == NULL)
		return false;

	bool writable = true;
	switch(spte->status)
	{
		case ALL_ZERO:
			memset(frame_page,0,PGSIZE);
			break;
		case ON_FRAME:
			break;
		case ON_SWAP:
			swap_in(spte->swap_index,frame_page);
			break;
		case FROM_FILESYS:
			if(load_page_from_filesys(spte,frame_page) == false){
				frame_free(frame_page);
				return false;
			}
			writable = spte->writable;
			break;
		default:
			PANIC("unreachable state");

	}
	
	if(!pagedir_set_page(pagedir,upage,frame_page,writable)){
		frame_free(frame_page);
		return false;
	}

	spte->kpage = frame_page;
	spte->status = ON_FRAME;

	pagedir_set_dirty(pagedir,frame_page,false); //frame_page아니고 upage아닌가
	
	frame_unpin(frame_page);

	return true;
}

static bool load_page_from_filesys(struct supplemental_page_table_entry *spte,void *kpage){
	file_seek(spte->file,spte->file_offset);

	int n_read = file_read(spte->file,kpage,spte->read_bytes);
	if(n_read != (int)spte->read_bytes)
		return false;

	ASSERT(spte->read_bytes + spte->zero_bytes == PGSIZE);
	memset(kpage+n_read,0,spte->zero_bytes);
	return true;
}

void pin_page(struct supplemental_page_table *supt,void *page){
	struct supplemental_page_table_entry *spte;
	spte = supt_lookup(supt,page);
	if(spte == NULL)
		return;
	ASSERT(spte->status == ON_FRAME);
	frame_pin(spte->kpage);
}
void unpin_page(struct supplemental_page_table *supt, void *page){

	struct supplemental_page_table_entry *spte;

	spte = supt_lookup(supt,page);
	if(spte == NULL) PANIC("request page is non-exist");

	if(spte->status == ON_FRAME)
		frame_unpin(spte->kpage);
}

static unsigned spte_hash_func(const struct hash_elem *elem, void *aux UNUSED){
	struct supplemental_page_table_entry *entry = hash_entry(elem,struct supplemental_page_table_entry,elem);
	return hash_int((int)entry->upage);
}

static bool spte_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED){
	struct supplemental_page_table_entry *a_entry = hash_entry(a,struct supplemental_page_table_entry,elem);
	struct supplemental_page_table_entry *b_entry = hash_entry(b,struct supplemental_page_table_entry,elem);
	return a_entry->upage < b_entry->upage;
}

static void spte_destroy_func(struct hash_elem *elem,void *aux UNUSED){

	struct supplemental_page_table_entry *entry = hash_entry(elem,struct supplemental_page_table_entry,elem);

	if(entry->kpage != NULL){
		ASSERT(entry->status == ON_FRAME);
		frame_remove_entry(entry->kpage);
	}
	else if(entry->status == ON_SWAP){
		swap_free(entry->swap_index);
	}

	free(entry);
}
