/*
 *    File: chat_proc.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * Chat system daemon
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "ibsschat.h"

/* Local routines */
static void *handle_client_sending(void *);
static void *chat_client_accept_sending(void *);
static void *chat_client_accept_receive(void *);

/* Local variables */
static struct in_addr ina;
static struct in_addr inm;



/*
 * Thread entry point.
 * Handle connected client.
 */
static void *
handle_client_sending(void *sock)
{
	struct chatmsg cm;
	int cfd = *((int *)sock);
	int ret = 0;

	/* Read the message from the unix socket */
	if (readn(cfd, &cm.txt, 
			sizeof(struct chatxt)) != sizeof(struct chatxt)) {
		anderrs("Failed to read chat text from socket");
		goto finished;
	}

	andlog("[+] handle_client_sending: Sending \"%s\"\n", 
		cm.txt.msg);

	/* Set up message */
	cm.type = CHAT_MSG;
	msgbuf_setid((struct message *)&cm);

	/* Send it as a broadcast */
	if (mcast_send((struct message *)&cm, 1) < 0) {
		andlog("** Error: Failed to broadcast chat message\n");
		ret = -1;
	}

	/* Write status to client */
	if (writen(cfd, &ret, sizeof(ret)) != sizeof(ret)) {
		fprintf(stderr, "** Error: Failed to write return value to socket: %s\n",
			strerror(errno));
	}

	finished:
		close(cfd);
		free(sock);
		return NULL;
}


/*
 * Accept local clients that want to send a message.
 */
static void *
chat_client_accept_sending(void *arg)
{
	struct sockaddr_in sin;
	socklen_t addrlen;
	int cfd = -1;
	int sd = -1;

	addrlen = sizeof(struct sockaddr_in);

    /* Create the listening socket */
	listen_tcp:

	for (;;) {
		if ( (sd = tcp_listen(ina.s_addr, htons(CHAT_SEND_PORT))) < 0) {
			andlog("Failed to create listening TCP chat socket on %s:%u"
			", retrying in 5 seconds\n", inet_ntoa(ina), CHAT_SEND_PORT);
			sleep(5);
		}
		else
			break;
	}

    andlog("[+] chat process accepting sending clients on %s:%u\n",
		inet_ntoa(ina), CHAT_SEND_PORT);
    while ( (cfd = accept(sd, (struct sockaddr *)&sin, &addrlen)) > 0) {
		int *spt;

		spt = calloc(1, sizeof(int));
		*spt = cfd;

		andlog("[+] client %s:%u connected to send socket \n",
			inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

        /* Spawn a thread to handle the client */
        andlog("chat sending client connected\n");
		thread_spawn(handle_client_sending, (void *)spt);

	    andlog("[+] chat process accepting sending clients on %s:%u\n",
			inet_ntoa(ina), CHAT_SEND_PORT);
    }

	/* Restart listening thread in case someone
     * re-configured the interface */
	andlog("[+] ** Error: Failed to accept chat clients, restarting");

	if (sd >= 0)
		close(sd);
	goto listen_tcp;


	/* Unreached */
	return NULL;
}

struct recvarg {
	int sock;
	uint32_t ip;
};

static void *
handle_client_receive(void *arg)
{
	struct recvarg *a = (struct recvarg *)arg;
	int enc = 0;

	/* Encrypt if this is not a local client 
     * since it is transfered on the network */
	if (a->ip != ina.s_addr)
		enc = 1;
	
	/* Dump the buffered messages */
	msgbuf_dump(a->sock, enc);

	/* Disconnect remote client after synchronize */
	if (a->ip != ina.s_addr) {
		andlog("Disconnecting remote client (%08x) after synchronization\n", a->ip);
		close(a->sock);
		free(arg);
		return NULL;
	}

	/* Register the socket for future messages */
	if (msgbuf_addsock(a->sock) < 0)
		close(a->sock);

	free(arg);
	return NULL;
}


/*
 * Accept clients that want to read messages.
 * The first thing we do is to dump the messages
 * in the buffer and then register the socket in
 * the chat buffer.
 */
static void *
chat_client_accept_receive(void *arg)
{
	struct sockaddr_in sin;
	socklen_t addrlen;
    int cfd = -1;
    int sd = -1;


	addrlen = sizeof(struct sockaddr_in);

    /* Create the listening socket */
    listen_tcp:

    for (;;) {
        if ( (sd = tcp_listen(ina.s_addr, htons(CHAT_RECV_PORT))) < 0) {
            andlog("Failed to create listening TCP chat socket on %s:%u"
            ", retrying in 5 seconds\n", inet_ntoa(ina), CHAT_SEND_PORT);
            sleep(5);
        }
        else
            break;
    }

    andlog("[+] chat process accepting reading clients on %s:%u\n",
        inet_ntoa(ina), CHAT_RECV_PORT);

    while ( (cfd = accept(sd, (struct sockaddr *)&sin, &addrlen)) > 0) {
		struct recvarg *arg;

		andlog("[+] client %s:%u connected to read socket \n",
			inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

		arg = calloc(1, sizeof(struct recvarg));
		arg->sock = cfd;
		arg->ip = sin.sin_addr.s_addr;
		thread_spawn(handle_client_receive, (void *)arg);

		/* Continue and accept the next client */
    	andlog("[+] chat process accepting reading clients on %s:%u\n",
        	inet_ntoa(ina), CHAT_RECV_PORT);
    }

    /* Restart listening thread in case someone
     * re-configured the interface */
    andlog("[+] ** Error: Failed to accept chat clients, restarting");

    if (sd >= 0)
        close(sd);
    goto listen_tcp;


    /* Unreached */
    return NULL;
}


/*
 * Start of the chat process.
 * Returns -1 on error.
 */
extern void
chat_proc_run(char *iface)
{
	char tmp[256];

    /* Make sure we got root! */
    if (getuid() != 0) {
        fprintf(stderr, "** Error: root privileges required!\n");
		exit(EXIT_FAILURE);
    }

	andlog("[+] Chat process %u up and running\n", getpid());

	/* Add route for multicast */
	andlog("Adding route 224.0.0.0/4 to %s\n", iface);
	snprintf(tmp, sizeof(tmp), "ip route add 224.0.0.0/4 dev %s", 
		(char *)iface);
	system(tmp);

	/* Get interface address */
	if (ifconfig_get(iface, &ina.s_addr, &inm.s_addr) != 0) {
		anderrs("Failed to get IPv4 address and netmask from interface");
		exit(EXIT_FAILURE);
	}

	andlog("%s has IPv4 address %s\n", iface, inet_ntoa(ina));
	andlog("%s has network mask %s\n", iface, inet_ntoa(inm));

	/* Init the message buffer */
	msgbuf_init(ina.s_addr);

	/* Start the multicast thread */
	chat_mcast_reader_start(ina.s_addr, inm.s_addr);

	/* Start thread for clients receiving messages */
	thread_spawn(chat_client_accept_receive, NULL);

	/* Run the chat send message service */
	chat_client_accept_sending(NULL);

	/* Unreached */
	exit(EXIT_FAILURE);
}

