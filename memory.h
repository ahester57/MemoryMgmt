#ifndef MEMORY_H_
#define MEMORY_H_


int secondchance(page_table* pagetable);
int requestavailable(page_table* table, const int req,
			 const int pnum, const int resnum);
int requestpage(page_table* table, const int reqnum,
			const unsigned int pnum);
int findinallocated(page_table* table, const int reqnum,
			const unsigned int pnum);
int findavailableslot(unsigned int* list);

#endif
