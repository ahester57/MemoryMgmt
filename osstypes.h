#ifndef OSSTYPES_H_
#define OSSTYPES_H_

#define LINESIZE 256
#define BILLION 1000000000
#define MAXPROCESSES 18
#define NUMPAGES 256 
#define PAGESIZE 1024

// for message queues
typedef struct
{
	int mtype;
	unsigned int proc_id;
	char mtext[LINESIZE];
} mymsg_t;

// oss simulated clock
typedef struct
{
	unsigned int sec;
	unsigned int nsec;
} oss_clock_t;

// process control block
typedef struct
{
	unsigned int proc_id;
	unsigned int used_cpu_time;
	unsigned int system_total_time;
	unsigned int quantum;
	int done;
} pxs_cb_t;

typedef struct
{
	char data[PAGESIZE];
	int ref_bit;
	int dirty_bit;
	int valid_bit;
} page_frame;

typedef struct
{
	page_frame table[NUMPAGES];
} page_table;

#endif
