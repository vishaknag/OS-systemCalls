// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "noff.h"
#include <stdio.h>
#include <iostream>
#define MAX_CHARLEN 50

using namespace std;
int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
      result = machine->ReadMem( vaddr, 1, paddr );
      while(!result) // FALL 09 CHANGES
	  {
   			result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
	  }	
      
      buf[n++] = *paddr;
     
      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    delete paddr;
    return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;			// The number of bytes copied in

    while ( n >= 0 && n < len) {
      // Note that we check every byte's address
      result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );

      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf) return;

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Create\n");
	delete buf;
	return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *f;			// The new open file
    int id;				// The openfile id

    if (!buf) {
	printf("%s","Can't allocate kernel buffer in Open\n");
	return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Open\n");
	delete[] buf;
	return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if ( f ) {
	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
	    delete f;
	return id;
    }
    else
	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.
    
    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output

    if ( id == ConsoleInput) return;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf;
	    return;
	}
    }

    if ( id == ConsoleOutput) {
      for (int ii=0; ii<len; ii++) {
	printf("%c",buf[ii]);
      }

    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    f->Write(buf, len);
	} else {
	    printf("%s","Bad OpenFileId passed to Write\n");
	    len = -1;
	}
    }

    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;		// Kernel buffer for input
    OpenFile *f;	// Open file for output

    if ( id == ConsoleOutput) return -1;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer in Read\n");
	return -1;
    }

    if ( id == ConsoleInput) {
      //Reading from the keyboard
      scanf("%s", buf);

      if ( copyout(vaddr, len, buf) == -1 ) {
	printf("%s","Bad pointer passed to Read: data not copied\n");
      }
    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    len = f->Read(buf, len);
	    if ( len > 0 ) {
	        //Read something from the file. Put into user's address space
  	        if ( copyout(vaddr, len, buf) == -1 ) {
		    printf("%s","Bad pointer passed to Read: data not copied\n");
		}
	    }
	} else {
	    printf("%s","Bad OpenFileId passed to Read\n");
	    len = -1;
	}
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
    OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

    if ( f ) {
      delete f;
    } else {
      printf("%s","Tried to close an unopen file\n");
    }
}

int CreateLock_Syscall(unsigned int vaddr, int len) {
	// Allocates a LOCK from the kernel lock list to the user prog
	
	int rv = -1;	// index value of the Lock created
	char *lockName = new char[len+1];
	
	if((vaddr+len) <= vaddr){
		printf("%s", "CreateLock_Syscall:Invalid Virtual address length\n");
		delete[] lockName;
		return -1;
	}	
	
	if((vaddr < 0) || (vaddr > currentThread->space->spaceSize)){
		printf("%s", "CreateLock_Syscall:Invalid virtual address\n");
		delete[] lockName;
		return -1;
	}	
	
	if((vaddr + len) > currentThread->space->spaceSize){
		printf("%s", "CreateLock_Syscall:Trying to access Invalid Virtual address.\n");
		delete[] lockName;
		return -1;
	}	
	
	if(len > MAX_CHARLEN){
		printf("%s", "CreateLock_Syscall:Exceeding Max Lock Name length\n");
		delete[] lockName;
		return -1;
	}	
	
	if (!lockName) {
		printf("%s","CreateLock_Syscall:Can't allocate kernel buffer in CreateLock\n");
		delete[] lockName;
		return -1;
    }

    if( copyin(vaddr,len,lockName) == -1 ) {
		printf("%s", "CreateLock_Syscall:Could not copy from the virtual address\n");
		delete[] lockName;
		return -1;
    }
    lockName[len]='\0';
	
	if(kernelLockListLock == NULL){
		printf("%s","CreateLock_Syscall:kernelLockListLock is NULL\n");
		delete[] lockName;
		return -1;
    }
	
	if(nextLockIndex > MAX_LOCKS){
		printf("%s","CreateLock_Syscall:No more Locks can be created, count reached MAX\n");
		delete[] lockName;
		return -1;
    }	
	// @
//	printf("creating the Lock %s\n", lockName);
	// Acquire the Lock over the Lock list to access 
	if(kernelLockListLock == NULL){
		printf("%s","CreateLock_Syscall:Kernel Lock List Lock is NULL\n");
		return -1;
    }	
	kernelLockListLock->Acquire();
	
	// Create the Lock for the user prog
	lockList[nextLockIndex].lock = new Lock(lockName);
	
	// Make the address space of the Lock same as the currentThread
	lockList[nextLockIndex].lockAddrSpace = currentThread->space;
	
	// Make the usage counter to zero for the new lock created
	lockList[nextLockIndex].usageCounter = 0;
	
	lockList[nextLockIndex].toBeDestroyed = true;
	// @
//	printf("lock created : %d\n", nextLockIndex);
	rv = nextLockIndex;
	nextLockIndex++;
	kernelLockListLock->Release();
	delete[] lockName;
	// return the index of the Lock created to the user program
	return rv;
}	

void Acquire_Syscall(unsigned int lockIndex) {
	
	if(kernelLockListLock == NULL){
		printf("%s","Acquire_Syscall:Kernel Lock List Lock is NULL\n");
		return;
    }	
	kernelLockListLock->Acquire();
	
	if((lockIndex < 0) || lockIndex >= nextLockIndex){
		printf("%s","Acquire_Syscall:Invalid Lock index\n");
		kernelLockListLock->Release();
		return;
    }	
	
	if(lockList[lockIndex].lock == NULL){
		printf("%s","Acquire_Syscall:Lock does not exist\n");
		kernelLockListLock->Release();
		return;
    }

	if(lockList[lockIndex].lockAddrSpace != currentThread->space){
		printf("%s","Acquire_Syscall:Lock does not belong to the current user prog\n");
		kernelLockListLock->Release();
		return;
    }	
	
	lockList[lockIndex].usageCounter++;
	lockList[lockIndex].toBeDestroyed = false;
	
	kernelLockListLock->Release();
	lockList[lockIndex].lock->Acquire();
	return;
}

void Release_Syscall(unsigned int lockIndex) {
	
	if(kernelLockListLock == NULL){
		printf("%s","Release_Syscall:Kernel Lock List Lock is NULL\n");
		return;
    }	
	kernelLockListLock->Acquire();
	
	if((lockIndex < 0) || lockIndex >= nextLockIndex){
		printf("%s","Release_Syscall:Invalid Lock index\n");
		kernelLockListLock->Release();
		return;
    }	
	
	if(lockList[lockIndex].lock == NULL){
		printf("%s","Release_Syscall:Lock does not exist\n");
		kernelLockListLock->Release();
		return;
    }
	
	if(lockList[lockIndex].lockAddrSpace != currentThread->space){
		printf("%s","Release_Syscall:Lock does not belong to the current user prog\n");
		kernelLockListLock->Release();
		return;
    }	
	lockList[lockIndex].lock->Release();
	lockList[lockIndex].usageCounter--;	
	// Lock can be destroyed since its free
	if(lockList[lockIndex].usageCounter == 0)
		lockList[lockIndex].toBeDestroyed = true;
	
	kernelLockListLock->Release();
	
	return;
}

void DestroyLock_Syscall(unsigned int lockIndex) {
	
	if(kernelLockListLock == NULL){
		printf("%s","DestroyLock_Syscall:Kernel Lock List Lock is NULL\n");
		return;
    }	
	kernelLockListLock->Acquire();
	
	if((lockIndex < 0) || lockIndex >= nextLockIndex){
		printf("%s","DestroyLock_Syscall:Invalid Lock index\n");
		kernelLockListLock->Release();
		return;
    }	
	
	if(lockList[lockIndex].lock == NULL){
		printf("%s","DestroyLock_Syscall:Lock does not exist\n");
		kernelLockListLock->Release();
		return;
    }

	if(lockList[lockIndex].lockAddrSpace != currentThread->space){
		printf("%s","DestroyLock_Syscall:Lock does not belong to the current user prog\n");
		kernelLockListLock->Release();
		return;
    }	

	if(lockList[lockIndex].toBeDestroyed == false){
		printf("%s","DestroyLock_Syscall:Lock under use!\n");
		kernelLockListLock->Release();
		return;
	}	
	
	delete lockList[lockIndex].lock;
	lockList[lockIndex].lock = NULL;
	lockList[lockIndex].lockAddrSpace = NULL;
	lockList[lockIndex].usageCounter = 0;
	lockList[lockIndex].toBeDestroyed = FALSE;
	return;
}

int CreateCV_Syscall(unsigned int vaddr, int len) {
	// Allocates a CV from the kernel CV list to the user prog
	
	int rv = -1;	// index value of the CV created
	char *CVName = new char[len+1];
	
	if((vaddr+len) <= vaddr){
		printf("%s", "CreateCV_Syscall:Invalid Virtual address length\n");
		delete[] CVName;
		return -1;
	}	
	
	if((vaddr < 0) || (vaddr > currentThread->space->spaceSize)){
		printf("%s", "CreateCV_Syscall:Invalid virtual address\n");
		delete[] CVName;
		return -1;
	}	
	
	if((vaddr+len) > currentThread->space->spaceSize){
		printf("%s", "CreateCV_Syscall:Trying to access Invalid Virtual address\n");
		delete[] CVName;
		return -1;
	}	
	
	if(len > MAX_CHARLEN){
		printf("%s", "CreateCV_Syscall:Exceeding Max CV Name length\n");
		delete[] CVName;
		return -1;
	}	
	
	if (!CVName) {
		printf("%s","CreateCV_Syscall:Can't allocate kernel buffer in CreateCV\n");
		delete[] CVName;
		return -1;
    }

    if(copyin(vaddr,len,CVName) == -1 ) {
		printf("%s", "CreateCV_Syscall:Could not copy from the virtual address\n");
		delete[] CVName;
		return -1;
    }
    CVName[len]='\0';
	
	if(nextCVIndex > MAX_LOCKS){
		printf("%s","CreateCV_Syscall:No more CVs can be created, count reached MAX\n");
		delete[] CVName;
		return -1;
    }	
	
	// Acquire the Lock over the CV list to access 
	if(kernelCVListLock == NULL){
		printf("%s","CreateCV_Syscall:Kernel CV List Lock is NULL\n");
		return -1;
    }	
	kernelCVListLock->Acquire();
	
	// Create the CV for the user prog
	CVList[nextCVIndex].condition = new Condition(CVName);
	// @
//	printf("condition variable %d created\n", nextCVIndex);
	// Make the address space of the CV same as the currentThread
	CVList[nextCVIndex].conditionAddrSpace = currentThread->space;
	// Make the usage counter to zero for the new CV created
	CVList[nextCVIndex].usageCounter = 0;
	CVList[nextCVIndex].toBeDestroyed = true;
	
	rv = nextCVIndex;
	nextCVIndex++;
	kernelCVListLock->Release();
	delete[] CVName;
	
	// return the index of the CV created to the user program
	return rv;
}	

void Wait_Syscall(unsigned int CVIndex, unsigned int lockIndex) {

	if(kernelLockListLock == NULL){
		printf("%s","Wait_Syscall:Kernel Lock List Lock is NULL\n");
		return;
    }	
	kernelLockListLock->Acquire();
	
	if((lockIndex < 0) || lockIndex >= nextLockIndex){
		printf("%s","Wait_Syscall:Invalid Lock index\n");
		kernelLockListLock->Release();
		return;
    }	
	
	if(lockList[lockIndex].lock == NULL){
		printf("%s","Wait_Syscall:Lock does not exist\n");
		kernelLockListLock->Release();
		return;
    }

	if(lockList[lockIndex].lockAddrSpace != currentThread->space){
		printf("%s","Wait_Syscall:Lock does not belong to the current user prog\n");
		kernelLockListLock->Release();
		return;
    }	
	
	kernelLockListLock->Release();
	
	if(kernelCVListLock == NULL){
		printf("%s","Wait_Syscall:Kernel CV List Lock is NULL\n");
		return;
    }	
	kernelCVListLock->Acquire();
	
	if((CVIndex < 0) || CVIndex >= nextCVIndex){
		printf("%s","Wait_Syscall:Invalid CV index\n");
		kernelCVListLock->Release();
		return;
    }	
	
	if(CVList[CVIndex].condition == NULL){
		printf("%s","Wait_Syscall:CV does not exist\n");
		kernelCVListLock->Release();
		return;
    }

	if(CVList[CVIndex].conditionAddrSpace != currentThread->space){
		printf("%s","Wait_Syscall:CV does not belong to the current user prog\n");
		kernelCVListLock->Release();
		return;
    }	
	
	// Reacquire the Lock List Lock after validating CVIndex
	kernelLockListLock->Acquire();
	
	// increment the number of waiters on the CV
	CVList[CVIndex].usageCounter++;
	CVList[CVIndex].toBeDestroyed = false;
	
	kernelCVListLock->Release();
	kernelLockListLock->Release();
	
	CVList[CVIndex].condition->Wait(lockList[lockIndex].lock);
	
	return;
}

void Signal_Syscall(unsigned int CVIndex, unsigned int lockIndex) {

	if(kernelLockListLock == NULL){
		printf("%s","Signal_Syscall:Kernel Lock List Lock is NULL\n");
		return;
    }	
	kernelLockListLock->Acquire();
	
	if((lockIndex < 0) || lockIndex >= nextLockIndex){
		printf("%s","Signal_Syscall:Invalid Lock index\n");
		kernelLockListLock->Release();
		return;
    }	
	
	if(lockList[lockIndex].lock == NULL){
		printf("%s","Signal_Syscall:Lock does not exist\n");
		kernelLockListLock->Release();
		return;
    }

	if(lockList[lockIndex].lockAddrSpace != currentThread->space){
		printf("%s","Signal_Syscall:Lock does not belong to the current user prog\n");
		kernelLockListLock->Release();
		return;
    }	
	
	kernelLockListLock->Release();
	
	if(kernelCVListLock == NULL){
		printf("%s","Signal_Syscall:Kernel CV List Lock is NULL\n");
		return;
    }	
	kernelCVListLock->Acquire();
	
	if((CVIndex < 0) || CVIndex >= nextCVIndex){
		printf("%s","Signal_Syscall:Invalid CV index\n");
		kernelCVListLock->Release();
		return;
    }	
	
	if(CVList[CVIndex].condition == NULL){
		printf("%s","Signal_Syscall:CV does not exist\n");
		kernelCVListLock->Release();
		return;
    }

	if(CVList[CVIndex].conditionAddrSpace != currentThread->space){
		printf("%s","Signal_Syscall:CV does not belong to the current user prog\n");
		kernelCVListLock->Release();
		return;
    }	
	
	// Reacquire the Lock List Lock after validating CVIndex
	kernelLockListLock->Acquire();
	
	if(CVList[CVIndex].usageCounter == 0){
		printf("%s","Signal_Syscall:No waiters to signal\n");
		CVList[CVIndex].toBeDestroyed = true;
		kernelCVListLock->Release();
		kernelLockListLock->Release();
		return;
    }	
	
	CVList[CVIndex].usageCounter--;
	CVList[CVIndex].condition->Signal(lockList[lockIndex].lock);
	
	if(CVList[CVIndex].usageCounter == 0){
		CVList[CVIndex].toBeDestroyed = true;
		kernelCVListLock->Release();
		kernelLockListLock->Release();
		return;
    }
	
	kernelCVListLock->Release();
	kernelLockListLock->Release();
	
	return;
}

void Broadcast_Syscall(unsigned int CVIndex, unsigned int lockIndex) {

	if(kernelLockListLock == NULL){
		printf("%s","Broadcast_Syscall:Kernel Lock List Lock is NULL\n");
		return;
    }	
	kernelLockListLock->Acquire();
	
	if((lockIndex < 0) || lockIndex >= nextLockIndex){
		printf("%s","Broadcast_Syscall:Invalid Lock index\n");
		kernelLockListLock->Release();
		return;
    }	
	
	if(lockList[lockIndex].lock == NULL){
		printf("%s","Broadcast_Syscall:Lock does not exist\n");
		kernelLockListLock->Release();
		return;
    }

	if(lockList[lockIndex].lockAddrSpace != currentThread->space){
		printf("%s","Broadcast_Syscall:Lock does not belong to the current user prog\n");
		kernelLockListLock->Release();
		return;
    }	
	
	kernelLockListLock->Release();
	
	if(kernelCVListLock == NULL){
		printf("%s","Broadcast_Syscall:Kernel CV List Lock is NULL\n");
		return;
    }	
	kernelCVListLock->Acquire();
	
	if((CVIndex < 0) || CVIndex >= nextCVIndex){
		printf("%s","Broadcast_Syscall:Invalid CV index\n");
		kernelCVListLock->Release();
		return;
    }	
	
	if(CVList[CVIndex].condition == NULL){

	printf("%s","Broadcast_Syscall:CV does not exist\n");
		kernelCVListLock->Release();
		return;
    }

	if(CVList[CVIndex].conditionAddrSpace != currentThread->space){
		printf("%s","Broadcast_Syscall:CV does not belong to the current user prog\n");
		kernelCVListLock->Release();
		return;
    }	
	
	// Reacquire the Lock List Lock after validating CVIndex
	kernelLockListLock->Acquire();
	
	if(CVList[CVIndex].usageCounter == 0){
		printf("%s","Broadcast_Syscall:No waiters to Broadcast\n");
		CVList[CVIndex].toBeDestroyed = true;
		kernelCVListLock->Release();
		kernelLockListLock->Release();
		return;
    }	
	
	CVList[CVIndex].usageCounter = 0;
	CVList[CVIndex].condition->Broadcast(lockList[lockIndex].lock);
	
	kernelCVListLock->Release();
	kernelLockListLock->Release();
	
	return;
}

void DestroyCV_Syscall(unsigned int CVIndex) {
	
	if(kernelCVListLock == NULL){
		printf("%s","DestroyCV_Syscall:Kernel CV List Lock is NULL\n");
		return;
    }	
	kernelCVListLock->Acquire();
	
	if((CVIndex < 0) || CVIndex >= nextCVIndex){
		printf("%s","DestroyCV_Syscall:Invalid CV index\n");
		kernelCVListLock->Release();
		return;
    }	
	
	if(CVList[CVIndex].condition == NULL){
		printf("%s","DestroyCV_Syscall:CV does not exist\n");
		kernelCVListLock->Release();
		return;
    }

	if(CVList[CVIndex].conditionAddrSpace != currentThread->space){
		printf("%s","DestroyCV_Syscall:CV does not belong to the current user prog\n");
		kernelCVListLock->Release();
		return;
    }	

	if(CVList[CVIndex].toBeDestroyed == false){
		printf("%s","DestroyCV_Syscall:CV under use!\n");
		kernelCVListLock->Release();
		return;
	}	
	
	delete CVList[CVIndex].condition;
	CVList[CVIndex].condition = NULL;
	
	return;
}

void Yield_Syscall() {

	currentThread->Yield();
	return;
	
}

void KernelFunc(unsigned int vaddr) {
// Set up all registers and switch to user mode to run the userprog
	int myPid = 0;
	
	machine->WriteRegister(PCReg, vaddr);	
	machine->WriteRegister(NextPCReg, (vaddr + 4));
	
	processTableLock->Acquire();	
	// loop through the entire process table to find the Pid of the current process
	for(int i = 0; i < processCount; i++) {
		if(processTable[i].addrSpace == currentThread->space){
			myPid = processTable[i].Pid;
			break;
		}	
	}	
	processTableLock->Release();
	
	machine->WriteRegister(StackReg, processTable[currentThread->pid].stackTops[currentThread->threadId]);
	// @
//	printf("Kernel func : myPid = %d thread id is %d name is %s, stacktop is %d\n", myPid, currentThread->threadId, currentThread->getName(), processTable[currentThread->pid].stackTops[currentThread->threadId]);
	currentThread->space->RestoreState();
	
	machine->Run();	
}

//**************************************************************************************
//								FORK SYSCALL
//**************************************************************************************
void Fork_Syscall(unsigned int vaddr) {
	
	// Parent process process id of the Thread to be created
	int myPid = -1;
	int oldNumPages = 0, newNumPages = 0;
	int freePageIndex = 0;
	AddrSpace *newSpace = NULL;
	TranslationEntry *pageTable = NULL;
	
	if((vaddr < 0) || (vaddr > currentThread->space->spaceSize)){
		printf("%s", "Fork_Syscall:Invalid virtual address\n");
		return;
	}
	
	processTableLock->Acquire();	

	myPid = currentThread->pid;
//	@
//	printf("Forked Thread : My Process Id is %d\n", myPid);
	
	processTableLock->Release();
	
	if(pageTableLock[myPid] == NULL){
		printf("%s", "Fork_Syscall:PageTable Lock is NULL\n");
		return;
	}
	
	pageTableLock[myPid]->Acquire();
	
	// Fetch the current size of the address space
	oldNumPages = currentThread->space->GetNumPages();
	newNumPages = oldNumPages + 8;
// @
//	printf("oldnumpages = %d, newnumpages = %d\n", oldNumPages, newNumPages);
	pageTable=new TranslationEntry[newNumPages];
	// update the new number of pages into the address space of the current thread
	currentThread->space->SetNumPages(newNumPages);
	
	// create a new page table with 8 pages more than the current address space size
	TranslationEntry *newPageTable = new TranslationEntry[newNumPages];
	pageTable = currentThread->space->GetPageTableRef();
	for(int index = 0; index < oldNumPages; index++){
		newPageTable[index].virtualPage = pageTable[index].virtualPage;
		newPageTable[index].physicalPage = pageTable[index].physicalPage;
		newPageTable[index].valid = pageTable[index].valid;
		newPageTable[index].use = pageTable[index].use;
		newPageTable[index].dirty = pageTable[index].dirty;
		newPageTable[index].readOnly = pageTable[index].readOnly;
	}
	
	for(int index = oldNumPages; index < newNumPages; index++){
		// Acquire the bitmap lock to find the free page available in physical memory
		freePageIndex = 0;
		bitMapObjectLock->Acquire();	
		freePageIndex = bitMapObject->Find();
// @
//		printf("in Fork found phyPage is %d\n", freePageIndex);
		if(freePageIndex == -1){
			printf("No physical memory available\n");
			interrupt->Halt();
		}	
		bitMapObjectLock->Release();
		newPageTable[index].virtualPage = index;
		newPageTable[index].physicalPage = freePageIndex;
		
		newPageTable[index].valid = true;
		newPageTable[index].use = false;
		newPageTable[index].dirty = false;
		newPageTable[index].readOnly = false;
	}
	
	currentThread->space->DeletePageTable();
	
	// Make the new page table official 
	currentThread->space->SetPageTableRef(newPageTable);
	
	machine->pageTable = currentThread->space->GetPageTableRef();
	
	pageTableLock[myPid]->Release();
	
	Thread *t = new Thread("childThread");
	
	processTableLock->Acquire();
	
	processTable[myPid].threadCount++;
	processTable[myPid].activeThreadCount++;
	t->threadId = processTable[myPid].threadCount;
	t->pid = myPid;
//	@	
//	printf("threadCount in fork is %d\n", processTable[myPid].threadCount);
	t->space = currentThread->space;

// @
//	printf("Thread id assigned is %d , my process Id is %d\n", t->threadId, myPid);
	processTable[myPid].stackTops[t->threadId] = ((newNumPages * PageSize) - 16);
	
// @
//	printf("stack top is %d\n", processTable[myPid].stackTops[t->threadId]);
	processTableLock->Release();

// @
//	printf("Forking a thread %d in ecception.cc\n", processTable[myPid].threadCount);
	t->Fork((VoidFunctionPtr)KernelFunc, vaddr);
}

//**************************************************************************************
//								EXIT SYSCALL
//**************************************************************************************
void Exit_Syscall(int status) {
		
 	// Exit System call Local variables
		unsigned int index = 0;
		unsigned int StackPage = 0, numPages = 0;
		int stackTop = 0; 
		TranslationEntry* myPageTable;
	// End of Exit System call Local variables	 
		
	// Finish the UNIX process which invoked the testfiles from command prompt	
	if(currentThread->threadId == -1) 
	{ 
		currentThread->Finish();
		return;
	} 
	else
	{
		processTableLock->Acquire();
		if((noOfActiveProcesses == 1) && ((processTable[currentThread->pid].activeThreadCount) == 0))
		{
			//Halt Nachos
			processTableLock->Release();
		
			interrupt->Halt();
		}

		//Last thread in Process
		else if((processTable[currentThread->pid].activeThreadCount) == 0)
		{
			myPageTable = currentThread->space->GetPageTableRef();
			//printf("THREAD IN PROCESS %d EXITING \n",currentThread->threadId);
			numPages = currentThread->space->GetNumPages();
			//Clearing the bitmap array for the entire address space length
			for (index = 0;index < numPages;index++) 
			{	
				// Acquire the bitmap lock to mutullly exclusively access the bitmap array
				bitMapObjectLock->Acquire();
		
				bitMapObject->Clear(myPageTable[index].physicalPage); 
				myPageTable[index].physicalPage = NULL;
		
				bitMapObjectLock->Release();
			}
			// Delete the address space memory allocated to this process
			delete currentThread->space;

			// keep track of the current number of active processes
			noOfActiveProcesses--;
			
			processTableLock->Release();
						
			// Process finish - I our case, finish the last thread in carls jr simulation
			currentThread->Finish();
		}

		// If the thread is not the last thread in the process "Pid", then the thread should clear the stack
		// which was allocated to it
		else 
		{
			// Clear the entire 8 pages of stack memory allocated to this thread in the address space
			int StackTop = processTable[currentThread->pid].stackTops[currentThread->threadId];
			myPageTable = currentThread->space->GetPageTableRef();
			
			StackPage = divRoundUp(StackTop,PageSize) - 8;
			
			// Clearing the bitmap array for the entire stack space length
			for (index = 0; index < 8;index++) 
			{
				bitMapObjectLock->Acquire();
				
				bitMapObject->Clear(myPageTable[StackPage + index].physicalPage); 
				myPageTable[StackPage + index].physicalPage = NULL;
		
				bitMapObjectLock->Release();
			}
			
		
			//Clear the stack entry in the Process Table
			processTable[currentThread->pid].stackTops[currentThread->threadId] = NULL;

			//	keep track of the current number of active processes
			if(processTable[currentThread->pid].activeThreadCount > 0)
			{
			   processTable[currentThread->pid].activeThreadCount--;
			}

			processTableLock->Release();

			currentThread->Finish();
		}
	}
  
}

	
void ExecFunc()
{
	int stackStart = 0;
	
    currentThread->space->InitRegisters();		// set the initial register values
	
	stackStart = ((currentThread->space->GetNumPages()) * PageSize) - 16;
    machine->WriteRegister(StackReg, stackStart);
	
	currentThread->space->RestoreState();		// load page table register

    machine->Run();			// jump to the user progam
	
    ASSERT(FALSE);			// machine->Run never returns;
							// the address space exits
							// by doing the syscall "exit"
}					

//**************************************************************************************
//								EXEC SYSCALL
//**************************************************************************************
SpaceId	Exec_Syscall(unsigned int vaddr, unsigned int len) {

	Thread *t;
	AddrSpace *space;
	int stackStart = 0;
	char *buf = new char[len+1];	// Kernel buffer to put the name in
	OpenFile *fd = NULL;					// The new open file
   	int id;							// The openfile id
	
	
	if((vaddr+len) <= vaddr){
		printf("%s", "Exec_Syscall : Invalid Virtual address length\n");
		delete[] buf;
		return -1;
	}	
	
	if((vaddr < 0) || (vaddr > currentThread->space->spaceSize)){
		printf("%s", "Exec_Syscall : Invalid virtual address\n");
		delete[] buf;
		return -1;
	}	
	
	if((vaddr+len) > currentThread->space->spaceSize){
		printf("%s", "Exec_Syscall : Trying to access Invalid Virtual address\n");
		delete[] buf;
		return -1;
	}	
	
    if (!buf) {
		printf("%s","Exec_Syscall : Can't allocate kernel buffer in Open\n");
		return -1;
    }
	
	if(copyin(vaddr,len,buf) == -1 ) {
		printf("%s","Exec_Syscall : Bad pointer passed to Exec\n");
		delete[] buf;
		return -1;
    }
	
	buf[len]='\0';
	fd = fileSystem->Open(buf);	
	
	if (fd == NULL)
	{
		printf("Exec_Syscall : Unable to open file %s\n", buf);
		return -1;
   	}
  
	// Create a new Address space for this new Process that will be created
  	space = new AddrSpace(fd);
	
	// Create a new thread (represents a process)
	t = new Thread(buf);
	
	// Give the created address space to the new thread which is created
	t->space = space;
	
	// Update the process table with the new process details
	processTableLock->Acquire();
	
	// This value is never decremented
	processCount++;
// @
//	printf("process count is incremented to %d\n", processCount);
	processTable[processCount].Pid = processCount;
	t->pid = processTable[processCount].Pid;
	
// @ 
//	printf("in Exec: Process Id set to %d\n", processTable[processCount].Pid);
	processTable[processCount].threadCount = 0;
	processTable[processCount].addrSpace = space;
	stackStart = ((t->space->GetNumPages()) * PageSize) - 16;

	// Every Process will have its own stack start location at the 
	// beginning of the stack top array in processTable	
	processTable[processCount].stackTops[0] = stackStart;
	processTable[processCount].activeThreadCount = 0;
	processTableLock->Release();
	
	activeProcessesLock->Acquire();
	// This value tells the current number of active processes in the system
	noOfActiveProcesses++;
	activeProcessesLock->Release();
	t->threadId = 0;	
	t->Fork((VoidFunctionPtr)ExecFunc, NULL);
	
	// Return the spaceId 
	return processTable[processCount].Pid;
}	


void Print_Stmt_Syscall(unsigned int vaddr)
{
   int len = 150, i;
   char *buffer = new char[len];  // Kernel buffer to put the name in

   if (!buffer) return;
       //buffer exists
	
	for(i = 0; i < len; i++){	
		if( copyin(vaddr,1,&buffer[i]) == -1 ) {
			printf("%s","Print_Stmt_Syscall : Bad pointer passed to Print\n");
				return;
		}else{		
			if(buffer[i] == '$')
				break;
			vaddr++;	
		}	
	}
		
    //statement with no access specifiers stored into buffer
   buffer[i]='\0';
   
printf(buffer);
}


void Print_1Arg_Syscall(unsigned int vaddr, int val)
{
   int len = 90, i = 0;
   char *buffer = new char[len]; 
   
   if (!buffer) return;
       //buffer exists
	   
   for(i = 0; i < len; i++){	
		if( copyin(vaddr,1, &buffer[i]) == -1 ) {
			printf("%s","Print_Stmt_Syscall : Bad pointer passed to Print\n");
				return;
		}else{		
			if(buffer[i] == '$')
				break;
			vaddr++;	
		}	
	}
   // statement with 1 access specifiers stored into buffer
   buffer[i]='\0';
	
printf(buffer, val);
}


void Print_Str_Syscall(unsigned int vaddr1, unsigned int vaddr2) {

	int len = 90, i = 0;
	char *buffer1 = new char[len];
	char *buffer2 = new char[len];
	
	if (!buffer1) return;
	if (!buffer2) return;
      
	for(i = 0; i < len; i++){	
		if( copyin(vaddr1,1, &buffer1[i]) == -1 ) {
			printf("%s","Print_Stmt_Syscall : Bad pointer passed to Print\n");
				return;
		}else{		
			if(buffer1[i] == '$')
				break;
			vaddr1++;	
		}	
	}
    buffer1[i]='\0';
	for(i = 0; i < len; i++){	
		if( copyin(vaddr2,1, &buffer2[i]) == -1 ) {
			printf("%s","Print_Stmt_Syscall : Bad pointer passed to Print\n");
				return;
		}else{		
			if(buffer2[i] == '$')
				break;
			vaddr2++;	
		}	
	}
	buffer2[len]='\0';
	
printf(buffer1, buffer2);
}


void Print_2Arg_Syscall(unsigned int vaddr, int val1, int val2)
{
   int len=90, i = 0;
   char *buffer = new char[len];

   if (!buffer) return;
       //buffer exists
	
	for(i = 0; i < len; i++){	
		if( copyin(vaddr,1, &buffer[i]) == -1 ) {
			printf("%s","Print_Stmt_Syscall : Bad pointer passed to Print\n");
				return;
		}else{		
			if(buffer[i] == '$')
				break;
			vaddr++;	
		}	
	}
   // statement with 2 access specifiers stored into buffer
   buffer[i]='\0';

printf(buffer,val1,val2);
}


void Print_3Arg_Syscall(unsigned int vaddr, int val1, int val2, int val3)
{
   int len=200, i = 0;
   char *buffer = new char[len];

   if (!buffer) return;
   for(i = 0; i < len; i++){	
		if( copyin(vaddr,1, &buffer[i]) == -1 ) {
			printf("%s","Print_Stmt_Syscall : Bad pointer passed to Print\n");
				return;
		}else{		
			if(buffer[i] == '$')
				break;
			vaddr++;	
		}	
	}
   // statement with 2 access specifiers stored into buffer
   buffer[i]='\0';

printf(buffer,val1,val2,val3);
}


// @ Not used - can be removed
void Sprint_Syscall(unsigned int vaddr1, unsigned int vaddr2, unsigned int vaddr3, int id)
{
	int len=50;
	char *buffer1 = new char[len];
	char *buffer2 = new char[len];
	char *buffer3 = new char[len];
	
	if (!buffer1) return;
	if (!buffer2) return;
	if (!buffer3) return;
	
	if( copyin(vaddr1,len,buffer1) == -1 ) {
       printf("%s","Sprint_Str+Int_Syscall : Bad pointer passed to Print\n");
       delete buffer1;
       return;
	}
	buffer1[len]='\0';
   
	if( copyin(vaddr2,len,buffer2) == -1 ) {
		printf("%s","Sprint_Str+Int_Syscall : Bad pointer passed to Print\n");
		delete buffer2;
		return;
	}
	buffer2[len]='\0';
   
	if( copyin(vaddr3,len,buffer3) == -1 ) {
		printf("%s","Sprint_Str+Int_Syscall : Bad pointer passed to Print\n");
		delete buffer3;
		return;
	}
	buffer3[len]='\0';
   
	sprintf(buffer1, buffer2, buffer3, id);

	printf("concatenated string is %s\n", buffer1);	
	return;      
}
	
int Scan_Syscall(unsigned int vaddr)
{
   int len=2;
   int p;
   char *buffer = new char[len+1];        // Kernel buffer to put the name in
 
   if (!buffer) return -1;
       //buffer exists
   if( copyin(vaddr,len,buffer) == -1 ) {
       printf("%s","Bad pointer passed to Scan\n");
       delete buffer;
       return -1;
   }
   
   //access spacifier read into buffer  
   buffer[len]='\0';
   scanf(buffer,&p);

   return p;
} 

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2); // Which syscall?
    int rv=0; 	// the return value from a syscall

    if ( which == SyscallException ) {
	switch (type) {
	    default:
		DEBUG('a', "Unknown syscall - shutting down.\n");
		
	    case SC_Halt:
		DEBUG('a', "Shutdown, initiated by user program.\n");
		interrupt->Halt();
		break;
		
	    case SC_Create:
		DEBUG('a', "Create syscall.\n");
		Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
		
	    case SC_Open:
		DEBUG('a', "Open syscall.\n");
		rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
		
	    case SC_Write:
		DEBUG('a', "Write syscall.\n");
		Write_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6));
		break;
		
	    case SC_Read:
		DEBUG('a', "Read syscall.\n");
		rv = Read_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6));
		break;
		
	    case SC_Close:
		DEBUG('a', "Close syscall.\n");
		Close_Syscall(machine->ReadRegister(4));
		break;
		
		case SC_CreateLock:
		DEBUG('a', "CreateLock syscall.\n");
		rv = CreateLock_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5));
		break;	
		
		case SC_Acquire:
		DEBUG('a', "Acquire syscall.\n");
		Acquire_Syscall(machine->ReadRegister(4));
		break;
		
		case SC_Release:
		DEBUG('a', "Release syscall.\n");
		Release_Syscall(machine->ReadRegister(4));
		break;
		
		case SC_DestroyLock:
		DEBUG('a', "DestroyLock syscall.\n");
		DestroyLock_Syscall(machine->ReadRegister(4));
		break;	
		
		case SC_CreateCV:
		DEBUG('a', "CreateCV syscall.\n");
		rv = CreateCV_Syscall(machine->ReadRegister(4),
				  machine->ReadRegister(5));
		break;
		
		case SC_Wait:
		DEBUG('a', "Wait syscall.\n");
		Wait_Syscall(machine->ReadRegister(4),
				  machine->ReadRegister(5));
		break;
		
		case SC_Signal:
		DEBUG('a', "Signal syscall.\n");
		Signal_Syscall(machine->ReadRegister(4),
				  machine->ReadRegister(5));
		break;
		
		case SC_Broadcast:
		DEBUG('a', "Broadcast syscall.\n");
		Broadcast_Syscall(machine->ReadRegister(4),
				  machine->ReadRegister(5));
		break;
		
		case SC_DestroyCV:
		DEBUG('a', "DestroyCV syscall.\n");
		DestroyCV_Syscall(machine->ReadRegister(4));
		break;	
		
		case SC_Yield:
		DEBUG('a', "Yield syscall.\n");
		Yield_Syscall();
		break;		
		
		case SC_Fork:
		DEBUG('a', "Fork syscall.\n");
		Fork_Syscall(machine->ReadRegister(4));
		break;		
		
		case SC_Exit:
		DEBUG('a', "Exit syscall.\n");
		Exit_Syscall(machine->ReadRegister(4));
		break;		
		
		case SC_Scan:
		DEBUG('a', "Scan syscall.\n");
		rv = Scan_Syscall(machine->ReadRegister(4));
		break;
		
	 	case SC_Exec:
		DEBUG('a', "Exec syscall.\n");
		rv = Exec_Syscall(machine->ReadRegister(4), 
			machine->ReadRegister(5));
		break; 
		
		case SC_Print_Stmt:
		DEBUG('a', "Print_Stmt syscall.\n");
		Print_Stmt_Syscall(machine->ReadRegister(4));	
		break; 
		
		case SC_Print_1Arg:
		DEBUG('a', "Print_1Arg syscall.\n");
		Print_1Arg_Syscall(machine->ReadRegister(4),
			machine->ReadRegister(5));	
		break;
		
		case SC_Print_2Arg:
		DEBUG('a', "Print_2Arg syscall.\n");
		Print_2Arg_Syscall(machine->ReadRegister(4),
			machine->ReadRegister(5),
			machine->ReadRegister(6));	
		break;
		
		case SC_Print_3Arg:
		DEBUG('a', "Print_3Arg syscall.\n");
		Print_3Arg_Syscall(machine->ReadRegister(4),
			machine->ReadRegister(5),
			machine->ReadRegister(6),
			machine->ReadRegister(7));	
		break;
		
		case SC_Sprint:
		DEBUG('a', "Sprint syscall.\n");
		Sprint_Syscall(machine->ReadRegister(4),
			machine->ReadRegister(5),
			machine->ReadRegister(6),
			machine->ReadRegister(7));	
		break;
		
		case SC_Print_Str:
		DEBUG('a', "Print_Str syscall.\n");
		Print_Str_Syscall(machine->ReadRegister(4),
			machine->ReadRegister(5));	
		break;
	}

	// Put in the return value and increment the PC
	machine->WriteRegister(2,rv);
	machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
	machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
	machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
	return;
    } else {
      cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
      interrupt->Halt(); 
    }
}
