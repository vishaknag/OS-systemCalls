/* testcases.c
 *	Simple program to test the system calls
 */

#include "syscall.h"

/* Global Variables */
unsigned int wait_cv_rv = 0, wait_lk_rv = 0;
unsigned int bcast_lk_rv = 0, bcast_cv_rv = 0;



/* Done */
void CreateLock_test() {
	
	unsigned int rv = 0;
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING CREATE LOCK SYSCALL <<<\n", 40, ConsoleOutput);	
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	/* Tests for CreateLock syscall */
	/* valid Lock create syscall */
	Write("\nCreating a Lock successfully\n", 32, ConsoleOutput);	
	rv = CreateLock("firstLock", 9);

	/* Destroy the created Lock */
	DestroyLock(rv);
	
	/* create lock syscall with an invalid length */
	Write("\nTrying to create a Lock with invalid Lock name length\n", 56, ConsoleOutput);	
	rv = CreateLock("secondLock", -1);
	
	/* create lock syscall with lock name length greater than max lock name length */
	Write("\nTrying to create a Lock with length of name greater than the MAX_CHARLEN\n", 76, ConsoleOutput);	
	rv = CreateLock("Lockname_which_is_longer_than_the_maximum_lock_name", 52);
	
	Write("\n-------------------------------------------------------------\n", 63, ConsoleOutput);
	
	Exit(0);
}	

void AcquireLock_test() {
	
	unsigned int rv = 0;
	
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING ACQUIRE LOCK SYSCALL <<<\n", 40, ConsoleOutput);
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	/* Tests for Acquire syscall */
	rv = CreateLock("AcquireLock", 11);
	
	/* Calling acquire with a valid Lock Index */
	Write("\nAcquiring a Lock successfully\n", 33, ConsoleOutput);	
	Acquire(rv);
	Release(rv);
	/* Acquire syscall with a negative invalid lock index */
	Write("\nTrying to acquire a lock with an invalid lock index\n", 55, ConsoleOutput);	
	Acquire(-3);
	
	/* Destroy the created Lock */
	DestroyLock(rv);
	Write("\n-------------------------------------------------------------\n", 63, ConsoleOutput);
	
	Exit(0);
}

void ReleaseLock_test() {

	unsigned int rv = 0;
	
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING RELEASE LOCK SYSCALL <<<\n", 39, ConsoleOutput);
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	/* Tests for Release syscall */
	rv = CreateLock("ReleaseLock", 9);
	
	/* Release syscall with a valid Lock index */
	Acquire(rv);
	Write("\nReleasing a Lock successfully\n", 33, ConsoleOutput);
	Release(rv);
	
	/* Release syscall with a negative lock index */
	Acquire(rv);
	Write("\nTrying to Release a Lock with an invalid Lock index\n", 55, ConsoleOutput);
	Release(-3);
	Write("\n-------------------------------------------------------------\n", 63, ConsoleOutput);
	
	Exit(0);
}

void DestroyLock_test() {
	
	unsigned int rv = 0;
	
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING DESTROY LOCK SYSCALL <<<\n", 39, ConsoleOutput);
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	/* Tests for Destroy syscall */
	rv = CreateLock("DestroyLock", 11);
	/* Acquire the created Lock */
	Acquire(rv);
	
	/* Destroy syscall to delete a lock under use */
	Write("\nTrying to destroy a Lock which is not released yet\n", 52, ConsoleOutput);	
	DestroyLock(rv);
	Release(rv);
	
	/*Destroy syscall with valid parameters*/
	Write("\nDestroying a Lock successfully\n", 33, ConsoleOutput);	
	DestroyLock(rv);
	
	/* Cannot acquire after deleting the lock */
	Acquire(rv);
	Write("\n-------------------------------------------------------------\n", 63, ConsoleOutput);
	
	Exit(0);
}

void CreateCV_test() {
	
	unsigned int rv = 0;
	
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING CREATE CV SYSCALL <<<\n", 38, ConsoleOutput);
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	/* Tests for Create CV syscall */
	rv = CreateCV("createCV", 8);
	Write("\nCreating a CV successfully\n", 30, ConsoleOutput);	
	
	/* create CV syscall with an invalid length */
	rv = CreateCV("CVname", -1);
	
	rv = CreateCV("CVname", 10000);
	
	/* create CV syscall with CV name length greater than max CV name length */
	rv = CreateCV("Lockname_which_is_longer_than_the_maximum_CV_name", 52);
	Write("\n-------------------------------------------------------------\n", 63, ConsoleOutput);
	
	Exit(0);	
}


void WaitThreadFunc() {

	Write("\nWait Thread : I am the child thread going on wait\n", 51, ConsoleOutput);
	
	Write("Wait Thread : Acquire the Lock to become eligible to be a waiter\n", 65, ConsoleOutput);
	Acquire(wait_lk_rv);
	
	/* Thread Going on wait */
	Write("Wait Thread : Going on wait!!\n", 30, ConsoleOutput);
	Wait(wait_cv_rv, wait_lk_rv);
	Write("\n\nWait Thread : I am back from wait\n", 36, ConsoleOutput);
	
	Release(wait_lk_rv);
	
	Exit(0);
}

void SignalTheWaitThreadFunc() {

	Write("\nSignal Thread : I am the child thread signalling the waiting thread\n", 69, ConsoleOutput);
	
	Write("Signal Thread : Acquire the Lock to wake up the waiter\n", 55, ConsoleOutput);
	Acquire(wait_lk_rv);
	
	/* Thread Going to signal the waiting thread */
	Write("Signal Thread : Signalling the waiter\n", 38, ConsoleOutput);
	Signal(wait_cv_rv, wait_lk_rv);
	
	Release(wait_lk_rv);

	Exit(0);
}

void Wait_Signal_test() {

	/* Tests for valid Wait syscall */
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING WAIT SIGNAL SYSCALLS <<<\n", 40, ConsoleOutput);	
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	wait_lk_rv = CreateLock("WaitLock", 8);
	wait_cv_rv = CreateCV("WaitCV", 6);
	
	/* Fork a thread to make it go on wait */
	Fork(WaitThreadFunc);
	
	/* Fork a thread to make it signal the waiting thread */
	Fork(SignalTheWaitThreadFunc);
	
	Exit(0);
} 

void BroadcastThreadFunc() {

	Write("\nBroadcast Thread : I am the child thread broadcasting the waiting threads\n", 75, ConsoleOutput);
	
	Write("\nBroadcast Thread : Acquire the Lock to wake up the waiters\n", 60, ConsoleOutput);
	Acquire(wait_lk_rv);
	
	/* Thread Going to broadcast the waiting thread */
	Write("\nBroadcast Thread : Broadcast the Signal\n", 41, ConsoleOutput);
	Broadcast(wait_cv_rv, wait_lk_rv);
	Release(wait_lk_rv);
	
	Exit(0);

}

void Broadcast_test() {

	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING BROADCAST SYSCALL <<<\n", 38, ConsoleOutput);
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	bcast_lk_rv = CreateLock("BroadcastLock", 13);
	bcast_cv_rv = CreateCV("BroadcastCV", 11);
	
	/* Fork 2 threads to make it go on wait */
	Fork(WaitThreadFunc);
	Fork(WaitThreadFunc);
	
	/* Fork a thread to make it Broadcast the signal to the waiting thread */
	Fork(BroadcastThreadFunc);
	
	Exit(0);
}

void DestroyCV_test() {
	
	unsigned int rv = 0;
	
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING DESTROY CV SYSCALL <<<\n", 37, ConsoleOutput);
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	/* Tests for Destroy syscall */
	rv = CreateCV("DestroyCV", 9);
	
	/* Destroy syscall to delete a CV which does not exist */
	Write("\nTrying to destroy a CV which is not created yet\n", 49, ConsoleOutput);	
	DestroyCV(10);
	
	/*Destroy syscall with valid parameters*/
	Write("\nDestroying a CV successfully\n", 30, ConsoleOutput);	
	DestroyCV(rv);
	
	Exit(0);
}

void YieldThreadFunc() {
	
	Write("\nYield Thread : I am going to Yield now\n", 40, ConsoleOutput);
	Yield();
	
	Exit(0);
}

void Yield_test() {
	
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING YIELD SYSCALL <<<\n", 32, ConsoleOutput);
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	Fork(YieldThreadFunc);

	Exit(0);
}

void ThreadFunc() {

	Write("\nForked Thread : I am the child thread Forked\n", 51, ConsoleOutput); 
	
	Exit(0);
}


void Fork_test() {
	
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
    Write("\t\n>>> TESTING FORK SYSCALL <<<\n", 32, ConsoleOutput);
	Write("-----------------------------------\n", 37, ConsoleOutput);
	
	Write("\nParent : I am the parent, Forking child thread 1\n", 50, ConsoleOutput);
	Fork(ThreadFunc);
	
	Write("\nParent : I am the parent, Forking child thread 2\n", 50, ConsoleOutput);
	Fork(ThreadFunc);
	
	Exit(0);
}


void ExitThread() {

	Write("\nExit Thread : I am going to Exit now!!\n", 40, ConsoleOutput); 
	Write("If I am the Last Thread in NACHOS then you will see 'Machine halting!' in the next line\n", 88, ConsoleOutput);
	
	Exit(0);
}

void Exit_test() {

	int threadCount = 0;
	
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING EXIT SYSCALL <<<\n", 33, ConsoleOutput);
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	Write("\nCreating thread1 >>>\n", 22, ConsoleOutput);
	Fork(ExitThread);
	
	Write("\nCreating thread2 >>>\n", 22, ConsoleOutput);
	Fork(ExitThread); 
	
	Exit(0);
}


void Exec_test() {
	
	Write("\t\n-----------------------------------", 38, ConsoleOutput);
	Write("\t\n>>> TESTING EXEC SYSCALL <<<\n", 33, ConsoleOutput);
	Write("-----------------------------------\n", 38, ConsoleOutput);
	
	Write("\nThe file 'test/testfiles' is EXECed to create a new process\n so you can see the same menu replicated\n", 102, ConsoleOutput);
	Write("\nThis is the output of the EXEC call\n", 37, ConsoleOutput);
	
	Exec("../test/testfiles", 17);
	
	Exit(0);
}

void Print_test() {

	int a = 1, b = 2, c = 3;
	
	Print_Stmt("I am printing only this statement\n");
	Print_1Arg("I am printing the value of a = %d\n", a);
	Print_2Arg("I am printing the value of a = %d and b = %d\n", a, b);
	Print_3Arg("I am printing the value of a = %d and b = %d and c = %d\n", a, b, c);
	
	Exit(0);
}

void Sprint_test() {
	
	char buffer[50];
	Print_Stmt("I am calling Sprint syscall\n");
	*buffer = Sprint(buffer, "%s%d", "ordertaker", 2);
	Print_Stmt("I am after Sprint syscall\n");
	Print_Str("The output of Sprint syscall is %s\n", buffer);
	
	Exit(0);
}

void CarlsJr_test(int instance) {
	
	int index = 0;
	
	for(index = 0; index < instance; index++) {
		Exec("../test/CarlsJr", 15);
	}
	
}


void main() {

	int userInput = 0;
	char buf[3];
	
    /* Output statements for test case menu */
	Print_Stmt("\n\t\t\t\t--------------------------------------------------------------\n");
	Print_Stmt("\n\t\t			     PROJECT 2		  GROUP - 58\n\n");
	Print_Stmt("\t\t\t		     Multiprogramming and System Calls\n");
	Print_Stmt("\t\t\t		      >>>>>>>> TEST CASES <<<<<<<<\n");
	Print_Stmt("\n\t\t\t\t--------------------------------------------------------------\n\n");
	
	Write(" 1 > Create Lock Syscall Test\n", 31, ConsoleOutput);
	Write(" 2 > Acquire Lock Syscall Test\n", 32, ConsoleOutput);
	Write(" 3 > Release Lock Syscall Test\n", 32, ConsoleOutput);
	Write(" 4 > Destroy Lock Syscall Test\n", 32, ConsoleOutput);
	Write(" 5 > Create CV Syscall Test\n", 29, ConsoleOutput);
	Write(" 6 > Wait-Signal Syscall Test\n", 31, ConsoleOutput);
	Write(" 7 > Broadcast Syscall Test\n", 29, ConsoleOutput);
	Write(" 8 > Destroy CV Syscall Test\n", 30, ConsoleOutput);
	Write(" 9 > Yield Syscall Test\n", 25, ConsoleOutput);
	Write("10 > Fork Syscall Test\n", 23, ConsoleOutput);
	Write("11 > Exec Syscall Test\n", 23, ConsoleOutput);
	Write("12 > Exit Syscall Test\n", 23, ConsoleOutput); 
	Write("13 > Exec 1 Instance of Carls Jr Simulation\n", 44, ConsoleOutput);
	Write("14 > Exec 2 Instances of Carls Jr Simulation\n\n", 46, ConsoleOutput);
	Write("\n\nEnter the index of the testcase you want to run here ->", 57, ConsoleOutput);
    
/* 	userInput = Scan("%d");  */
	userInput = 14;
 	switch(userInput){
		
		case 1:		CreateLock_test();
				break;
		case 2:		AcquireLock_test();
				break;
		case 3:		ReleaseLock_test();
				break;
		case 4:		DestroyLock_test();
				break;
		case 5:		CreateCV_test();
				break;
		case 6:		Wait_Signal_test();
				break;
		case 7:		Broadcast_test();
				break;
		case 8:		DestroyCV_test();
				break;
		case 9:		Yield_test();
				break;
		case 10:	Fork_test();
				break; 
	    case 11:	Exec_test();
				break;   
		case 12:	Exit_test();
				break; 		
		case 13:	CarlsJr_test(1);
				break; 
		case 14:	CarlsJr_test(2);
				break; 		
	    default: Write("\n Invalid option entered by the user !!\n\n", 41, ConsoleOutput);	
				break; 
	}  
 
}