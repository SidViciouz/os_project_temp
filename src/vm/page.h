#ifndef PAGE_HEADER
#define PAGE_HEADER
#include <hash.h>

struct spt_e{
	void *vaddr;
	size_t page_read_bytes;
	size_t page_zero_bytes;
	bool writable;
	struct file* file;
	struct hash_elem elem;
	size_t ofs;
};

unsigned hash_value(const struct hash_elem* e,void *aux);

bool hash_compare(const struct hash_elem *a,const struct hash_elem *b,void *aux);
#endif
