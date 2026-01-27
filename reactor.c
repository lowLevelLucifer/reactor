#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

/* ================== CONFIG ================== */

#define QUEUESIZE 10
#define INTERNAL_EVENTS_PER_TICK 2   // budget per TIMER event

/* ================== EVENT MODEL ================== */

typedef enum {
    EVENT_TIMER,
    EVENT_INTERNAL
} EventType;

typedef struct {
    EventType type;
    long timestamp;
} Event;

/* ================== QUEUE STATE ================== */

typedef struct {
    Event buffer[QUEUESIZE];
    int head;
    int tail;
    int size;
} EventQueue;

/* ================== GLOBAL STATE ================== */

EventQueue globalQueue;

/* ================== TIME SOURCE ================== */

long now(void) {
    return time(NULL);   // seconds resolution (sufficient here)
}

/* ================== QUEUE OPERATIONS ================== */

bool enqueue(EventQueue *q, Event e) {
    if (q->size == QUEUESIZE) {
        // DROP-OLDEST policy
        q->head = (q->head + 1) % QUEUESIZE;
        q->size--;
        printf("[DROP] queue full — dropped oldest event\n");
    }

    q->buffer[q->tail] = e;
    q->tail = (q->tail + 1) % QUEUESIZE;
    q->size++;

    printf("[ENQ ] type=%d size=%d\n", e.type, q->size);
    return true;
}

bool dequeue(EventQueue *q, Event *out) {
    if (q->size == 0)
        return false;

    *out = q->buffer[q->head];
    q->head = (q->head + 1) % QUEUESIZE;
    q->size--;

    printf("[DEQ ] type=%d size=%d\n", out->type, q->size);
    return true;
}

/* ================== ARRIVAL DETECTION ================== */
/* One TIMER event per real second */

bool timer_fired(void) {
    static long last = 0;
    long current = now();

    if (current > last) {
        last = current;
        return true;
    }
    return false;
}

/* ================== HANDLERS ================== */

void handle_timer(Event e, int *budget) {
    static int tick_count = 0;

    tick_count++;
    printf("HANDLER: TIMER tick %d at %ld\n", tick_count, e.timestamp);

    // Every 5 timer ticks, attempt to emit an internal decision
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
        (*budget)--;   // spend reactor-granted permission
    }
}

void handle_internal(Event e, int *budget) {
    (void)budget; // unused for now
    printf("HANDLER: INTERNAL decision at %ld\n", e.timestamp);
}

/* ================== DISPATCH ================== */

void dispatch(Event e, int *budget) {
    switch (e.type) {
        case EVENT_TIMER:
            handle_timer(e, budget);
            break;

        case EVENT_INTERNAL:
            handle_internal(e, budget);
            break;
    }

    // Invariant check (should never happen)
    if (*budget < 0) {
        printf("[BUG] internal budget underflow — invariant violated\n");
        abort();
    }
}

/* ================== REACTOR LOOP ================== */

void reactor_loop(EventQueue *q) {
    int internal_budget = 0;

    while (1) {

        /* -------- Arrival -------- */
        if (timer_fired()) {
            Event e = {
                .type = EVENT_TIMER,
                .timestamp = now()
            };
            enqueue(q, e);
        }

        /* -------- Observation & Execution -------- */
        Event e;
        if (dequeue(q, &e)) {

            // TIMER events reset the budget
            if (e.type == EVENT_TIMER) {
                internal_budget = INTERNAL_EVENTS_PER_TICK;
            }

            dispatch(e, &internal_budget);

            // Simulate bounded work per cycle
            usleep(200000);  // 200 ms
        }
    }
}

/* ================== MAIN ================== */

int main(void) {
    globalQueue = (EventQueue){0};   // critical initialization
    reactor_loop(&globalQueue);
    return 0;
}
