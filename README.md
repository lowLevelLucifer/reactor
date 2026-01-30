# Project 2 — Event-Driven Reactor with Budgeted Policies (Linux / User Space)

## Overview

This project implements a **policy-aware, event-driven reactor** in user space on a Linux system.

The system models how real operating systems and event loops manage:

- time as a discrete input,
- bounded queues,
- backpressure,
- and controlled internal work via explicit budgets.

The primary goal is **correctness and predictability under load**, not throughput or feature richness.

This project intentionally avoids frameworks, threads, or asynchronous libraries in order to make all control flow and policy decisions explicit and inspectable.



## Design Goals

The system is designed around the following goals:

- **Bounded memory usage**   
    The event queue has a fixed size and explicit overflow behavior.
    
- **Bounded work per time unit**   
    Internal work is strictly limited per timer tick using a budget.
    
- **Explicit policy enforcement**    
    All tradeoffs (dropping events, limiting internal work) are intentional and centralized.
    
- **Predictable behavior under load**  
    No unbounded loops, no implicit amplification of work.
    
- **Separation of concerns**
    Timekeeping, event capture, buffering, policy, and execution are cleanly separated.


## High-Level Architecture

The system follows a classic **reactor pattern**, organized into the following layers:

1. **Time Source Layer**
    Detects real time progression and emits TIMER events.
    
2. **Event Ingestion Layer**
    Converts external stimuli (time) into discrete events.
    
3. **Event Buffering Layer**
    Stores pending events in a bounded FIFO queue.
    
4. **Backpressure Policy Layer**
    Enforces a drop-oldest policy when the queue is full.
    
5. **Policy & Dispatch Layer**
    Controls how much internal work is allowed per time unit.
    
6. **Execution Layer**
    Executes event handlers with strict adherence to policy.



## Event Model
The system operates on two event types:

```c
typedefenum {
    EVENT_TIMER,
    EVENT_INTERNAL
} EventType;

```

- **EVENT_TIMER** 
    Represents the passage of one real second.
    
- **EVENT_INTERNAL**  
    Represents internal work triggered by timer events, subject to policy limits.
    
Each event carries a timestamp for observability and reasoning.



## Event Queue Design
The event queue is implemented as a **fixed-size circular buffer**:

```c
typedefstruct {
    Event buffer[QUEUESIZE];
int head;
int tail;
int size;
} EventQueue;

```


### Key Properties
- **FIFO order**
- **Explicit size tracking**
- **No dynamic memory allocation**
- **Constant-time enqueue and dequeue**

### Overflow Policy (Backpressure)

When the queue is full, the system applies a **drop-oldest** policy:

> The oldest event is discarded to make room for the newest one.
> 

This policy favors **responsiveness to recent input** over preserving historical events.

This tradeoff is intentional and documented.



## Time Handling
Time enters the system through the function:

```c
booltimer_fired(void);

```

This function emits **at most one TIMER event per real second**, even if the reactor loop runs faster than wall-clock time.

This ensures:

- time is **edge-triggered**, not level-triggered
- the system does not flood itself with TIMER events
- time is treated as an **external input**, not an assumption



## Budgeted Internal Work (Policy)
Internal events are governed by a **reactor-owned budget**:

```c
#define INTERNAL_EVENTS_PER_TICK 2

```

### Policy Rules

- The budget is **reset only when a TIMER event is processed**
- Internal events **consume budget**
- Internal events **cannot create more budget**
- When the budget is exhausted, further internal work is denied

This prevents runaway amplification where internal events recursively generate more work.

This mirrors real-world systems such as:

- kernel softirq budgets
- NAPI polling limits
- cooperative schedulers




## Dispatch Logic

Event handling is centralized in a dispatcher:

```c
voiddispatch(Event e, int *budget);

```

### Responsibilities

- Route events to the correct handler
- Enforce policy via budget consumption
- Assert invariants after execution

### Invariant Enforcement

```c
if (*budget <0) {
abort();
}

```

A budget underflow represents a **logic error**, not a recoverable condition.

The system fails loudly to expose invariant violations early.




## Reactor Loop

The core loop follows a strict structure:

1. **Arrival**
    
    Detect new TIMER events.
    
2. **Observation**
    
    Dequeue at most one event.
    
3. **Policy Application**
    
    Reset budget if the event is a TIMER.
    
4. **Execution**
    
    Dispatch the event.
    
5. **Throttling**
    
    Sleep briefly to ensure bounded CPU usage.
    

Only **one event is processed per iteration**, preserving fairness and predictability.

---

## Invariants

The system relies on the following invariants:

- Queue size never exceeds `QUEUESIZE`
- Budget never becomes negative
- Internal events cannot exceed the per-tick budget
- TIMER events are not starved by internal events
- Memory usage is bounded at all times

Any violation of these invariants is treated as a bug.

---

## Tradeoffs

This design makes several explicit tradeoffs:

- **Drop-oldest vs completeness**
    
    Older events may be lost to preserve responsiveness.
    
- **Correctness vs throughput**
    
    The system prioritizes predictable behavior over maximum speed.
    
- **Single-threaded simplicity vs parallelism**
    
    The system avoids concurrency to make policies explicit and analyzable.
    

These tradeoffs are intentional and appropriate for the system’s goals.

---

## What This System Does *Not* Do

This project intentionally does **not** include:

- multithreading
- asynchronous frameworks
- networking
- persistent storage
- real-time guarantees
- dynamic scaling

Those concerns are deferred to later projects where they can be addressed explicitly.

---

## Why This Project Matters

This project demonstrates:

- policy vs mechanism separation
- backpressure handling
- budgeted work
- invariants and correctness reasoning
- portability of systems thinking from embedded to Linux

It serves as a foundation for more complex systems, including distributed and backend architectures.

---

## Next Steps

Future extensions may include:

- multiple event sources
- priority queues
- distributed message passing
- failure isolation across components

Those are intentionally outside the scope of this project.

---

### Final Note

This system is not designed to impress visually.

It is designed to **behave correctly under pressure**.

That is the point.