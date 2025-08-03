# rw_ticket_spinlock
A small, fair, reader-writer ticket-based spinlock which allows many readers to acquire the lock at the same time while allowing only one writer at at time while preventing starvation of writers and readers.
