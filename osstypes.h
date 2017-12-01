#ifndef OSSTYPES_H_
#define OSSTYPES_H_

#define LINESIZE 256
#define BILLION 1000000000
#define MAXPROCESSES 18
#define NUMRESOURCES 20

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

// resource descriptor
typedef struct
{
	unsigned int requests[MAXPROCESSES];
	unsigned int allocation[MAXPROCESSES];
	unsigned int release[MAXPROCESSES];
	int issharable;
	int instances;
	int available;
} resource_dt;

// resource table
typedef struct
{
	resource_dt table[NUMRESOURCES];	
} resource_table;

typedef struct
{
	char data[1024];
} page_frame;

typedef struct
{
	page_frame tabele[256];
} page_table;

#endif
