/*
$Id: user.c,v 1.3 2017/11/13 04:00:18 o1-hester Exp o1-hester $
$Date: 2017/11/13 04:00:18 $
$Revision: 1.3 $
$Log: user.c,v $
Revision 1.3  2017/11/13 04:00:18  o1-hester
soon to be

Revision 1.2  2017/10/26 03:30:40  o1-hester
glad its over

Revision 1.1  2017/10/23 07:27:24  o1-hester
Initial revision

Revision 1.5  2017/10/11 20:32:12  o1-hester
turnin

Revision 1.4  2017/10/10 20:19:24  o1-hester
cleanup

Revision 1.3  2017/10/09 22:00:49  o1-hester
*** empty log message ***

Revision 1.2  2017/10/04 07:55:28  o1-hester
clock set up

Revision 1.1  2017/10/04 07:44:46  o1-hester
Initial revision

$Author: o1-hester $
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include "osstypes.h"
#include "ipchelper.h"
#include "sighandler.h"
#include "memory.h"
#define BOUND 250000
#define FILEPERMS (O_WRONLY | O_CREAT)

// id and operations for semaphores
int semid;
struct sembuf mutex[2];
struct sembuf msgsignal[1];
struct sembuf logmutex[2];

oss_clock_t* initsharedclock(const key_t shmkey);
page_table* initsharedtable(const key_t diskey);
int sem_wait();
int sem_signal();
int log_wait();
int log_signal();
int initsemaphores(const key_t skey);
int initsighandler();

// Child process spawn by oss 
// Each child is assigned a quantum
// It decides whether to use entire or partial quantum
// It then adds this to the current time
int
main (int argc, char** argv)
{
	// get keys from file
	key_t mkey, skey, shmkey, diskey;
	mkey = ftok(KEYPATH, MSG_ID);
	skey = ftok(KEYPATH, SEM_ID);
	shmkey = ftok(KEYPATH, SHM_ID);
	diskey = ftok(KEYPATH, SHM_ID*2);
	if (mkey == -1 || skey == -1 || shmkey == -1 || diskey == -1) {
		perror("USER: Failed to retreive keys.");
		return 1;
	}

	/*************** Set up signal handler ********/
	if (initsighandler() == -1) {
		perror("USER: Failed to set up signal handlers");
		return 1;
	}

	long pid = (long)getpid();
	fprintf(stderr, "USER: %ld begin.\n", pid);
	/***************** Set up shared memory *******/
	// Child has read-only permissions on shm
	oss_clock_t* clock = initsharedclock(shmkey);
	if (clock == (void*)-1) {
		perror("USER: Failed to attach shared memory.");	
		return 1;
	}
	page_table* table = initsharedtable(diskey);
	if (table == (void*)-1) {
		perror("USER: Failed to attach shared memory.");	
		return 1;
	}

	/***************** Set up semaphopage ***********/
	if (initsemaphores(skey) == -1) {
		perror("USER: Failed to setup semaphores.");
		return 1;
	}

	/**************** Set up message queue *********/
	// for writing
	int msgid = msgget(mkey, PERM);
	if (msgid == -1) {
		if (errno == EIDRM) {
			perror("USER: Interrupted");
			return 1;
		}
		perror("USER: Failed to create message queue.");
		return 1;
	}

	// seed random 
	struct timespec tm;
	clock_gettime(CLOCK_MONOTONIC, &tm);
	srand((unsigned)(tm.tv_sec ^ tm.tv_nsec ^ (tm.tv_nsec >> 31)));

	// open log file
	char* fname = "child.log";
	int logf = open(fname, FILEPERMS, PERM | O_SYNC);
	if (logf == -1) {
		perror("OSS: Could not open log file:");
		return 1;
	}

	// flags
	int complete = 0;
	int overonesec = 0;
	char* msg;
	// clock stuff
	long int quantum = BILLION * 2;
	oss_clock_t start;
	start.sec = clock->sec;
	start.nsec = clock->nsec;
	oss_clock_t end = calcendtime(*clock, quantum);
	oss_clock_t nextreq = calcendtime(start, BILLION / 4);

	if (log_wait() == -1) {
		return 1;
	}
	fprintf(stderr,"USER:[%ld] start:%d,%d\n",pid,start.sec,start.nsec);
	//dprintf(logf,"USER:[%ld] start:%d,%d\n",pid,start.sec,start.nsec);
	if (log_signal() == -1) {
		return 1;
	}

	/************** start loop ********************/
	do {

	if (clock == NULL || table == NULL) {
		perror("USER: Shared memory corrupt.");
		return 1;
	}


	/********* Wait for you time ******/
	
	while (!((nextreq.sec <= clock->sec && nextreq.nsec <= clock->nsec)
		|| (nextreq.sec < clock->sec))) {};

	/**************** Child loop **************/
	// begin looping over critical section
	// where each child checks the clock in shmem
	
	/************ Entry section ***************/	
	// wait until your turn

	if (sem_wait() == -1) {
		// failed to lock shm
		return 1;
	}

	//printf("yooooooo\n");
	// request and address
	int address = (int) rand() % (NUMPAGES * PAGESIZE);
	// Block all signals during critical section
	sigset_t newmask, oldmask; 	
	if ((sigfillset(&newmask) == -1) ||
		(sigprocmask(SIG_BLOCK, &newmask, &oldmask) == -1)) {
		perror("CHILD: Failed setting signal mask.");
		return 1;
	} 
	/************ Critical section ***********/
	// set flag when 1 second is up
	if ((end.sec <= clock->sec && end.nsec <= clock->nsec)
		|| (end.sec < clock->sec)) {
		// child's time is up
		overonesec = 1;
		if (log_wait() == -1) {
			return 1;
		}
		msg = (char*)malloc(sizeof(char) * 32);
		if (msg == NULL) {
			perror("");
			return 1;
		}
		sprintf(msg, "USER: [%ld] second up\n", pid);
		fprintf(stderr, msg);
		write(logf, msg, 25);
		fsync(logf);
		free(msg);
		if (log_signal() == -1) {
			return 1;
		}
	}
	//printf("yooo1ooo\n");
	// request some page
	if ((nextreq.sec <= clock->sec && nextreq.nsec <= clock->nsec)
		|| (nextreq.sec < clock->sec)) {

		if (requestpage(table, address % 256, pid) == -1) {
			//fprintf(stderr,"Failed to request page.\n");
		}
		if (log_wait() == -1) {
			return 1;
		}
		msg = (char*)malloc(sizeof(char) * 64);
		sprintf(msg, "USER: [%ld] request byte: %d\n\0",pid,address);
		fprintf(stderr, msg);
		float a = address / 100000;
		int p;
		if (a >= 1.0) {
			p = 7;
		} else {
			a = address % 100000 / 10000;
			if (a >= 1)
				p = 6;
			else {
				a = address % 10000 / 1000;
				if (a >= 1)
					p = 5;
			}
		}
		int asd = 27 + p;
		write(logf, msg, asd + 3);
		fsync(logf);
		free(msg);
		if (log_signal() == -1) {
			return 1;
		}
		nextreq = calcendtime(*clock, BILLION / 4);

	}
	//printf("yoooooo2o\n");
	/*********** Exit section **************/
	// unlock shared memory read 
	if (sem_signal() == -1) { 		
		if (errno == EINVAL) {
			char* m0 = "finished critical section after signal";
			fprintf(stderr, "CHILD: %ld %s\n", pid, m0);
			return 1;
		}
		return 1;
	}
	// Unblock signals after critical sections
	if ((sigprocmask(SIG_BLOCK, &newmask, &oldmask) == -1)) {
		perror("CHILD: Failed setting signal mask.");
		return 1;
	} 

	// decide if terminate after 1s
	if (overonesec) {
		complete = (int)(rand()) % 2;
		if (complete) {
			if (log_wait() == -1) {
				return 1;
			}
			msg = (char*)malloc(sizeof(char) * 32);
			sprintf(msg, "USER: [%ld] I'm done.\n", pid);
			fprintf(stderr, msg);
			write(logf, msg, 25);
			fsync(logf);
			free(msg);
			if (log_signal() == -1) {
				return 1;
			}
			sendmessage(msgid, pid, end, *clock);
			// signal parent that a new message is available
			if (semop(semid, msgsignal, 1) == -1) {
				perror("CHILD: Failed to signal parent.");
				return 1;	
			}
			break;	// out of main do{}while
		}
	}

	} while (!complete); // end whole do{}while
	

	// detach shared memory
	if (shmdt(table) == -1 || shmdt(clock) == -1) {
		perror("USER: Failed to detach shared memory.");
		return 1;
	}


 	if (errno != 0) {
		perror("CHILD: uncaught error:");
		return 1;
	}
	return 0;
}

oss_clock_t*
initsharedclock(const key_t shmkey)
{
	int shmid = getclockshmid_ro(shmkey);
	if (shmid == -1) {
		return (void*)-1;
	}
	return attachshmclock(shmid);
}

page_table*
initsharedtable(const key_t diskey)
{
	int shmid = gettableshmid(diskey);
	if (shmid == -1) {
		return (void*)-1;
	}
	return attachshmtable(shmid);
}

/***** NOTE about semaphopage *****/
/* I do NOT log every time sem is waited on
 * and acquired. That would be ridiculous. Thanks. */ 

// Semaphore blocking wait, return -1 on error
int
sem_wait()
{
	if (semop(semid, mutex, 1) == -1) {
		if (errno == EIDRM) {
			perror("CHILD: Interrupted");
			return -1;
		}
		perror("CHILD: Failed to lock shared memory.");
		return -1;	
	}
	return 0;
}

// Semaphore signal, return -1 on error
int
sem_signal()
{
	if (semop(semid, mutex+1, 1) == -1) { 		
		perror("CHILD: Failed to unlock semid.");
		return -1;
	}
	return 0;
}

// Semaphore blocking wait, return -1 on error
int
log_wait()
{
	if (semop(semid, logmutex, 1) == -1) {
		if (errno == EIDRM) {
			perror("CHILD: Interrupted");
			return -1;
		}
		perror("CHILD: Failed to lock shared memory.");
		return -1;	
	}
	return 0;
}

// Semaphore signal, return -1 on error
int
log_signal()
{
	if (semop(semid, logmutex+1, 1) == -1) { 		
		perror("CHILD: Failed to unlock semid.");
		return -1;
	}
	return 0;
}

// initialize semaphore operations
int
initsemaphores(const key_t skey)
{
	semid = semget(skey, 4, PERM);
	if (semid == -1) {
		if (errno == EIDRM) {
			perror("CHILD: Interrupted");
			return -1;
		}
		perror("CHILD: Failed to set up semaphore.");
		return -1;
	}
	// mutex for reading from shared memory
	setsembuf(mutex, 0, -1, 0);
	setsembuf(mutex+1, 0, 1, 0);
	// msgsignal for letting parent know when message is available
	setsembuf(msgsignal, 2, 1, 0);
	setsembuf(logmutex, 3, -1, 0);
	setsembuf(logmutex+1, 3, 1, 0);
	return 0;
}
// initialize signal handler, return -1 on error
int
initsighandler()
{
	struct sigaction act = {{0}};
	act.sa_handler = catchuserintr;
	act.sa_flags = 0;
	if ((sigemptyset(&act.sa_mask) == -1) ||
	    (sigaction(SIGINT, &act, NULL) == -1) ||
	    (sigaction(SIGALRM, &act, NULL) == -1) ||
	    (sigaction(SIGUSR1, &act, NULL) == -1)) {
		return -1;
	}	
	return 0;
}
