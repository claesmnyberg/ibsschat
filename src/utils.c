/*
 *    File: ibsschat.h
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * This file contain utility routines
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>

#if __ANDROID__
#include <android/log.h>
#endif

#include "ibsschat.h"


/*
 * Write to Android log
 */
void
andlog(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

#if __ANDROID__
	__android_log_vprint(ANDROID_LOG_VERBOSE, 
		ANDROID_LOG_TAG, fmt, ap);
	va_end(ap);
#else
	vprintf(fmt, ap);
	va_end(ap);

#endif
}

/*
 * Write error message to Android log
 * and append the error string for the
 * latest error code
 */
void
anderrs(const char *msg)
{
#if __ANDROID__
	__android_log_print(ANDROID_LOG_ERROR, 
		ANDROID_LOG_TAG, "** Error: %s : %s\n", 
		msg, strerror(errno));
#endif

	fprintf(stderr, "** Error: %s : %s\n",
		msg, strerror(errno));
}

/*
 * Write error message to Android log
 * and append the error string for the
 * latest error code
 */
void
anderr(const char *fmt, ...)
{
	va_list ap;

#if __ANDROID__
	va_start(ap, fmt);
	__android_log_vprint(ANDROID_LOG_ERROR, 
		ANDROID_LOG_TAG, fmt, ap);
	va_end(ap);
#endif

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

/*
 * Write N bytes to a file descriptor
 */
ssize_t
writen(int fd, void *buf, size_t n)
{
    size_t tot = 0;
    ssize_t w;

    do {
        if ( (w = write(fd, (void *)((unsigned char *)buf + tot), 
				n - tot)) <= 0)
            return(w);
        tot += w;
    } while (tot < n);

    return(tot);
}

/*
 * Read N bytes from a file descriptor
 */
ssize_t
readn(int fd, void *buf, size_t n)
{
    size_t tot = 0;
    ssize_t r;

    do {
        if ( (r = read(fd, (void *)((unsigned char *)buf + tot), 
				n - tot)) <= 0) 
            return r;
        tot += r;
    } while (tot < n);

    return(tot);
}

/*
 * Convert 6 byte MAC address to string
 */
const char *
net_macstr(const unsigned char *mac)
{
    static unsigned char mstr[48];

    memset(mstr, 0x00, sizeof(mstr));
    snprintf((char *)mstr, sizeof(mstr)-1, "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return((const char *)mstr);
}


/*
 * Fork twice to get a daemon process
 * which we do not have to wait for.
 * Return the new PID on success for the parent
 * and zero for the new daemon process.
 */
int
fork_twice()
{
    pid_t pid;

    pid = fork();

    /* Parent process */
    if (pid > 0) {
        int status;

        /* Wait for child to exit to
         * avoid a zombie */
        waitpid(pid, &status, 0);
        return pid;
    }

    /* Child */
    if (pid == 0) {

        /* fork again and terminate 
         * parent process, the child of
         * the first fork. */
        pid = fork();
        if (pid > 0)
            exit(EXIT_SUCCESS);

        /* Second child */
        if (pid == 0)
            return 0;

        /* Error */
        anderrs("Failed to fork");
        return -1;
    }

    /* Error */
    anderrs("Failed to fork");
    return -1;
}


/*
 * read random bytes non blocking.
 * Returns the number of bytes read on success.
 */
int
getrand_nonblock(uint8_t *buf, size_t len)
{
	int fd;
	size_t n = 0;

	if ( (fd = open("/dev/urandom", O_RDONLY)) > 0) 
		n = read(fd, buf, len);
	else {
		anderrs("Failed to open /dev/urandom");
	}


	/* Fallback if /dev/urandom fail */
	if ((fd < 0) || (n != len)) {
		struct timeval tv;
		size_t i;

		gettimeofday(&tv, NULL);
		srand(tv.tv_sec ^tv.tv_usec ^ getpid());
		for (i=0; i < len; i++)
			buf[i] = rand() & 0xff;
	}

	if (fd >= 0)
		close(fd);

	return len;
}
