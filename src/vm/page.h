#ifndef PAGE_HEADER
#define PAGE_HEADER
/*
#include <hash.h>
#include <threads/thread.h>

struct spt_e{
	void *vaddr;
	size_t page_read_bytes;
	size_t page_zero_bytes;
	bool writable;
	struct file* file;
	struct hash_elem elem;
	size_t ofs;
	int swap_slot;
};

unsigned hash_value(const struct hash_elem* e,void *aux);

bool hash_compare(const struct hash_elem *a,const struct hash_elem *b,void *aux);

void add_spte(void* upage,size_t page_read_bytes,size_t page_zero_bytes,bool writable,struct file* file,size_t ofs);*/

#include "vm/swap.h"
#include <hash.h>
#include "filesys/off_t.h"

enum page_status{
	ALL_ZERO,
	ON_FRAME,
	ON_SWAP,
	FROM_FILESYS
};

struct supplemental_page_table{
	struct hash page_map;
};
struct supplemental_page_table_entry{
	void *upage;
	void *kpage;

	struct hash_elem elem;

	enum page_status status;

	bool dirty;

	int swap_index;

	struct file* file;
	off_t file_offset;
	int read_bytes,zero_bytes;
	bool writable;
};

struct supplemental_page_table* supt_create(void);
void supt_destroy(struct supplemental_page_table *);

bool supt_install_frame(struct supplemental_page_table *supt,void *upage,void *kpage);
bool supt_install_zeropage(struct supplemental_page_table *supt,void *);
bool supt_set_swap(struct supplemental_page_table* supt,void *,int);
bool supt_install_filesys(struct supplemental_page_table *supt,void *page,
		struct file* file,off_t offset,int read_bytes,int zero_bytes,bool writable);

struct supplemental_page_table_entry* supt_lookup(struct supplemental_page_table *supt,void *);
bool supt_has_entry(struct supplemental_page_table *,void *page);

bool supt_set_dirty(struct supplemental_page_table *,void*, bool);

bool load_page(struct supplemental_page_table *supt,int *pagedir,void *upage);

void pin_page(struct supplemental_page_table *supt, void *page);
void unpin_page(struct supplemental_page_table *supt, void *page);

#endif
