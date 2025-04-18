#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/slab.h>       // for kmalloc, kzalloc, kfree
#include <linux/fs.h>         // For filp_open, kernel_read, kernel_write
#include <linux/uaccess.h>    // For accessing user space
#include <linux/time64.h>     // For time in kernel
#include <linux/random.h>     // For random number generation
#include <linux/delay.h>      // for msleep
#include <linux/atomic.h>     // for atomic_t
#include "buffer.h"

#define LINE_BUFFER_SIZE 512	//max 512 for reading each line in the input file

// a mutex for output log
static DEFINE_MUTEX(output_log_mutex);


static struct file *shared_file = NULL;     // shared pointer for input file(3 producers)
static struct semaphore file_sem;           // semaphore for input file

struct circular_buffer *main_buffer;
struct temp_buffer *temp_buffers;
atomic_t producers_running;   		// how many active producers

//1-5 consumer process data to temp buffer and quit, 
//the last consumer processes data and prints all content in temp_buffer to output log
static atomic_t consumers_remaining;

// 1-5 consumer process data to temp buffer and quit, 
// the last consumer processes data and calls this function 
// to print all content in temp_buffer to output log
static void write_temp_buffers_summary(struct file *log_file)
{
    char log_msg[256];
    int i, j;

    mutex_lock(&output_log_mutex);

    // write for temp_buffer 1
    snprintf(log_msg, sizeof(log_msg), "\n[Final Summary] Temp Buffer 1:\n");
    kernel_write(log_file, log_msg, strlen(log_msg), &log_file->f_pos);
    for (j = 0; j < TEMP_BUFFER_SIZE; j++) {
        snprintf(log_msg, sizeof(log_msg),
                 "App%d, %d, %d, [%s]\n",
                 temp_buffers[0].items[j].app_id + 1,
                 temp_buffers[0].items[j].priority,
                 temp_buffers[0].items[j].sleep_interval,
                 temp_buffers[0].items[j].data);
        kernel_write(log_file, log_msg, strlen(log_msg), &log_file->f_pos);
    }

    // write for temp_buffer 1
    snprintf(log_msg, sizeof(log_msg), "\n[Final Summary] Temp Buffer 2:\n");
    kernel_write(log_file, log_msg, strlen(log_msg), &log_file->f_pos);
    for (j = 0; j < TEMP_BUFFER_SIZE; j++) {
        snprintf(log_msg, sizeof(log_msg),
                 "App%d, %d, %d, [%s]\n",
                 temp_buffers[1].items[j].app_id + 1,
                 temp_buffers[1].items[j].priority,
                 temp_buffers[1].items[j].sleep_interval,
                 temp_buffers[1].items[j].data);
        kernel_write(log_file, log_msg, strlen(log_msg), &log_file->f_pos);
    }

    // write for temp_buffer 1
    snprintf(log_msg, sizeof(log_msg), "\n[Final Summary] Temp Buffer 3:\n");
    kernel_write(log_file, log_msg, strlen(log_msg), &log_file->f_pos);
    for (j = 0; j < TEMP_BUFFER_SIZE; j++) {
        snprintf(log_msg, sizeof(log_msg),
                 "App%d, %d, %d, [%s]\n",
                 temp_buffers[2].items[j].app_id + 1,
                 temp_buffers[2].items[j].priority,
                 temp_buffers[2].items[j].sleep_interval,
                 temp_buffers[2].items[j].data);
        kernel_write(log_file, log_msg, strlen(log_msg), &log_file->f_pos);
    }

    mutex_unlock(&output_log_mutex);
}




static void bubble_sort(int sleep_interval)
{
    int delay_size = sleep_interval * 2;  
    int i, j, temp;
    int arr[delay_size];


    for (i = 0; i < delay_size; i++) {
        arr[i] = delay_size - i;
    }


    for (i = 0; i < delay_size - 1; i++) {
        for (j = 0; j < delay_size - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}


SYSCALL_DEFINE0(buffer_init)
{
    int i;

    // allocate memory for the main buffer
    main_buffer = kzalloc(sizeof(struct circular_buffer), GFP_KERNEL);
    if (!main_buffer) {
        printk(KERN_ERR "Failed to allocate memory for main_buffer\n");
        return -ENOMEM;
    }

    // allocate memory for temp buffers
    temp_buffers = kzalloc(sizeof(struct temp_buffer) * NUM_APPS, GFP_KERNEL);
    if (!temp_buffers) {
        printk(KERN_ERR "Failed to allocate memory for temp_buffers\n");
        kfree(main_buffer);
        return -ENOMEM;
    }

    // initialize the mutexes and semaphores
    sema_init(&main_buffer->mutex, 1);
    sema_init(&main_buffer->full, 0);
    sema_init(&main_buffer->empty, BUFFER_SIZE);
    main_buffer->read = 0;
    main_buffer->write = 0;
    main_buffer->count = 0;

    // initialize temp_buffers
    for (i = 0; i < NUM_APPS; i++) {
        temp_buffers[i].count = 0;
		//TODO: initialize mutex for each temp buffer
        
	
    }

    // open input file
    shared_file = filp_open("input.txt", O_RDONLY, 0);
    if (IS_ERR(shared_file)) {
        printk(KERN_ERR "Failed to open input.txt in buffer_init\n");
        kfree(temp_buffers);
        kfree(main_buffer);
        return PTR_ERR(shared_file);
    }

    // TODO: initialize input file semaphore




    // set producer_count = 3, consumer_count = 6
    atomic_set(&producers_running, NUM_PRODUCERS);
    atomic_set(&consumers_remaining, NUM_CONSUMERS);

    printk(KERN_INFO "Buffer initialized successfully\n");
    return 0;
}


SYSCALL_DEFINE0(buffer_cleanup)
{
    int i;

    // TODO: destroy mutexes for temp buffers
	

    //If file opened and the shared_file pointer isn't empty, close the file safely
    if (shared_file && !IS_ERR(shared_file)) {
        filp_close(shared_file, NULL);
        shared_file = NULL;
    }

    // free memory
    if (main_buffer) {
        kfree(main_buffer);
    }
    if (temp_buffers) {
        kfree(temp_buffers);
    }

    printk(KERN_INFO "Buffer cleaned up successfully\n");
    return 0;
}


SYSCALL_DEFINE0(producer_function)
{
    struct file *log_file;
    struct buffer_item item;
    char line[LINE_BUFFER_SIZE];//each line in input file
    ssize_t bytes_read;
    char c;						//how many characters we have read
    char log_msg[256];			//put characters to a line
    int i, num, line_count = 0;

    printk(KERN_INFO "Producer function started\n");

    // open log file, O_APPEND=>write from the end of this file. 
    // and it is atomic, so we don't need to lock the producer log file.
    log_file = filp_open("producer_log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (IS_ERR(log_file)) {
        printk(KERN_ERR "Failed to open producer_log.txt\n");
        return PTR_ERR(log_file);
    }
    printk(KERN_INFO "Producer log file opened successfully\n");


    while (1) { //continue reading from input file line by line untill end of the input file
        
		//TODO: semaphore operation
        

		//read a whole line 
        int idx = 0;
        while (idx < LINE_BUFFER_SIZE - 1) {
            bytes_read = kernel_read(shared_file, &c, 1, &shared_file->f_pos);
            if (bytes_read <= 0)
                break;
            line[idx++] = c;
            if (c == '\n')
                break;
        }
        line[idx] = '\0'; //ending sign \0

		//TODO: semaphore operation
        
		

        if (bytes_read <= 0) {
            printk(KERN_INFO "Producer: End of input.txt or error\n");
            break;
        }

        line_count++;
        printk(KERN_INFO "Processing line %d: %s", line_count, line);


        // parse this line
        num = sscanf(line, "app%d %d %d %s",
                     &item.app_id, &item.priority,
                     &item.sleep_interval, item.data);
        if (num != 4) {
            printk(KERN_ERR "Invalid input format: %s", line);
            continue;
        }

        // app ID=1 => index=0
        item.app_id--;
        ktime_get_real_ts64(&item.timestamp);

        //write to main buffer
        down(&main_buffer->empty);//wait for empty slot
        down(&main_buffer->mutex);//lock main buffer

        main_buffer->items[main_buffer->write] = item;
        main_buffer->write = (main_buffer->write + 1) % BUFFER_SIZE;
        main_buffer->count++;

        up(&main_buffer->mutex);
        up(&main_buffer->full);

        // write log
        snprintf(log_msg, sizeof(log_msg),
                 "Producer: Added message for app %d [Priority: %d, Time: %llu, Message: %s]\n",
                 item.app_id + 1, item.priority,
                 (unsigned long long)item.timestamp.tv_sec, item.data);
        kernel_write(log_file, log_msg, strlen(log_msg), &log_file->f_pos);

        // simulate CPU processing
        // msleep(item.sleep_interval);
	bubble_sort(item.sleep_interval);
    }

    //One producer is finished, producer count-=1
    atomic_dec(&producers_running);

    printk(KERN_INFO "Producer finished processing %d lines\n", line_count);

    filp_close(log_file, NULL);
    return 0;
}


SYSCALL_DEFINE0(consumer_function)
{
    int items_processed = 0;	//how many messages items
    int idx;			//index of temp buffer
    struct buffer_item item;
    struct file *log_file;
    char log_msg[256];

    printk(KERN_INFO "Consumer function started\n");

    // open output log
    log_file = filp_open("output_log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (IS_ERR(log_file)) {
        printk(KERN_ERR "Failed to open output_log.txt\n");
        return PTR_ERR(log_file);
    }
    printk(KERN_INFO "Output log file opened successfully\n");

    while (1) {
        // if no active producer, no item in main buffer, then consumer quit
        if (atomic_read(&producers_running) == 0 && main_buffer->count == 0) {
            break;
        }

        // if three temp buffers are full, then consumer quit
        if (temp_buffers[0].count >= TEMP_BUFFER_SIZE &&
            temp_buffers[1].count >= TEMP_BUFFER_SIZE &&
            temp_buffers[2].count >= TEMP_BUFFER_SIZE) {
            break;
        }

        // try to get a full slot
        if (down_trylock(&main_buffer->full) != 0) { //if no full slot
            // if no full slot and no active producer, then consumer quit
            if (atomic_read(&producers_running) == 0) { 
                break;
            } else {
                msleep(10);
                continue;
            }
        }

        // get data from main buffer
        down(&main_buffer->mutex);
        item = main_buffer->items[main_buffer->read];
        main_buffer->read = (main_buffer->read + 1) % BUFFER_SIZE;
        main_buffer->count--;
        up(&main_buffer->mutex);
        up(&main_buffer->empty);
	
	mutex_lock(&output_log_mutex);
        // format the log message
        snprintf(log_msg, sizeof(log_msg),
                 "Consumer received item for app %d [%s]\n",
                 item.app_id + 1, item.data);
        kernel_write(log_file, log_msg, strlen(log_msg), &log_file->f_pos);
        mutex_unlock(&output_log_mutex);

        //Put the item into a corresponding temp buffer
        if (item.app_id >= 0 && item.app_id < NUM_APPS) {
			
			//TODO: lock the corresponding temp buffer
            
			
            if (temp_buffers[item.app_id].count < TEMP_BUFFER_SIZE) {
                idx = temp_buffers[item.app_id].count;
                temp_buffers[item.app_id].items[idx] = item;
                temp_buffers[item.app_id].count++;

                snprintf(log_msg, sizeof(log_msg),
                         "Consumer: Moved message to temp buffer %d [App: %d, Priority: %d, Count: %d, %s]\n",
                         item.app_id + 1, item.app_id + 1, item.priority,
                         temp_buffers[item.app_id].count, item.data);
            } else {
                snprintf(log_msg, sizeof(log_msg),
                         "Consumer: Temp buffer %d is full, message dropped [App: %d, Priority: %d, %s]\n",
                         item.app_id + 1, item.app_id + 1, item.priority, item.data);
            }
			
			//TODO: unlock the corresponding temp buffer
            

            // write output log
            mutex_lock(&output_log_mutex);
            kernel_write(log_file, log_msg, strlen(log_msg), &log_file->f_pos);
            mutex_unlock(&output_log_mutex);
        }

        items_processed++;
        if (items_processed % 10 == 0) {
            printk(KERN_INFO "Consumer has processed %d items\n", items_processed);
        }
    }

    //For the last consumer, print content of three temp buffers
    if (atomic_dec_return(&consumers_remaining) == 0) {
        printk(KERN_INFO "This is the last consumer. Writing final summary...\n");
        write_temp_buffers_summary(log_file);
    }

    printk(KERN_INFO "Consumer processed %d items, now exiting.\n", items_processed);

    filp_close(log_file, NULL);
    return 0;
}

