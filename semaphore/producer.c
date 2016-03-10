#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>

#define NUM_THREADS 2
#define BUFFER_SIZE 10

// Simulate a bounded buffer
int queue[10];

// Initialize 10 resources to put to, 0 to get from
int production_semaphore = BUFFER_SIZE;
int consumption_semaphore = 0;

// Create pointers to our semaphores
int *prod_sema_ptr = &production_semaphore;
int *cons_sema_ptr = &consumption_semaphore;


/**
 * Block while waiting for a resource to become available.
 * @param (sema_ptr) Pointer to an integer (mimicking a semaphore val).
 * @return Nothing, just unblocks.
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
 * @param (sema_ptr) Pointer to an integer (mimicking a semaphore val).
 */
void sem_post(int *sema_ptr)
{
  (*sema_ptr)++;
}

/**
 * Writes random chars to a bounded buffer, waiting politely for opportunities.
 */
void *producer(void *threadid)
{
  int cur_index = 0;
  char cur_char;
  while (1)  {
    sem_wait(prod_sema_ptr);
    cur_char = 65 + (cur_index % 52);
    queue[cur_index % 10] = cur_char;
    printf("PROD %c\n", cur_char);
    cur_index++;
    sem_post(cons_sema_ptr);
    usleep((random() % 100) * 10000);
  }
}

/**
 * Consumes from a bounded buffer in order, waiting politely for opportunities.
 */
void *consumer(void *threadid)
{
  int cur_index = 0;
  int cur_char;
  while (1)  {
    sem_wait(cons_sema_ptr);
    cur_char = queue[cur_index % 10];
    printf("       CONS %c\n", cur_char);
    cur_index++;
    sem_post(prod_sema_ptr);
    usleep((random() % 100) * 10000);
  }
}

int main(void)
{
  // Kick off consumer and producer, passing to each of them the simulated
  // bounded buffer and pointers to each semaphore.
  int cons;
  int prod;

  pthread_t threads[NUM_THREADS];
  cons = pthread_create(&threads[0], NULL, consumer, NULL);
  prod = pthread_create(&threads[1], NULL, producer, NULL);

  if (cons) {
    printf("ERROR in cons: %d\n", cons);
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(-1);
  }
  if (prod) {
    printf("ERROR in prod: %d\n", prod);
    fprintf(stderr, "Error: %s\n", strerror(errno));
    exit(-1);
  }

  // Block main thread to support child threads until they're done.
  // Could also use pause()?
  pthread_exit(NULL);
}
