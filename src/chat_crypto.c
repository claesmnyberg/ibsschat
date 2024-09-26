/*
 *    File: chat_crypto.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * Wrapper functions for symmetric encryption
 *
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "ibsschat.h"
#include "libbfish/bfish.h"

/* Local routines */

/* Local Variables */
static lock_t keylock;

static uint8_t key[CRYPTO_KEY_MAXLEN+1];
static size_t keylen = 0;
static int key_set = 0;

/* The blowfish key */
struct bfish_key *bkey;

/*
 * Initialize the crypto system
 * Return 0 on success, -1 on error.
 */
int
chat_crypto_init(void)
{
	thread_memlock_init(keylock);
	return 0;
}


/*
 * Set the encryption key.
 * Return 0 on succes, -1 on error.
 */
extern int
chat_crypto_set_key(uint8_t *newkey, size_t len)
{
	struct bfish_key *bk;

	if (len > CRYPTO_KEY_MAXLEN) {
		fprintf(stderr, "** Error: Key exceed maximum length\n");
		andlog("** Error: Key exceed maximum length\n");
		return -1;
	}

	if ( (bk = bfish_keyinit(newkey, len)) == NULL) {
		anderr("** Error: Failed to initialize Blowfish key\n");
		return -1;
	}

	thread_memlock_lock(keylock);

	if (bkey != NULL)
		free(bkey);
	bkey = bk;

	memset(key, 0x00, sizeof(key));
	memcpy(key, newkey, len);
	keylen = len;
	key_set = 1;

	thread_memlock_unlock(keylock);

	return 0;
}

/*
 * Get the encryption key.
 * Return the length of the key on success, 0 on error
 * or if the key has not been set.
 * The memory pointed to by buf must be at least CRYPTO_KEY_MAXLEN
 * bytes long.
 */
size_t
chat_crypto_get_key(uint8_t *buf)
{
	if (key_set != 0) {
		memcpy(buf, key, CRYPTO_KEY_MAXLEN);
		return keylen;
	}

	andlog("** Error: Encryption key not set\n");
	return 0;
}


/*
 * Encrypt chat message.
 * Return 0 on success, -1 on error.
 */
int
chat_crypto_encrypt(struct message *m)
{
	uint8_t *buf;
	size_t len;

	if (key_set == 0) {
		andlog("** Error: encryption key not set\n");
		return -1;
	}

    /* Generate a random IV for this message.
     * Use non-blocking random just for this example
     * program to avoid non blocking, should use 
     * /dev/random for better security though */
	getrand_nonblock(m->iv, sizeof(m->iv));

	/* Encrypt message */
	thread_memlock_lock(keylock);
	buf = (uint8_t *)m;
	buf += (sizeof(m->type) + sizeof(struct msgid) + sizeof(m->iv));
	len = sizeof(struct message);
	len -= (sizeof(m->type) + sizeof(struct msgid) + sizeof(m->iv));
	bfish_cbc_encrypt(buf, len, m->iv, bkey);
	thread_memlock_unlock(keylock);

	return 0;
}


/*
 * Decrypt chat message.
 * Return 0 on success, -1 on error.
 */
int
chat_crypto_decrypt(struct message *m)
{
	uint8_t *buf;
	size_t len;

	if (key_set == 0) {
		andlog("** Error: encryption key not set\n");
		return -1;
	}


	/* Decrypt message */
	thread_memlock_lock(keylock);
	buf = (uint8_t *)m;
	buf += (sizeof(m->type) + sizeof(struct msgid) + sizeof(m->iv));
	len = sizeof(struct message);
	len -= (sizeof(m->type) + sizeof(struct msgid) + sizeof(m->iv));
	bfish_cbc_decrypt(buf, len, m->iv, bkey);
	thread_memlock_unlock(keylock);

	return 0;
}
