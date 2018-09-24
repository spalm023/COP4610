// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------
 
int SharedVariable ; 
int threadsNum ;
Semaphore * sem = new Semaphore( "loop", 1 ) ;
Semaphore * bar = new Semaphore( "barrier", 0 ) ;
Lock * lock = new Lock( "loop" ) ;
void
SimpleThread( int which )
{
    int num, val ; 

#ifdef HW1_SEMAPHORES   
//printf( "HW1_SEMAPHORES LOOP\n" ) ;
 
    for ( num = 0 ; num < 5 ; num++ ) {

	sem->P();

	val = SharedVariable ;
	printf( "*** thread %d sees value %d\n", which, val ) ;
	currentThread->Yield() ;
	SharedVariable = val + 1 ;

	sem->V();
	
	currentThread->Yield() ;
    }	
	
    if( SharedVariable == ( threadsNum * 5 ) ) {
	bar->V();
    } 

    bar->P();	
    bar->V();

    val = SharedVariable ;
    printf( "Thread %d sees final value %d\n", which, val ) ;

#elif HW1_LOCKS
//printf( "HW1_LOCKS LOOP\n" ) ;   

    

    for ( num = 0 ; num < 5 ; num++ ) {

	lock->Acquire();

	val = SharedVariable ;
	printf( "*** thread %d sees value %d\n", which, val ) ;
	currentThread->Yield() ;
	SharedVariable = val + 1 ;

	lock->Release();
	
	currentThread->Yield() ;
	
    }

    if( SharedVariable == ( threadsNum * 5 ) ) {
	bar->V();
    } 

    bar->P();	
    bar->V();

    val = SharedVariable ;
    printf( "Thread %d sees final value %d\n", which, val ) ;

#else
//printf( "ELSE LOOP\n" ) ;
    for( num = 0 ; num < 5 ; num++ ) {
        val = SharedVariable ; 
        printf( "*** thread %d sees value %d\n", which, val ) ;
    	currentThread->Yield() ; 
	SharedVariable = val + 1 ;
	currentThread->Yield() ;
    }
    val = SharedVariable ;
    printf( "Thread %d sees final value %d\n", which, val ) ;
#endif
   
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1( )
{
    DEBUG('t', "Entering ThreadTest1");
 
    for( int i = 0 ; i < threadsNum ; i++ ) {
    	Thread *t = new Thread("forked thread");
    	t->Fork(SimpleThread, i);
    }

}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest( int n )
{	
    threadsNum = n ;	
    switch (testnum) {
    case 1:
	ThreadTest1( );
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}

//----------------------------------------------------------------------
// Elevator

// The elevator is represented as a thread; each student or faculty member is also represented by a thread. 
//---------------------------------------------------------------------

struct Floor
{
    int gettingOn;
    int gettingOff;
};

struct Person
{
    int id;
    int atFloor;
    int toFloor;
};

enum Direction {UP, DOWN, NONE};

struct Floor *floors;
Condition* eleCond;
Lock* eleLock;
Direction direction = UP;
int curFloor;
int occupied;
int elevCap = 5; //Max elevator capacity

//creates elevator and initializes it to have floors=numFloors
void create_elevator(int numFloors)
{
    int i;
    floors = new struct Floor[numFloors];
    for(i = 0; i < numFloors; i++)
    {
	floors[i].gettingOn = 0;
	floors[i].gettingOff = 0;
    }
    eleCond = new Condition("Elevator Condition");
    eleLock = new Lock("Elevator Lock");
    curFloor = -1;
    occupied = 0;
    direction = UP;
}

//Helper function to run elevator
void run_elevator(int numFloors)
{
    do
    {
	//Move to next floor
	eleLock->Acquire();

	int i;
	//elevator is moving count 50 ticks
	for(i = 0; i < 50; i++);

	//update floor number based on direction
	if(direction == UP)
       	    curFloor++;
	else if(direction == DOWN)
	    curFloor--;
	else
	    break;

	printf("Elevator arrives at floor %d.\n", curFloor + 1);

	eleLock->Release();

        //free up people thread to do whatever they please
	currentThread->Yield(); 

	if(curFloor == numFloors - 1)
	    direction = DOWN;
	else if(curFloor == 0)
	    direction = UP;

        //Let people off elevator
	eleLock->Acquire();

	eleCond->Broadcast(eleLock);

	eleLock->Release();

	eleLock->Acquire();

	while(floors[curFloor].gettingOff > 0)
	    eleCond->Wait(eleLock);

	eleLock->Release();

	//people at current floor get on the elevator	
	eleLock->Acquire();

	eleCond->Broadcast(eleLock);
	while(floors[curFloor].gettingOn > 0 && occupied < elevCap)
	    eleCond->Wait(eleLock);    

	eleLock->Release();
    }
    while( occupied > 0 || curFloor < ( numFloors - 1 ) );
 }

//elevator main call
void Elevator(int numFloors)
{
    Thread* elevator = new Thread("Elevator Thread");
    create_elevator(numFloors);
    elevator->Fork(run_elevator, numFloors);
}

//helper for ArrivingGoingFromTo
void run_person(int p)
{
    Person *person = (Person*)p;

    //Arrive at floor
    eleLock->Acquire();

    floors[person->atFloor-1].gettingOn++;
    printf("Person %d wants to go to floor %d from floor %d.\n", 
	   person->id, person->toFloor, person->atFloor);

    eleLock->Release();

    //Wait for the elevator to arrive and have room
    eleLock->Acquire();

    while(curFloor != person->atFloor-1 || occupied == elevCap) { 
	eleCond->Wait(eleLock); }

    //eleLock->Release();
    
    //Get on elevator and wait to arrive at destination floor
    //eleLock->Acquire();

    floors[person->toFloor-1].gettingOff++;
    floors[person->atFloor-1].gettingOn--;
    occupied++;
    printf("Person %d got into the elevator.\n", person->id );
    eleCond->Broadcast(eleLock);

    eleLock->Release();

    eleLock->Acquire();

    while(curFloor != person->toFloor-1) { 
	eleCond->Wait(eleLock); }
   
    //person gets off elevator
    occupied--;
    printf("Person %d got out of the elevator.\n", person->id); 
    floors[person->toFloor-1].gettingOff--;
    eleCond->Broadcast(eleLock);

    eleLock->Release();
}

int nextId;
void ArrivingGoingFromTo(int atFloor, int toFloor)
{
    Thread* person = new Thread("Person Thread");
    Person *p = new Person;
    p->atFloor = atFloor;
    p->toFloor = toFloor;
    p->id = nextId++;
    person->Fork(run_person,(int)p);
}
