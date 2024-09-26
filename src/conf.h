/*
 *    File: conf.h
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * This file is the header file for the
 * configuration of the wireless interface
 */

#ifndef _CONF_H
#define _CONF_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <stdint.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/wireless.h>

#define CONF_PORT 11011

#include "chat.h"

/* 
 * Below are the message structures
 * used by the server and client when
 * communicating over the unix socket for
 * configuring the wireless interface.
 * The client send a request that the
 * server respond to with a message.
 */
#define MSG_CODE_OK			0
#define MSG_CODE_ERR		1
#define MSG_REQ_STATUS		2
#define MSG_WI_REQ_CONF		3
#define MSG_WI_STATUS		4

/* Request for wireless interface status */
struct msg_req_status {
	char iface[IW_ESSID_MAX_SIZE];
} __attribute__((packed));

/* Wireless interface information */
struct wiface {
	/* The name of the wireless interface */
	char iface[IFNAMSIZ];
	/* Interface IPv4 address */
	uint32_t ipv4;
	/* Interface network mask */
	uint32_t mask;
	/* Wireless mode (IW_MODE_ADHOC) */
	uint8_t mode;
	/* Wireless channel */
	uint8_t channel;
	/* The hardware address of the access point */
	uint8_t mac_bssid[6];
	/* The name of the ad-hoc network */
	char essid[IW_ESSID_MAX_SIZE];
	/* The encryption key for the chat protocol */
	uint8_t key[CRYPTO_KEY_MAXLEN+1];
} __attribute__((packed));

/* Interface configuration request */
struct msg_req_conf {
	struct wiface wi;
} __attribute__((packed));

/* Message with an error string */
struct msg_err {
	char str[1024];
} __attribute__((packed));

/* Message with interface status information */
struct msg_wi_status {
	struct wiface wi;
} __attribute__((packed));

union msgbuf {
	struct msg_err err;
	struct msg_req_status req_status;
	struct msg_wi_status wi_status;
	struct msg_req_conf wi_conf;
};


/* conf_thread.c */
extern void *conf_thread_run(void *);

/* conf_client.c */
extern int conf_client(int, char **);

#endif /* _CONF_H */
