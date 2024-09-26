/*
 *    File: linkedlist.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * Simple linked list implementation
 */


#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"


/*
 * Add an entry to the end of the list
 */
struct linkedlist *
linkedlist_append(struct linkedlist *list, void *data)
{
	struct listent *new;

	/* Next entry */
	if ( (new = calloc(1, sizeof(struct listent))) == NULL) {
		anderrs("Failed to allocate memory for a linked list entry\n");
		return(NULL);
	}
	new->data = data;

	/* New list */
	if (list == NULL) {
		if ( (list = calloc(1, sizeof(struct linkedlist))) == NULL) {
			anderrs("Failed to allocate memory for a linked list entry\n");
			return(NULL);
		}

		list->count = 1;	
		list->head = new;	
		list->tail = new;
		return(list);
	}

	/* Append entry */
	list->tail->next = new;
	new->prev = list->tail;
	list->tail = new;
	list->count += 1;
	return(list);	
}



/*
 * Unlink the first entry in the list.
 * A pointer to the link entry data is saved at *data if
 * data is not NULL.
 */
struct linkedlist *
linkedlist_getfirst(struct linkedlist *list, void **data)
{
	/* Empty list */
	if (list == NULL) {
		if (data != NULL)
			*data = NULL;
		return(NULL);
	}

	if (data != NULL)
		*data = list->head->data;
	
	list = linkedlist_unlink(list, list->head);
	return(list);
}


/*
 * Get the last entry in the list
 * A pointer to the link entry data is saved at *data if
 * data is not NULL.
 */
struct linkedlist *
linkedlist_getlast(struct linkedlist *list, void **data)
{
    /* Empty list */
    if (list == NULL) {
        if (data != NULL)
            *data = NULL;
        return(NULL);
    }
	if ((data != NULL) && (list->tail != NULL))
		*data = list->tail->data;
	list = linkedlist_unlink(list, list->tail);
	return(list);
}

/*
 * Returns the link entry if it exists, NULL otherwise
 */
struct listent *
linkedlist_exists(struct linkedlist *list, void *data, comparefunc cmp)
{
	struct listent *ent;

	if (list == NULL)
		return(NULL);

	ent = list->head;
	while (ent != NULL) {

		/* Entry found */
		if (cmp(data, ent->data) == 0) 
			return(ent);	
		ent = ent->next;
	}

	return(NULL);	
}


/*
 * Unlink and free entry.
 */
struct linkedlist *
linkedlist_unlink(struct linkedlist *list, struct listent *ent)
{
	/* Empty list */
	if (list == NULL)
		return(NULL);

	/* Empty entry */
	if (ent == NULL) {
		andlog("%s got NULL pointer as entry\n", __FUNCTION__);
		return(list);
	}

	/* Head entry */
	if (ent == list->head) {
		list->head = list->head->next;
		
		if (list->head != NULL)
			list->head->prev = NULL;
	}

	/* Tail entry */
	else if (ent == list->tail) {
		list->tail = list->tail->prev;
		
		if (list->tail != NULL)
			list->tail->next = NULL;
	}
	
	/* Middle entry */
	else {
		ent->prev->next = ent->next;
		ent->next->prev = ent->prev;
	}

	/* Last entry in list */
	if (((list->head == NULL) && (list->tail != NULL)) ||
			((list->head != NULL) && (list->tail == NULL))) {
		free(list);
		list = NULL;
	}

	if (list != NULL)
		list->count--;

	free(ent);
    return(list);
}



/*
 * Get a specific entry in the list.
 */
struct linkedlist *
linkedlist_get(struct linkedlist *list, 
		void *data, void **save, comparefunc cmp)
{
	struct listent *ent;	

	if ( (ent = linkedlist_exists(list, data, cmp)) != NULL) {
		if (save)
			*save = ent->data;	
		list = linkedlist_unlink(list, ent);
	}
	return(list);	
}



/*
 * Delete an entry from the list
 */
struct linkedlist *
linkedlist_delete(struct linkedlist *list, void *data, comparefunc cmp)
{
	struct listent *ent;	

	if ( (ent = linkedlist_exists(list, data, cmp)) != NULL) 
		list = linkedlist_unlink(list, ent);
	return(list);	
}

