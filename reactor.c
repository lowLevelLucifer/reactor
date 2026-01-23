#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<stdbool.h>
#include<unistd.h>
//defining the default queue size
#define QUEUESIZE 10
//an enum function to take the type of event
typedef enum{
  EVENT_TIMER, 
  EVENT_INTERNAL
}EventType;

//a structure of Event which stores the type of event and when it has occured
typedef struct{
  EventType type; 
  long timeStamp;
}Event; 

//a basic circular array to store our events
typedef struct{
  //has the array of size 10
  Event buffer[QUEUESIZE]; 
//a head which holds the oldest event
  int head; 
//tail to hold the current event
  int tail; 
//size of the queue to push and pop
  int size;
}EventQueue;

bool enqueue(EventQueue *q ,Event e){
  if(q->size == QUEUESIZE){
    //this checks if the queue is full , if true then we move the head forward and dequeue the oldest event
    q->head = (q->head+1) % QUEUESIZE;
    q->size--;
    printf("Queue OverFlowed, oldest events dropped...\n");
  }
  //else we just push the event in the array
  q->buffer[q->tail] = e;
  q->tail = (q->tail + 1) % QUEUESIZE;
  q->size++;
  return true;
}

bool dequeue(EventQueue *q, Event *out){
  if(q->size == 0){
    //if the queue is empty then we dont have to dequeue anything
    return false;
  }
  //else we store the oldest event in the out and move the head to the next oldest event
  *out = q->buffer[q->head]; 
  q->head = (q->head + 1) % QUEUESIZE;
  q->size--; 
  return true;
}

bool timer_fired(){
  //a timer function to keep track of time of outside world and fire the timer_fired() for the TIMER_EVENT to occur
  static long last = 0; 
  long now = get_time_ms();

  if(now - last >= 1000){
    last = now;
    return true;
  }
  return false;
}

void reactor_loop(EventQueue *q){
  while(1){
    //this runs on a infinity loop
    if(timer_fired()){
      //we enter this when our function has to react to the outside world timer
      Event e = {
      //store the event and timeStamp
        .type = EVENT_TIMER; 
        .timeStamp = now()
      };
      //we enqueue the stored values
      enqueue(q,e);
    }
    Event e; 
    //we dequeue and print the current processing event
    while(dequeue(q,&e)){
      printf("LOG processing... Type %d at Time: %ld\n",.type,.timeStamp);
    }
  }
}

