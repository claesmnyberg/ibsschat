/*
 *    File: linkedlist.h
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * Header file for simple linked list implementation
 */


#ifndef _CMN_LINKEDLIST_H
#define _CMN_LINKEDLIST_H

#include <sys/types.h>
#include "ibsschat.h"

/* The comapre function */
typedef int (*comparefunc)(const void *, const void *);

/* List entry */
struct listent {
	struct listent *next;
	struct listent *prev;
	void *data;
};

/* List head */
struct linkedlist {
	size_t count;	/* Number of entries in list */
	struct listent *head;	/* List head */
	struct listent *tail;	/* Last entry in list */
};


/* linkedlist.c */
extern struct linkedlist *linkedlist_append(struct linkedlist *, void *);
extern struct linkedlist *linkedlist_getfirst(struct linkedlist *, void **);
extern struct linkedlist *linkedlist_getlast(struct linkedlist *, void **);
extern struct listent *linkedlist_exists(struct linkedlist *, void *, comparefunc);
extern struct linkedlist *linkedlist_get(struct linkedlist *, void *, void **, comparefunc);
extern struct linkedlist *linkedlist_delete(struct linkedlist *, void *, comparefunc);
extern struct linkedlist *linkedlist_unlink(struct linkedlist *, struct listent *);

#define linkedlist_elements(list) ((list) == NULL ? 0 : (list)->count)

#endif /* _CMN_LINKEDLIST_H */
