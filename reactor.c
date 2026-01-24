#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
//defining the queue size and the event budget to contol the overflow and burden on CPU
#define QUEUESIZE 10
#define INTERNAL_EVENT_BUDGET 2

//an enum function to define the type of events we have
typedef enum {
    EVENT_TIMER,
    EVENT_INTERNAL
} EventType;

//a structure to describe our events
typedef struct {
    EventType type;
    long timestamp;
} Event;

//a simple circular array representation to store our events 
typedef struct {
    Event buffer[QUEUESIZE];
    int head;
    int tail;
    int size;
} EventQueue;

//a global event queue description
EventQueue globalQueue;

//storing the time now
long now(void) {
    return time(NULL);   
}

//a enqueue function to store the upcoming events
bool enqueue(EventQueue *q, Event e) {
	//if our size is equal to the capacity then we drop the oldest event from the queue this makes space for new events
    if (q->size == QUEUESIZE) {
       
        q->head = (q->head + 1) % QUEUESIZE;
        q->size--;
        printf("[DROP] queue full — dropped oldest event\n");
    }
	//the enqueue logic
    q->buffer[q->tail] = e;
    q->tail = (q->tail + 1) % QUEUESIZE;
    q->size++;

    printf("[ENQ ] type=%d size=%d\n", e.type, q->size);
    return true;
}

bool dequeue(EventQueue *q, Event *out) {
	//if our queue is empty then we dont do anything or drop anything
    if (q->size == 0) {
        return false;
    }
	//the out variable helps us to remember the dequeued event 
	//after removing it out from the queue
    *out = q->buffer[q->head];
    q->head = (q->head + 1) % QUEUESIZE;
    q->size--;

    printf("[DEQ ] type=%d size=%d\n", out->type, q->size);
    return true;
}

// the external world timer which indicates this time has passed or certain amount of time has passed
bool timer_fired(void) {
    return true;  
}

//a handle timer this controls the event for the EVENT_TIMER
void handle_timer(Event e, int *budget) {
    static int tick_count = 0;

    tick_count++;
    printf("HANDLER: TIMER event at %ld\n", e.timestamp);

    if (tick_count % 5 == 0) {
        if (*budget <= 0) {
            printf("[DENY] internal event budget exhausted\n");
            return;
        }

        Event ie = {
            .type = EVENT_INTERNAL,
            .timestamp = now()
        };

        enqueue(&globalQueue, ie);
        (*budget)--;   
    }
}

//a handler which controls the event for the EVENT_INTERNAL
void handle_internal(Event e,int *budget) {
    printf("HANDLER: INTERNAL event at %ld\n", e.timestamp);
}

//a dispacther which makes a decision for the handler (Which event has to run now)
void dispatch(Event e) {
	int internal_budget = INTERNAL_EVENT_BUDGET;
    switch (e.type) {
        case EVENT_TIMER:
            handle_timer(e,&internal_budget);
            break;

        case EVENT_INTERNAL:
            handle_internal(e,&internal_budget);
            break;
    }
    //a budget to control the overflow 
    	if (internal_budget < 0) {
    		printf("[BUG] budget underflow — invariant violated\n");
    		abort();  // optional but honest
	}	
}

//a reactor function which simulates like the entry point for this system
void reactor_loop(EventQueue *q) {
    while (1) {
        if (timer_fired()) {
            Event e = {
                .type = EVENT_TIMER,
                .timestamp = now()
            };
            enqueue(q, e);
        }

        Event e;
        while (dequeue(q, &e)) {
            dispatch(e);
            usleep(200000);  
        }
    }
}

// since we are running this on the host event we are using the main function to give control over the reactor
int main(void) {
    globalQueue = (EventQueue){0};  
    reactor_loop(&globalQueue);
    return 0;
}
