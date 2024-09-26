/*
 *    File: thread.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * Wrapper functions for thread related routines
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "ibsschat.h"


/*
 * Create a new thread in function start_func.
 * Returns 0 on success, non zero on error.
 */
int
thread_spawn(void *(*start_func)(void *), void *arg)
{
    pthread_t thread;
    pthread_attr_t attr;
    int r;

    if (start_func == NULL) {
		anderr("Missing start function");
        return(-1);
    }

    if ( (r = pthread_attr_init(&attr)) != 0) {
		anderrs("pthread_attr_init failed");
        return(r);
    }

    if ( (r = pthread_attr_setdetachstate(&attr,
                PTHREAD_CREATE_DETACHED)) != 0) {
		anderrs("pthread_attr_setdetachstate failed");
        return(r);
    }

    if ( (r = pthread_create(&thread, &attr, start_func, arg)) != 0) {
		anderrs("pthread_create failed");
        return(r);
    }

    if ( (r = pthread_attr_destroy(&attr)) != 0) {
		anderrs("pthread_attr_destroy failed");
    }


    return(r);
}

/*
 * Initialize memory lock.
 * Return 0 on success, -1 on error.
 */
int
thread_memlock_init(lock_t lock)
{
	if (pthread_mutex_init(&lock, NULL) != 0) {
		anderrs("pthread_mutex_init failed");
		return -1;
	}

	return 0;
}

/*
 * Destroy memory lock.
 * Return 0 on success, -1 on error.
 */
int
thread_memlock_fini(lock_t lock)
{
	if (pthread_mutex_destroy(&lock) != 0) {
		anderrs("pthread_mutex_destroy failed");
		return -1;
	}

	return 0;
}

/*
 * Lock memory lock.
 * Return 0 on success, -1 on error.
 */
int
thread_memlock_lock(lock_t lock)
{
	if (pthread_mutex_destroy(&lock) != 0) {
		anderrs("pthread_mutex_lock failed");
		return -1;
	}

	return 0;
}

/*
 * Unlock memory lock.
 * Return 0 on success, -1 on error.
 */
int
thread_memlock_unlock(lock_t lock)
{
	if (pthread_mutex_unlock(&lock) != 0) {
		anderrs("pthread_mutex_unlock failed");
		return -1;
	}

	return 0;
}
