/*
 *    File: thread.h
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * This is header file for common thread functionality
 * wrapper functions
 */

#ifndef _THREAD_H
#define _THREAD_H

#include <pthread.h>

typedef pthread_mutex_t lock_t;

/* thread.c */
extern int thread_spawn(void *(*)(void *), void *);
extern int thread_memlock_init(lock_t);
extern int thread_memlock_fini(lock_t);
extern int thread_memlock_lock(lock_t);
extern int thread_memlock_unlock(lock_t);


#endif /* _THREAD_H */
