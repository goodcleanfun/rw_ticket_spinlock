#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "greatest/greatest.h"
#include "threading/threading.h"

#include "rw_ticket_spinlock.h"

static rw_ticket_spinlock_t lock;
static volatile int counter = 0;
static atomic_uint reads;

#define READ_THREADS 4
#define WRITE_THREADS 2
#define MAX_COUNTER 10000

int increment_counter_thread(void *arg) {
    uint64_t nsec = (uint64_t)arg;
    bool done = false;
    while (1) {
        rw_ticket_spinlock_write_lock(&lock);
        if (counter == MAX_COUNTER) {
            done = true;
        } else {
            counter++;
        }
        rw_ticket_spinlock_write_unlock(&lock);
        if (done) break;
        thrd_sleep(&(struct timespec){.tv_nsec = nsec}, NULL);
    }
    return 0;
}

int read_counter_thread(void *arg) {
    int last_counter = -1;
    while (1) {
        rw_ticket_spinlock_read_lock(&lock);
        if (last_counter != counter) {
            last_counter = counter;
        }
        atomic_fetch_add_explicit(&reads, 1, memory_order_relaxed);
        rw_ticket_spinlock_read_unlock(&lock);
        if (last_counter >= MAX_COUNTER) break;
    }
    return 0;
}


TEST rw_ticket_spinlock_test(void) {
    atomic_init(&reads, 0);
    rw_ticket_spinlock_init(&lock);
    thrd_t write_threads[WRITE_THREADS];
    thrd_t read_threads[READ_THREADS];
    for (int i = 0; i < WRITE_THREADS; i++) {
        ASSERT_EQ(thrd_success, thrd_create(&write_threads[i], increment_counter_thread, (void *)10000));
    }
    for (int i = 0; i < READ_THREADS; i++) {
        ASSERT_EQ(thrd_success, thrd_create(&read_threads[i], read_counter_thread, NULL));
    }
    for (int i = 0; i < WRITE_THREADS; i++) {
        thrd_join(write_threads[i], NULL);
    }
    for (int i = 0; i < READ_THREADS; i++) {
        thrd_join(read_threads[i], NULL);
    }

    ASSERT_GT(atomic_load(&reads), MAX_COUNTER);
    ASSERT_EQ(counter, MAX_COUNTER);

    PASS();
}

// Main test suite
SUITE(rw_ticket_spinlock_tests) {
    RUN_TEST(rw_ticket_spinlock_test);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(rw_ticket_spinlock_tests);

    GREATEST_MAIN_END();
}