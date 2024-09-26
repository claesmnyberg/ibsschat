/*
 *    File: chat.h
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * This file is the header file for the chat protocol.
 */

#ifndef _CHAT_H
#define _CHAT_H

/* number of times to resend a message
 * if no ACK have been received */
#define MSG_RESEND_TIMES	10

/*
 * This is the ID of the message 
 * used to distinguish messages from each other
 * which is used by the broadcast reciever to 
 * re-send a message again
 */
struct msgid {
	uint32_t ip;    /* Sender IPv4 address */
	uint32_t sec;   /* Sender Seconds (UTC) */
	uint16_t usec;  /* Sender part of micro seconds */
	uint16_t sum;   /* Checksum of message data */
} __attribute__((packed));


/*
 * Make sure each message have the same size
 */
#define MSGSIZE	100

/* The basic message */
struct message {

	/* The different types of messages */
	uint8_t type;
		#define CHAT_DISCOVER 1
		#define CHAT_MSG 2

	struct msgid id;

	/* Crypto IV */
	uint8_t iv[8];

	char pad[MSGSIZE - (sizeof(struct msgid)+1+8)]; 
} __attribute__((packed));


/* Chat Discovery Message */
struct discover {
	uint8_t type;
	struct msgid id;
    char pad[MSGSIZE - (sizeof(struct msgid)+1)]; 
} __attribute__((packed));


struct chatxt {
	char msg[MSGSIZE - (sizeof(struct msgid)+1+8)];  
} __attribute__((packed)); 


/* A single chat message (100 bytes) */
struct chatmsg {
	uint8_t type;
	struct msgid id;
	uint8_t iv[8];
	struct chatxt txt;
} __attribute__((packed));


#define msgtype_valid(m) \
	(((m)->type == CHAT_DISCOVER) || \
	((m)->type == CHAT_MSG))

/* For multicast and discovery */
#define CHAT_SEND_PORT 11012
#define CHAT_RECV_PORT 11013
#define CHAT_GROUP  "239.0.0.1"
#define CHAT_GROUP_PORT 11011

/* chat_proc.c */
extern void chat_proc_run(char *);

/* chat_mcast.c */
extern int chat_mcast_reader_start(uint32_t, uint32_t);
extern void iplist_reset(void);
extern void iplist_clean(uint32_t *);
extern int mcast_send(struct message *, int);

/* chat_crypto.c */
#define CRYPTO_KEY_MAXLEN 60
extern int chat_crypto_init(void);
extern int chat_crypto_set_key(uint8_t *, size_t);
extern size_t chat_crypto_get_key(uint8_t *);
extern int chat_crypto_encrypt(struct message *);
extern int chat_crypto_decrypt(struct message *);

/* msgbuf.c */
extern int msgbuf_add(struct message *);
extern int msgbuf_exist(struct message *);
extern void msgbuf_setid(struct message *);
extern void msgbuf_init(uint32_t);
extern int msgbuf_dump(int, int);
extern int msgbuf_addsock(int);
extern int msgbuf_delsock(int);
extern void msgbuf_print(struct message *);
extern int msgbuf_delete(struct message *);
extern int msgbuf_sync(uint32_t, uint16_t);

/* chat_client.c */
extern int chat_prompt(const char *);
extern int chat_send(const char *, const char *);
extern int chat_send_rand(const char *, int, int);
extern int chat_status(const char *);

#endif /* _CHAT_H */
