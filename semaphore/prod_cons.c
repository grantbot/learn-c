#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>

#define NUM_THREADS 2
#define BUFFER_SIZE 10

struct CircBuffer {
  int queue[BUFFER_SIZE];
  int production_semaphore;
  int consumption_semaphore;
};


/**
 * Decrement a resource, then block while waiting for it to become available.
 * @param (sema_ptr) Pointer to an integer condition var.
 */
void sem_wait(int *sema_ptr)
{
  (*sema_ptr)--;

  while (1) {
    if (*sema_ptr >= 0) {
      return;
    }
  }
}

/**
 * Increment a semaphore by 1, to signal the addition of a resource.
 * @param (sema_ptr) Pointer to an integer condition var.
 */
void sem_post(int *sema_ptr)
{
  (*sema_ptr)++;
}


/**
 * Writes random chars to a bounded buffer, waiting politely for enough space.
 */
void *producer(void *circ_buffer)
{
  struct CircBuffer *buffer;
  int cur_index = 0;
  char cur_char;

  buffer = (struct CircBuffer *)circ_buffer;

  while (1)  {
    sem_wait(&buffer->production_semaphore);
    cur_char = 65 + (cur_index % 52);  // A-Z, then a-z, plus a few extra
    buffer->queue[cur_index % 10] = cur_char;
    printf("PROD %c\n", cur_char);
    cur_index++;
    sem_post(&buffer->consumption_semaphore);
    usleep((random() % 100) * 10000);
  }
}

/**
 * Consumes from a bounded buffer, waiting politely for new items.
 */
void *consumer(void *circ_buffer)
{
  struct CircBuffer *buffer;
  int cur_index = 0;
  int cur_char;

  buffer = (struct CircBuffer *)circ_buffer;

  while (1)  {
    sem_wait(&buffer->consumption_semaphore);
    cur_char = buffer->queue[cur_index % 10];
    printf("       CONS %c\n", cur_char);
    cur_index++;
    sem_post(&buffer->production_semaphore);
    usleep((random() % 100) * 10000);
  }
}

int main(void)
{
  // Allocate a circular buffer on main thread's stack
  struct CircBuffer circ_buffer;
  circ_buffer.production_semaphore = BUFFER_SIZE;
  circ_buffer.consumption_semaphore = 0;


  // Kick off consumer and producer, passing them a pointer to our circular buffer
  int cons_thread;
  int prod_thread;
  pthread_t threads[NUM_THREADS];

  cons_thread = pthread_create(&threads[0], NULL, consumer, (void *)&circ_buffer);
  prod_thread = pthread_create(&threads[1], NULL, producer, (void *)&circ_buffer);

  if (cons_thread) {
    printf("ERROR in cons_thread: %d\n", cons_thread);
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(-1);
  }
  if (prod_thread) {
    printf("ERROR in prod_thread: %d\n", prod_thread);
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(-1);
  }

  // Block main thread to support child threads until they're done.
  // Could also use pause()?
  pthread_exit(NULL);
}
