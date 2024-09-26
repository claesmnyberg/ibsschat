/*
 *    File: iplist.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * This file implement a simple list of 32 bit 
 * ipv4 addresses.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

#include "ibsschat.h"


/* Local routines */
static int ipcmp(const uint32_t *, const uint32_t *);


static int
ipcmp(const uint32_t *p1, const uint32_t *p2)
{
	if (*p1 < *p2)
		return -1;

	if (*p1 > *p2)
		return 1;

	return 0;
}


/*
 * Add ipv4 to list if it does not exist and sort the
 * list. The ip is expected in network byte order.
 * Return 1 if added, 0 otherwise.
 */
int
iplist_add(uint32_t ip, uint32_t **lst, size_t n)
{
	uint32_t *l;
	size_t i;

	ip = ntohl(ip);

	/* Empty list */
	if (n == 0) {
		*lst = calloc(1, sizeof(uint32_t));
		*lst[n] = ip;
		return 1;
	}

	for (i = 0; i < n; i++) {
		if ( (*lst)[i] == ip)
			return 0;
	}

	/* ip was not part of list, append it */
	l = realloc(*lst, (n+1)*sizeof(uint32_t));
	if (l == NULL) {
		anderrs("Failed to allocate memory");
		return 0;
	}

	/* Sort the list */
	*lst = l;
	l[n] = ip;
	qsort(l, n+1, sizeof(uint32_t), (void *)ipcmp);
	return 1;
}


/*
 * Print list, for debugging.
 */
void
iplist_print(uint32_t *lst, size_t n)
{
	size_t i;

	printf("[");
	for (i=0; i<n; i++) {
		printf("%u, ", lst[i]);
	}
	printf("]\n");

}


/*
 * TODO: Clean old entries (cache time ?)
 */
void
iplist_clean(uint32_t *lst)
{

}
