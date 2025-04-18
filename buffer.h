#ifndef BUFFER_H
#define BUFFER_H

#include <linux/semaphore.h>   // For kernel semaphores
#include <linux/mutex.h>       // For kernel mutexes
#include <linux/time64.h>      // For time structures in the kernel
#include <linux/types.h>       // For basic types like bool
#include <linux/atomic.h>      // For atomic_t
#include "config.h"



// Kernel equivalent of time_t is struct timespec64
struct buffer_item {
    int app_id;
    int priority;
    int sleep_interval; 	// sleep time before processing this message
    char data[MAX_DATA_LEN];
    struct timespec64 timestamp;  // Kernel space, time representation
};

// Use kernel semaphores for circular buffer
struct circular_buffer {
    struct buffer_item items[BUFFER_SIZE];
    int read, write, count;	// index of read and write; count=how many messages in buffer
    struct semaphore mutex;   	// Semaphore for mutual exclusion
    struct semaphore full;    	// Semaphore for tracking filled slots
    struct semaphore empty;   	// Semaphore for tracking empty slots
};

// Use kernel mutex for temp_buffer
struct temp_buffer {
    struct buffer_item items[TEMP_BUFFER_SIZE];
    int count;
    struct mutex mutex;       // Kernel mutex for temp buffer
};

// Declare global variables
extern struct circular_buffer *main_buffer;
extern struct temp_buffer *temp_buffers;

// atomic is a count, but won't be interrupted by other threads
extern atomic_t producers_running;

// Declare functions (syscalls)
void buffer_init(struct temp_buffer *temp_buffers);
void buffer_cleanup(struct temp_buffer *temp_buffers);
void producer_function(void *arg);
void consumer_function(void *arg);

#endif

