/*
 *    File: msgbuf.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * The chat buffer
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "linkedlist.h"
#include "ibsschat.h"


/* Maximum number of messages in buffer */
#define MAXMSGS	1000


/* The message structure */
struct msg {

	/* The number of times message have been seen */
	uint32_t count;

	/* Time stamp when message was first seen */
	struct timeval tv;

	/* The message */
	struct message msg;

} __attribute__((packed));


/* Local variables */
static lock_t buflock;
static struct linkedlist *msgbuf;
static uint32_t myipv4;

/* Maximum number of clients that receive new messages */
#define MAXCLIENTS 20
static lock_t socklock;
static int socklist[MAXCLIENTS];


/* Local routines */
static struct msg *msgbuf_get(struct msgid *);
static int msgcmp(const void *, const void *);
static struct msg *msg_create(struct message *, time_t);
static int msgbuf_write_socklist(struct message *, int);
static int msgbuf_append(struct msg *);

/*
 * Message compare function.
 */
static int
msgcmp(const void *a, const void *b)
{
	struct message *m1;
	struct message *m2;

	struct msgid *i1 = NULL;
	struct msgid *i2 = NULL;


	/* Shut up compiler */
	i1++; i2++; i1 = NULL; i2 = NULL;

	/* Find the ID structure */
	m1 = &(((struct msg *)a)->msg);
	m2 = &(((struct msg *)b)->msg);
	i1 = &m1->id;
	i2 = &m2->id;

#if 0
	andlog("Comparing %08x%08x%02x%02x and %08x%08x%02x%02x\n",
		i1->ip, i1->sec, i1->usec, i1->sum,
		i2->ip, i2->sec, i2->usec, i2->sum);
#endif

	return memcmp((const void *)&m1->id, (const void *)&m2->id, sizeof(struct msgid));
}


/*
 * Create a message entry.
 * Returns the pointer to the allocated
 * memory on success, NULL on error.
 */
static struct msg *
msg_create(struct message *m, time_t sec)
{
	struct msg *mg;

	if ( (mg = calloc(1, sizeof(struct msg))) == NULL) {
		anderrs("Failed to allocate memory");
		return NULL;
	}

	if (sec == 0)
		gettimeofday(&mg->tv, NULL);
	else
		mg->tv.tv_sec = sec;

	mg->count = 1;
	memcpy(&(mg->msg), m, sizeof(struct message));
	return mg;
}

/*
 * Initialize the message buffer.
 */
void
msgbuf_init(uint32_t ip)
{
	int i=0;

	/* Initialize lock */
	thread_memlock_init(buflock);
	thread_memlock_init(socklock);
	msgbuf = NULL;
	myipv4 = ip;

	/* Initialize client sockets */
	for (i=0; i<MAXCLIENTS; i++)
		socklist[i] = -1;
}

/*
 * Synchronize by connecting to another node
 * and download messages.
 * Returns the number of messages read on
 * success, -1 on error.
 */
int
msgbuf_sync(uint32_t ip, uint16_t port)
{
	struct message msg;
	struct in_addr sad;
	int count = 0;
	int sock;

	sad.s_addr = ip;

	andlog("[SYNC] Attemting to synchronize with %s:%u\n",
		inet_ntoa(sad), ntohs(port));

	if ( (sock = tcp_connect(ip, port, 0, 0)) < 0) {
		anderr("** Error: Failed to connect to sync client on %s:%u\n",
			inet_ntoa(sad), ntohs(port));
		return -1;
	}

	/* Wait for data to be written on the other end
	 * to avoid getting blocked on the fd until next message 
	 * if the buffer is empty on the other side (very rare though ...)*/	
	sleep(1);

	while (readn(sock, &msg, sizeof(msg)) == sizeof(msg)) {
		struct msg *mb;

		/* Decrypt */
		chat_crypto_decrypt(&msg);

		thread_memlock_lock(buflock);

		/* Message does not exist */
		if ( (mb = msgbuf_get(&msg.id)) == NULL) {

			andlog("[SYNC] Read buffered message %u from %s:%u\n",
				count + 1, inet_ntoa(sad), ntohs(port));

			/* Create the message */
			if ( (mb = msg_create(&msg, msg.id.sec)) == NULL) {
				thread_memlock_unlock(buflock);
				return count;
			}

			/* Make sure the messages have been seen */
			mb->count = 2;

			/* Append it */
			msgbuf_append(mb);
			count++;
		}
		
		thread_memlock_unlock(buflock);
	}

	close(sock);
	return count;
}

/*
 * Add socket to list of descriptors.
 * Returns 0 on success, -1 on error.
 */
int
msgbuf_addsock(int sock)
{
	int ret = -1;
	int i;

	thread_memlock_lock(socklock);
	
	/* Check if socket already exist */
	for(i=0; i < MAXCLIENTS; i++) {
		if (socklist[i] == sock) {
			ret = 0;
			goto finished;
		}
	}

	for(i=0; i < MAXCLIENTS; i++) {
		if (socklist[i] == -1) {
			socklist[i] = sock;
			ret = 0;
			break;
		}
	}

	finished:
	
	thread_memlock_unlock(socklock);

	if (ret == -1)
		andlog("** Error: Could not find empty slot for client socket\n");

	return ret;
}

/*
 * Delete socket from list.
 * Return 0 on success, -1 on error.
 */
int
msgbuf_delsock(int sock)
{
	int ret = -1;
	int i;

	thread_memlock_lock(socklock);
	for(i=0; i < MAXCLIENTS; i++) {
		if (socklist[i] == sock) {
			socklist[i] = -1;
			ret = 0;
			break;
		}
	}
	thread_memlock_unlock(socklock);

	if (ret == -1)
		andlog("** Error: Could not find client socket in list\n");
	return ret;
}


/*
 * Write message to all clients in socket list.
 * Return the number of sockets written to.
 */
static int
msgbuf_write_socklist(struct message *m, int count)
{
	int ret = 0;
	int i;

	/* Require local messages to be acknowledged, 
	 * as in seen at least twice */
	if (m->id.ip == myipv4) {
		if (count <= 1) {
			return 0;
		}
	}

	
	thread_memlock_lock(socklock);
	for(i=0; i < MAXCLIENTS; i++) {
		if (socklist[i] != -1) {
			if (writen(socklist[i], m, sizeof(struct message)) !=
					sizeof(struct message)) {
				anderrs("msgbuf_write_socklist() Failed to write to socket");
				andlog("Deleting socket %u because of failed write\n", 
					socklist[i]);
				close(socklist[i]);
				socklist[i] = -1;
			}
			ret++;
		}
	}
	thread_memlock_unlock(socklock);
	return ret;
}


/*
 * Append message to buffer,
 * delete first message if 
 * buffer is full.
 * Return 0 on success, -1 on error.
 * Buffer must be locked when calling this function.
 */
static int
msgbuf_append(struct msg *m)
{
	/* Append it to the list */
	msgbuf = linkedlist_append(msgbuf, m);

	/* Delete first (oldest) entry 
     * If we reached the maximum number of messages */
	if (linkedlist_elements(msgbuf) > MAXMSGS) {
		if ((msgbuf->head == NULL) || (msgbuf->head->data == NULL)) {
			andlog("** Error: head of list is NULL!!!");
		}
		else {
			free(msgbuf->head->data);
			msgbuf = linkedlist_unlink(msgbuf, msgbuf->head);	
		}
	}

	andlog("%u messages in message buffer\n", 
		linkedlist_elements(msgbuf));
	return 0;
}

/*
 * Add message to chat buffer.
 * Returns the number of times that the message
 * have been seen, i.e. the number of times
 * that the message have been added using this routine.
 */
int
msgbuf_add(struct message *m)
{
	struct msg *mb = NULL;
	uint32_t count = 1;

	if (m == NULL) {
		anderrs("msgbuf_add() Received NULL pointer!");
		return -1;
	}

	if (msgtype_valid(m) == 0) {
		andlog("Refusing to add message with unvalid type\n");
		return -1;
	}

	thread_memlock_lock(buflock);

	/* Message exist, increase counter */
	if ( (mb = msgbuf_get(&m->id)) != NULL) {
		mb->count = mb->count + 1;
		count = mb->count;

		thread_memlock_unlock(buflock);

		andlog("msgbuf_add(): Message %08x%08x%02x%02x: seen %u times\n",
			m->id.ip, m->id.sec, m->id.usec, m->id.sum, count);

		/* If this is a message from us, we write it
         * to connected clients when seen two times */
		if ((count == 2) && (m->id.ip == myipv4))
			msgbuf_write_socklist(m, count);

		return count;
	}

	/* Append message */
	andlog("msgbuf_add(): Adding messsage %08x%08x%02x%02x\n", 
		m->id.ip, m->id.sec, m->id.usec, m->id.sum);

	/* Create the new message */
	if ( (mb = msg_create(m, 0)) == NULL) {
		thread_memlock_unlock(buflock);
		return -1;
	}

	/* Append it to the list */
	msgbuf_append(mb);


	/* Unlock and return counter */
	thread_memlock_unlock(buflock);

	{
		struct chatmsg *cm = NULL;
		struct in_addr sad;

		sad.s_addr = m->id.ip;

		/* Log information */
		switch (m->type) {

			case CHAT_DISCOVER:
				andlog("Discovery from %s\n", inet_ntoa(sad));
				break;

			case CHAT_MSG:
				cm = (struct chatmsg *)m;
				andlog("Text from %s (%08x%08x%04x%04x): \"%s\"\n",
					inet_ntoa(sad), cm->id.ip, cm->id.sec,
					cm->id.usec, cm->id.sum, cm->txt.msg);
				break;

		}
	}

	/* Write the message to all connected clients */
	msgbuf_write_socklist(m, count);
	return count;
}


/*
 * Get the message if it exist in buffer.
 * Return a pointer on success, NULL if the
 * message does not exist.
 *
 * Message buffer need to be locked when calling
 * this function.
 */
static struct msg *
msgbuf_get(struct msgid *id)
{
	struct listent *ent;
	struct msg m;

	memcpy(&m.msg.id, id, sizeof(struct msgid));
	ent = linkedlist_exists(msgbuf, &m, msgcmp);
	if (ent != NULL)
		return (struct msg *)ent->data;	

	return NULL;
}


/*
 * Check if message exist in chat buffer.
 * Return the number of times the message have been seen.
 */
int
msgbuf_exist(struct message *m)
{
	struct listent *ent;
	struct msg ms;
	int count = 0;

	if (m == NULL) {
		anderrs("msgbuf_exist() Received NULL pointer!");
		return -1;
	}

	memcpy(&ms.msg, m, sizeof(struct message));
	andlog("Checking if message %08x%08x%02x%02x exist.\n", 
		m->id.ip, m->id.sec, m->id.usec, m->id.sum);

	thread_memlock_lock(buflock);
	ent = linkedlist_exists(msgbuf, &ms, msgcmp);
	if (ent != NULL) {
		struct msg *md;

		md = (struct msg *)ent->data;
		count = md->count;
	}

	thread_memlock_unlock(buflock);
	return count;
}

/*
 * Delete a specific message from the list.
 * Return 1 if message was found and deleted,
 * 0 otherwise.
 */
int
msgbuf_delete(struct message *m)
{
    struct listent *ent;
    struct msg ms;
    int ret = 0;

    if (m == NULL) {
        anderrs("msgbuf_exist() Received NULL pointer!");
        return -1;
    }

    memcpy(&ms.msg, m, sizeof(struct message));

    thread_memlock_lock(buflock);

    ent = linkedlist_exists(msgbuf, &ms, msgcmp);
    if (ent != NULL) {
	    andlog("Deleting message %08x%08x%02x%02x\n",
    	    m->id.ip, m->id.sec, m->id.usec, m->id.sum);
		free(ent->data);
		msgbuf = linkedlist_unlink(msgbuf, ent);	
     	ret = 1;
    }

    thread_memlock_unlock(buflock);
    return ret;
}

/*
 * Compute and set the message identifier.
 */
void
msgbuf_setid(struct message *m)
{
    struct timeval tv;

    m->id.ip = myipv4;

    /* Time */
    gettimeofday(&tv, NULL);
    m->id.sec = htonl(tv.tv_sec);
    m->id.usec = htons((uint16_t)(tv.tv_usec & 0xffff));
	m->id.sum = 0;

    /* Add checksum */
    m->id.sum = chksum((uint16_t *)m, (sizeof(struct message) >> 1));
    m->id.sum = htons(m->id.sum);
}


/*
 * Write messages in buffer to file
 * descriptor. Returns the number of 
 * written messages on success, -1 on error.
 */
int
msgbuf_dump(int fd, int encrypt)
{
	int ret = 0;
	struct listent *ent;

	andlog("[+] Attempting to dump all messages to descriptor %d\n", fd);
	thread_memlock_lock(buflock);

	andlog("[**] msgbuf_dump(): Got lock!, dumping %u messages\n",
		linkedlist_elements(msgbuf));

	ent = msgbuf->head;
	while (ent != NULL) {
		struct message mc;
		struct msg *m = (struct msg *)ent->data;

		/* Require local messages to be acknowledged, 
		 * as in seen twice */
		if (m->msg.id.ip == myipv4) {
			if (m->count <= 1) {
				ent = ent->next;
				continue;
			}
		}

		/* Copy and encrypt message */
		memcpy(&mc, &m->msg, sizeof(struct message));
		if (encrypt)
			chat_crypto_encrypt(&mc);		

		/* Send message to client */
		if (writen(fd, &mc, sizeof(struct message)) != 
				sizeof(struct message)) {
			anderrs("Failed to write message to file descriptor");
			break;
		}

		ent = ent->next;
		ret++;
	}

	thread_memlock_unlock(buflock);
	andlog("[**] msgbuf_dump(): Released lock!\n");
	return ret;

}

/*
 * Pretty print message
 */
void
msgbuf_print(struct message *msg)
{
	struct in_addr sad;
	const struct tm *tm;
	char tbuf[128];
	time_t t;

	sad.s_addr = msg->id.ip;
	t = ntohl(msg->id.sec);
	tm = localtime(&t);
	strftime(tbuf, sizeof(tbuf), "%H:%M:%S", tm);

	if (msg->type == CHAT_DISCOVER) {
		printf("[discovered %s %s]\n", inet_ntoa(sad), tbuf);
	}

	if (msg->type == CHAT_MSG) {
		struct chatmsg *cm;
		cm = (struct chatmsg *)msg;
		printf("[%s %s] %s\n", inet_ntoa(sad), tbuf, cm->txt.msg);
	}
}
