#ifndef PAGE_HEADER
#define PAGE_HEADER
#include <hash.h>

struct spt_e{
	void *vaddr;
	struct hash_elem elem;
};

unsigned hash_value(const struct hash_elem* e,void *aux);

bool hash_compare(const struct hash_elem *a,const struct hash_elem *b,void *aux);
#endif
