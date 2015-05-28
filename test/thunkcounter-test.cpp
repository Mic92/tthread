#include "minunit.h"
#include "xpagelog.h"
#include <execinfo.h>

enum {
  PAGE_SIZE = 4096,
};


void *unlock_mutex(void *data) {
  pthread_mutex_unlock((pthread_mutex_t *)data); // unlock parent
  return NULL;
}

MU_TEST(test_lock_after_pthread_create) {
  pthread_t thread;

  pthread_mutex_t mutex;

  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_lock(&mutex);

  pthread_create(&thread, NULL, unlock_mutex, &mutex);

  // FIXME: obtain lock between pthread_create/pthread_join
  // leads to deadlock
  pthread_mutex_lock(&mutex); // should block until child unlocks
  pthread_join(thread, NULL);
}

typedef struct {
  pthread_mutex_t *mutex;
  char *data;
} lock_unlock_context_t;

void *lock_unlock_mutex(void *data) {
  lock_unlock_context_t *ctx = (lock_unlock_context_t *)data;

  pthread_mutex_lock(ctx->mutex);
  *ctx->data = 'b';
  pthread_mutex_unlock(ctx->mutex);
  return NULL;
}

MU_TEST(test_lock_unlock) {
  char *heapbuf = (char *)malloc(sizeof(char));

  mu_check(heapbuf != NULL);

  pthread_mutex_t mutex;

  pthread_mutex_init(&mutex, NULL);

  pthread_t thread[2];
  xpagelog log = xpagelog::getInstance();
  log.reset();
  lock_unlock_context_t ctx = { .mutex = &mutex, .data = &heapbuf[PAGE_SIZE] };
  pthread_create(&thread[0], NULL, lock_unlock_mutex, &ctx);
  pthread_create(&thread[1], NULL, lock_unlock_mutex, &ctx);

  pthread_join(thread[0], NULL);
  pthread_join(thread[1], NULL);

  int accesses = 0;

  // expect 2 logged write access to ctx.data
  for (int i = 0; i < log.len(); i++) {
    xpagelogentry e = log.get(i);

    if (e.getFirstAccessedAddress() == ctx.data) {
      accesses++;
      mu_check(e.getAccess() == xpagelogentry::WRITE);

      // address for mutex lock should come after function start
      mu_check(e.getFirstAccessedAddress() > ((const void *)lock_unlock_mutex));

      // 1. thunk: pthread_create()
      // 2. thunk: pthread_mutex_lock()
      mu_check(e.getThunkId() == 2);
    }
  }

  mu_check(accesses == 2);
  mu_check(*ctx.data == 'b');

  // Bug again
  // free(heapbuf);
}

MU_TEST_SUITE(test_suite) {
  // FIXME see testcase
  // MU_RUN_TEST(test_lock_after_pthread_create);
  MU_RUN_TEST(test_lock_unlock);
}

int main(int argc, char **argv) {
  MU_RUN_SUITE(test_suite);
  MU_REPORT();
  return 0;
}