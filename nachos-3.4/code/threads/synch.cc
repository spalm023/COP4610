// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!

//----------------------------------------------------------------------
//Lock::Lock
//	Initialize a lock, so that it can be used for synchronization.
//	
//	"debugName" is an arbitrary name, useful for debugging.
//	Intializes a Semaphore for use as part of the lock, with an 
//	itial value of one.
//----------------------------------------------------------------------
Lock::Lock(char* debugName) 
{
    name = debugName ;
    sem = new Semaphore( debugName, 1 ) ;
    holder = NULL ;
}

//----------------------------------------------------------------------
//Lock::Lock
//	De-allocate the lock.
//----------------------------------------------------------------------
Lock::~Lock() 
{
    delete sem ;
    holder = NULL ;
}

//----------------------------------------------------------------------
//Lock::Acquire
//	Wait until the current lock is free, and then mark it as busy.
//	This behaves like Sem->P(), with 0 meaning that it is busy 
//	and 1 meaning that it is free.
//	It also sets the "holder" to the current running thread.
//----------------------------------------------------------------------
void 
Lock::Acquire() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    sem->P();
    holder = currentThread ;

    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
//Lock::Release
//	Verify that the Lock is held by the current thread.  (Only the 
//	holder of the lock can release it.)
//	If it is, set "holder" to null and change the semaphore state.
//	Equivalent to running sem->V().
//----------------------------------------------------------------------
void 
Lock::Release() 
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    ASSERT( isHeldByCurrentThread() ) ;
    sem->V() ;	
    holder = NULL ; 

    (void) interrupt->SetLevel(oldLevel);
}

bool
Lock::isHeldByCurrentThread()
{
    return holder == currentThread ;
}

//----------------------------------------------------------------------
//Condition::Condition
//----------------------------------------------------------------------
Condition::Condition(char* debugName) 
{ 
    name = debugName ;
    waitList = new List ;
}

//----------------------------------------------------------------------
//Condition::~Condition
//----------------------------------------------------------------------

Condition::~Condition() 
{ 
    delete waitList ;
}

//----------------------------------------------------------------------
//Condition::Wait
//----------------------------------------------------------------------

void Condition::Wait(Lock* conditionLock) 
{ 
    //Disable interupts so that actions are atomic
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    //printf( "Wait Lock, %d\n", currentThread ) ;

    //Verify the caller is the owner of lock is the current thread
    ASSERT( conditionLock->isHeldByCurrentThread() ) ; 
    
    //Add the thread to a queue waiting for the variable
    waitList->Append( (void *)currentThread ) ; 
    
    //Release the lock
    conditionLock->Release() ; 

    //"Relinquish" the CPU by putting the thread to sleep
    currentThread->Sleep() ;

    //When the thread is woken up, reaquire the lock
    conditionLock->Acquire() ;

    //Return interupts to their original state
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
//Condition::Signal
//Code is based off of Semaphore->V()
//----------------------------------------------------------------------

void Condition::Signal(Lock* conditionLock) 
{ 
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    //If there are items waiting for the condition variable
    //IsEmpty() is in provided by list.h && list.cc
    if( !waitList->IsEmpty() ) {
	
	thread = (Thread *)waitList->Remove() ;
	scheduler->ReadyToRun(thread) ;
    }

    (void) interrupt->SetLevel(oldLevel) ;
}

//----------------------------------------------------------------------
//Condition::Signal
//Code is similar to Semaphore->V() and Condition->Signal() but
//is a continous while loop until the queue is empty.
//----------------------------------------------------------------------
void Condition::Broadcast(Lock* conditionLock) 
{ 

   Thread *thread;
   IntStatus oldLevel = interrupt->SetLevel(IntOff);
   
   while ( !waitList->IsEmpty() )
   {
      thread = (Thread *)waitList->Remove();
      scheduler->ReadyToRun(thread);
   }
   (void) interrupt->SetLevel(oldLevel);
}
