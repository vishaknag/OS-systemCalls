// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"
#include "synch.h"

#define MAX_LOCKS 500
#define MAX_CVS 500
#define MAX_PROCESSES 50
#define MAX_THREADS 500

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;	// user program memory and registers
extern Lock* kernelLockListLock;	// Kernel lock for user program Lock list
extern Lock* kernelCVListLock;		// Kernel lock for user program CV list 
extern Lock* processTableLock;		// Kernel lock for process table 
extern Lock *pageTableLock[MAX_PROCESSES];	// Kernel Locks for pagetables of all the processes
extern Lock *activeProcessesLock;		// shared by exec and exit syscalls


extern int fifoFlag;
extern int randFlag;
// List of Locks Maintained by the Kernel
typedef struct
{
	Lock *lock;
	AddrSpace *lockAddrSpace;
	int usageCounter;
	bool toBeDestroyed;
}KernelLock;
extern KernelLock lockList[MAX_LOCKS];	// List of user locks maintained by kernel

// List of CVs Maintained by the Kernel
typedef struct
{
	Condition *condition;
	AddrSpace *conditionAddrSpace;
	int usageCounter;
	bool toBeDestroyed;
}KernelCV;
extern KernelCV CVList[MAX_CVS];	// List of user CVs maintained by kernel

// Structure representing the process table 
typedef struct
{
	int Pid;
	int threadCount;
	AddrSpace *addrSpace;
	int activeThreadCount;
	int stackTops[MAX_THREADS];
}Process;
extern Process processTable[MAX_PROCESSES];

extern unsigned int nextLockIndex ;
extern unsigned int nextCVIndex;
extern int processCount;
extern int noOfActiveProcesses;
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#ifdef CHANGED
// creating a bitmap object 
extern BitMap* bitMapObject; 
extern Lock* bitMapObjectLock;
#endif 

#endif // SYSTEM_H
