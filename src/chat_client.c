/*
 *    File: conf_client.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * IBSS configuration client
 */

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

/* Local routines */
static void *client_read_messages(void *iface);

/* Number of messages read */
static uint32_t msgcount = 0;

/* Neighbours seen */
static uint32_t *iplist = NULL;
static uint32_t ips = 0;

/* Lock */
lock_t statlock;

/*
 * Thread entry point.
 * Thread that read incoming messages and print them
 * to the terminal.
 */
static void *
client_read_messages(void *iface)
{
	struct message msg;
	int sock;
	uint32_t ip;
	uint32_t mask;

	

	if (ifconfig_get((char *)iface, &ip, &mask) < 0) {
		fprintf(stderr, "**Error: Failed to get IPv4 address of interface %s: %s", 
			(char *)iface, strerror(errno));
		return NULL;
	}

	/* Connect to daemon */
	if ( (sock = tcp_connect(ip, htons(CHAT_RECV_PORT), 0, 0)) < 0) {
		fprintf(stderr, "Failed to connect to daemon (is it running?): %s\n",
			strerror(errno));
		exit(EXIT_FAILURE);
		return NULL;
	}

	 /* Read messages forever and print them on terminal */
	while (readn(sock, &msg, sizeof(msg)) == sizeof(msg)) {

		/* Statistics */
		thread_memlock_lock(statlock);
		msgcount++;
		if (iplist_add(htonl(msg.id.ip), &iplist, ips))
			ips++;
		thread_memlock_unlock(statlock);

		/* Print message */
		msgbuf_print(&msg);
	}

	fprintf(stderr, "Message reader thread terminating\n");
	close(sock);
	exit(EXIT_FAILURE);
	return NULL;
}


/*
 * Simple chat prompt for sending and receiving 
 * messages.
 */
int
chat_prompt(const char *iface)
{
	struct chatxt txt;
	struct in_addr ina;
	uint32_t mask;
	int i=0;


	if (wiface_adhoc(iface) != 1) {
		andlog("** Error: Interface is not up and running in ad-hoc mode\n", iface);
		andlog("** Error: Please configure interface in ad-hoc mode before runnig client\n");
		fprintf(stderr, "** Error: Interface %s has not been configured\n",
			iface);
		return -1;
	}

	/* Initialize lock */
	thread_memlock_init(statlock);

	printf("[Connected to local chat server]\n");
	printf("[Type .help for help]\n");

	/* Spawn reader thread */
	if (thread_spawn(client_read_messages, (void *)iface) < 0) {
		anderrs("Failed to spawn thread");
		return -1;
	}

	if (ifconfig_get(iface, &ina.s_addr, &mask) < 0) {
		fprintf(stderr, "**Error: Failed to get IPv4 address of interface %s: %s", iface, strerror(errno));
		return -1;
	}

	while (fgets(txt.msg, sizeof(txt.msg), stdin) != NULL) {

		/* Remove newline */
		i=0;
		while (i < sizeof(txt.msg)) {
			if (txt.msg[i] == '\n') {
				txt.msg[i] = '\0';
				break;
			}

			i++;
		}

		if (strlen(txt.msg) == 0)
			continue;


		/* Command */
		if (txt.msg[0] == '.') {

			/* Help */
			if (strcmp(txt.msg, ".help") == 0) {
				printf("[+] .quit - Quit prompt\n");
				printf("[+] .stat - Print statistics\n");
				printf("[+] .ver - Print %s\n", IBSSCHAT_VERSION);
				continue;
			}

			/* Quit */
			if (strcmp(txt.msg, ".quit") == 0) {
				printf("[+] Bye, bye!\n");
				return 0;
			}

			/* Status */
			if (strcmp(txt.msg, ".stat") == 0) {
				uint32_t n = 0;
			
				thread_memlock_lock(statlock);
				printf("[+] Received %u messages from %u different IPs\n", 
					msgcount, ips);

				if (ips != 0) {
					struct in_addr sad;

					printf("[+]");
					for (n=0; n<ips; n++) {
						sad.s_addr = iplist[n];
						printf(" %s ", inet_ntoa(sad));
					}
					printf("\n");
				}
				thread_memlock_unlock(statlock);
				continue;
			}

			/* Print version */
			if (strcmp(txt.msg, ".ver") == 0) {
				printf("[+] %s\n", IBSSCHAT_VERSION);
				continue;
			}

			printf("[+] No such command, type '.help' for help\n");
			continue;
		}

		/* Send message */
		if (chat_send(iface, txt.msg) < 0)
			return -1;

		/* Display message Read */
		//printf("%s\n", txt.msg);
#if 0
		prompt:
			printf("%s > ", inet_ntoa(ina));
			fflush(stdout);
#endif
	}

	return 0;
}

/*
 * Send random chat message for testing
 * Returns the number of messages send
 */
int
chat_send_rand(const char *iface, int count, int delay)
{
	struct chatxt txt;
	unsigned int seed;
	unsigned int len;
	unsigned int i;
	int c;

	#define CHARS \
		"ABCDEFGHIKLMNOPQRSTUVVXYZ0123456" \
		"789abcdefghijklmnopqrstuvwxyz"

	getrand_nonblock((uint8_t *)&seed, sizeof(seed));
	srand(seed);

	for (c=0; c < count; c++) {

		/* Create the random message */
		memset(&txt, 0x00, sizeof(txt));
		if (count > 0)
			snprintf(txt.msg, sizeof(txt.msg), "%d_", c+1);

		len = strlen(txt.msg) + (rand() % 39) + 1;
		for (i=strlen(txt.msg); i < len; i++)
			txt.msg[i] = CHARS[rand() % 60];

		printf("[+] Sending (%d out of %d) %s\n", c+1, count, txt.msg);
		chat_send(iface, txt.msg);
		if (delay)
			sleep(delay);
	}

	return count;
}


/*
 * Send chat message
 * Returns 0 on success, -1 on error.
 */
int
chat_send(const char *iface, const char *str)
{
	struct chatxt txt;
	int ret;
	int sock;
	uint32_t ip;
	uint32_t mask;

	if (str == NULL || strlen(str) == 0) {
		fprintf(stderr, "** Error: Invalid chat message!\n");
		return -1;
	}

	if (wiface_adhoc(iface) != 1) {
		andlog("** Error: Interface is not up and running in ad-hoc mode\n", iface);
		fprintf(stderr, "** Error: Interface %s has not been configured\n",
			iface);
		return -1;
	}

	/* Make sure message size is OK */
	if (strlen(str) > (sizeof(txt.msg)-1)) {
		fprintf(stderr, "** Error: Message exceeds maximum length!\n");
		return -1;
	}

	/* Copy message to buffer */
	snprintf(txt.msg, sizeof(txt.msg), "%s", str);

	if (ifconfig_get(iface, &ip, &mask) < 0) {
		fprintf(stderr, "**Error: Failed to get IPv4 address of interface %s: %s", iface, strerror(errno));
		return -1;
	}

	/* Connect to daemon */
	if ( (sock = tcp_connect(ip, htons(CHAT_SEND_PORT), 0, 0)) < 0) {
		fprintf(stderr, "Failed to connect to daemon (is it running?): %s\n",
			strerror(errno));
		return -1;
	}

	/* Write message */
	if (writen(sock, &txt, sizeof(txt)) != sizeof(txt)) {
		fprintf(stderr, "** Error: Failed to write message to socket: %s\n", 
			strerror(errno));
		close(sock);
		return -1;
	}

	/* Read response */
	if (readn(sock, &ret, sizeof(ret)) != sizeof(ret)) {
		fprintf(stderr, "** Error: Failed to read return value: %s\n",
			strerror(errno));
		close(sock);
		return -1;
	}

	if (ret != 0) {
		fprintf(stderr, "** Error: No ACK received! Failed to send message '%s'\n", str);
	}

	close(sock);
	return 0;
}


