#ifndef RW_TICKET_SPINLOCK_H
#define RW_TICKET_SPINLOCK_H

#include <stdatomic.h>
#include <stdbool.h>

#include "aligned/aligned.h"
#include "cpu_relax/cpu_relax.h"
#include "threading/threading.h"

#define MAX_PAUSE_ITERATIONS 40

typedef struct rw_ticket_spinlock {
    atomic_uint_fast8_t write;
    atomic_uint_fast8_t read;
    atomic_uint_fast8_t ticket;
} rw_ticket_spinlock_t;

static void rw_ticket_spinlock_init(rw_ticket_spinlock_t *lock) {
    atomic_init(&lock->write, 0);
    atomic_init(&lock->read, 0);
    atomic_init(&lock->ticket, 0);
}


static void rw_ticket_spinlock_write_lock(rw_ticket_spinlock_t *lock) {
    uint8_t me = atomic_fetch_add(&lock->ticket, 1);

    uint8_t write_serving;
    for (size_t i = 0; i < MAX_PAUSE_ITERATIONS; i++) {
        write_serving = atomic_load_explicit(&lock->write, memory_order_acquire);
        if (write_serving == me) return;
        cpu_relax();
    }

    while (true) {
        write_serving = atomic_load_explicit(&lock->write, memory_order_acquire);
        if (write_serving == me) return;
        thrd_yield();
    }
}

static void rw_ticket_spinlock_write_unlock(rw_ticket_spinlock_t *lock) {
    atomic_fetch_add(&lock->read, 1);
    atomic_fetch_add(&lock->write, 1);
}


static void rw_ticket_spinlock_read_lock(rw_ticket_spinlock_t *lock) {
    uint8_t me = atomic_fetch_add(&lock->ticket, 1);

    uint8_t read_serving;
    bool read_lock_acquired = false;
    for (size_t i = 0; i < MAX_PAUSE_ITERATIONS; i++) {
        read_serving = atomic_load_explicit(&lock->read, memory_order_acquire);
        if (read_serving == me) {
            read_lock_acquired = true;
            break;
        }
        cpu_relax();
    }
    if (!read_lock_acquired) {
        while (true) {
            read_serving = atomic_load_explicit(&lock->read, memory_order_acquire);
            if (read_serving == me) break;
            thrd_yield();
        }
    }

    atomic_fetch_add_explicit(&lock->read, 1, memory_order_release);
}

static void rw_ticket_spinlock_read_unlock(rw_ticket_spinlock_t *lock) {
    atomic_fetch_add_explicit(&lock->write, 1, memory_order_release);
}

#endif