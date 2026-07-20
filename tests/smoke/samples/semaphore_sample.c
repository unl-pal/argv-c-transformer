/* Regression guard for the sem_t gap in StdHeaders.hpp's concurrency
 * registry: under Concurrency=forbid, sem_t-touching functions must be
 * stripped just like pthread_t ones, so no live sem_wait/sem_post should
 * ever reach the final .i. */
#include <semaphore.h>

int use_semaphore(void) {
  sem_t sem;
  sem_wait(&sem);
  sem_post(&sem);
  return 0;
}
