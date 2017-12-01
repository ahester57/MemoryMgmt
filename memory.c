#include <stdlib.h>
#include <stdio.h>
#include "osstypes.h"
#include "memory.h"

// second chance algorithm
int
secondchance(page_table* pagetable)
{
	int pagetoreplace = 0;
	int i;
	for (i = 0; i < NUMPAGES; i++) {

	}
	return pagetoreplace;
}

// check if request from pnum <= available
int
requestavailable(page_table* pagetable, const int req,
		 const int pnum, const int resnum)
{
	page_frame page = pagetable->table[resnum];
	if (page.valid_bit) {
		return 1;
	}
	return 0;
}

// adds a request for a resource from pnum
int
requestpage(page_table* pagetable, const int reqnum,
		const unsigned int pnum)
{
	page_frame* tab = pagetable->table;
	page_frame page = tab[reqnum]; 
	//int index = findavailableslot(res.requests);
	int index = -1;
	if (index == -1)
		return -1;
	//res.requests[index] = pnum;
	pagetable->table[reqnum] = page;
	return 1;
}

// checks if it is allocated the resource
int
findinallocated(page_table* table, const int reqnum,
		const unsigned int pnum)
{
	//page_frame* tab = table->table;
	//page_frame page = tab[reqnum]; 
	//unsigned int* list = res.allocation;
	int i;
	for (i = 0; i < MAXPROCESSES; i++) {
	//	if (list[i] == pnum) {
	//		return 1;
	//	}
	}
	return -1;
}

// returns first available slot in given list
int
findavailableslot(unsigned int* list)
{
	int i;
	for (i = 0; i < MAXPROCESSES; i++) {
		if (list[i] == -1) {
			return i;
		}
	}
	return -1;
}
