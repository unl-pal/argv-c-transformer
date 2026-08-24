/* Regression guard: under Concurrency=forbid, functions touching pthread_t/
 * pthread_mutex_t must be stripped entirely, so no live pthread_* call
 * should ever reach the final .i. */
#include <pthread.h>
#include <stdio.h>

void *worker(void *arg) {
  (void)arg;
  return NULL;
}

int run_threads(void) {
  pthread_t tid;
  pthread_mutex_t lock;
  pthread_mutex_lock(&lock);
  pthread_create(&tid, NULL, worker, NULL);
  pthread_join(tid, NULL);
  pthread_mutex_unlock(&lock);
  return 0;
}
