#include "page.h"

unsigned hash_value(const struct hash_elem* e,void *aux)
{
	const struct spt_e *entry = hash_entry(e,struct spt_e,elem);

	return hash_bytes(&entry->vaddr,sizeof(entry->vaddr));
	//return *((unsigned*)entry->vaddr)%16;
}

bool hash_compare(const struct hash_elem *a,const struct hash_elem *b,void *aux)
{
	const struct spt_e *e1 = hash_entry(a,struct spt_e,elem);
	const struct spt_e *e2 = hash_entry(b,struct spt_e,elem);

	return e1->vaddr < e2->vaddr;
	//return *((unsigned*)(e1->vaddr)) <  *((unsigned*)(e2->vaddr));
}
