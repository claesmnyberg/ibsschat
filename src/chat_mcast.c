/*
 *    File: chat_mcast.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * IBSS chat multicast messages
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "ibsschat.h"

/* Local routines */
static void *mcast_read(void *);
static void *mcast_sync_thread(void *);

/* The chat clients */
struct clients {

	/* The list of discovered IPv4 addresses */
	uint32_t *iplist;
	uint32_t num_ips;

	/* Our IPv4 as an integer (in host byte order) */
	uint32_t myip;
};

/* Our IPv4 address */
static uint32_t myipv4;
static uint32_t mymask;

/* Private variables */
static lock_t statlock;
static struct clients r;

/* Initial discovery message */
static struct discover d;
static struct discover dc; /* Encrypted discovery */


/*
 * Reset the list of client IPv4 addresses.
 */
void
iplist_reset(void)
{
    thread_memlock_lock(statlock);
    free(r.iplist);
	r.iplist = NULL;
	r.num_ips = 0;
    thread_memlock_unlock(statlock);
}




/*
 * Send multicast message
 */
int
mcast_send(struct message *m, int wantack)
{
	struct sockaddr_in addr;
	struct message mc;
	socklen_t addrlen;
	int sock;
	int ret = 0;
	int retry = 0;

	if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		anderrs("mcast_send(): Failed to create socket");
		return -1;
	}		

	/* Use source address of the interface supplied on the
	 * command line */
	memset(&addr, 0x00, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(r.myip);
	addr.sin_port = 0;
	addrlen = sizeof(addr);
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		anderrs("Failed to bind ip on multicast socket");
	}

	memset(&addr, 0x00, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(CHAT_GROUP);
	addr.sin_port = htons(CHAT_GROUP_PORT);
	addrlen = sizeof(addr);

	/* Copy and encrypt message */
	memcpy(&mc, m, sizeof(struct message));
	chat_crypto_encrypt(&mc);

#ifdef DONT_WAIT_FOR_ACK
	/* For testing */
	wantack = 0;
#endif

	/* Resend message if necessary */
	ret = -1;
	retry = 1;
	while (retry < (MSG_RESEND_TIMES+1)) {
		useconds_t usec;

		andlog("mcast_send(): Sending message type %d\n", mc.type);
		if (sendto(sock, &mc, sizeof(struct message), 0, 
				(struct sockaddr *)&addr, addrlen) < 0) {
			anderrs("Failed to send multicast message");
			break;
		}

		/* No need to wait for an ACK */
		if (wantack == 0) {
			ret = 0;
			break;
		}

		/* Wait for response and increase time 
         * based on the number of re-sends */
		usec = retry * 100000; /* 100 milliseconds */
		if (retry > 3)
			usec  *= 2;

		//andlog("[++] Sleeping for  %lu micro seconds\n", usec);
		usleep(usec);

		/* A response have been seen */
		if (msgbuf_exist(m) > 1) {
			ret = 0;
			break;
		}

		retry++;
	}

	if (ret < 0) {
		andlog("Failed to send message, no acknowledge seen\n");	
		msgbuf_delete(m);
	}

	close(sock);
	return ret;
}



/*
 * Thread entry point.
 * Read ring discovery broadcasts and update status.
 * This function never return.
 */
static void *
mcast_read(void *arg)
{
	struct sockaddr_in addr;
	struct sockaddr_in sendaddr;
	struct sockaddr_in toaddr;
	struct ip_mreq mreq;
	socklen_t optlen;
	int sendbuff;
	int recvbuff;
	struct message m;
	struct message mc;
	int addrlen;
	int sock;
	int s_sock; /* Socket for sending */
	int n;

	andlog("Multicast Read Thread running\n");
	sock = 0;
	s_sock = 0;

	/* Set up address we bind to for receiving messages */
	memset(&addr, 0x00, sizeof(addr));
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//addr.sin_addr.s_addr = htonl(r.myip);
	addr.sin_port = htons(CHAT_GROUP_PORT);
	addrlen = sizeof(addr);
	int yes = 1;

	/* Ignore SIG pipe when a write() fails to a client 
     * since this is handled in the code */
	signal(SIGPIPE, SIG_IGN);

	/* Open multicast socket */
	if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		anderrs("Chat Read Thread Failed to open socket");
		return NULL;
	}

	optlen = sizeof(sendbuff);
	if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen) < 0) {
		anderrs("getsockopt() failed");
	}
	else 
		andlog("Multicast socket send buffer: %u bytes\n", sendbuff);


	optlen = sizeof(recvbuff);
	if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvbuff, &optlen) < 0) {
		anderrs("getsockopt() failed");
	}
	else 
		andlog("Multicast socket receive buffer: %u bytes\n", recvbuff);

	/* Reuse port to allow multiple listeners on the same machine */
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) <0) {
		anderrs("setsockopt failed");
	}

	/* Bind address */
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		anderrs("Failed to bind socket");
		goto finished;
	}

	/* Join group */
	mreq.imr_multiaddr.s_addr = inet_addr(CHAT_GROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			&mreq, sizeof(mreq)) < 0) {
		anderrs("Failed to join multicast group");
		goto finished;
	}	


	/* Create the socket for sending */
	if ( (s_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		anderrs("** Error: Failed to create sending socket");
		return NULL;
	}

	/* Set up address we bind to for sending */
	memset(&sendaddr, 0x00, sizeof(sendaddr));
	sendaddr.sin_family = AF_INET;
	sendaddr.sin_addr.s_addr = htonl(r.myip);
	sendaddr.sin_port = 0;
	addrlen = sizeof(sendaddr);
	if (bind(s_sock, (struct sockaddr *)&sendaddr, sizeof(sendaddr)) < 0) {
		anderrs("Failed to bind ip on multicast sending socket");
	}

	/* Set up destination address for sending with sendto() */
	memset(&toaddr, 0x00, sizeof(addr));
	toaddr.sin_family = AF_INET;
	toaddr.sin_addr.s_addr = inet_addr(CHAT_GROUP);
	toaddr.sin_port = htons(CHAT_GROUP_PORT);
	addrlen = sizeof(addr);


	/* Receive multicast messages */
	andlog("Listening for chat messages\n");
	while ( (n = recvfrom(sock, &m, sizeof(struct message), 
			0, (struct sockaddr *)&addr, (socklen_t *)&addrlen)) > 0) {
		struct in_addr sad;
		int seen = 0;
		int fromself = 0;
		int send_discover = 0;
		int fwd = 0;

		/* Ignore short message */
		if (n != sizeof(struct message)) {
			andlog("Ignored multicast message of different size (%u bytes)\n", n);
			continue;
		}

		/* Copy encrypted message for quick forwarding */
		memcpy(&mc, &m, sizeof(struct message));

		/* Decrypt message */
		if (chat_crypto_decrypt(&m) < 0) {
			anderr("** Error: Failed to decrypt message\n");
			continue;
		}

		/* Make sure type is valid */
		if (msgtype_valid(&m) == 0) {
			andlog("** Error: Received message of unknown type: %u\n",
				 m.type);
			continue;
		}

		/* Shut up compiler */
		fromself++; fromself = 0;

		/* Save original source ip from within the message */
		sad.s_addr = m.id.ip;

		/* Flag message sent by us */
		if (ntohl(addr.sin_addr.s_addr) == r.myip) 
			fromself = 1;

		/* Ignore messages sent by us the second time to
		 * keep track of acknowledgements from other clients */
		if (msgbuf_exist(&m) > 0) {
			if ( (fromself == 1) && (ntohl(m.id.ip) == r.myip)) 
				continue;
		}

		/* Add the message or get the number of times
         * it has been seen */
		seen = msgbuf_add((struct message *)&m);

		/* Always forward message the first time it is seen */
		if ((seen >= 1) && (seen <= 5)) {
			fwd = 1;
			andlog("Forwarding (first time seen) message %08x%08x%04x%04x\n",
				mc.id.ip, mc.id.sec, mc.id.usec, mc.id.sum);
		}

		/* Always forward messages from the original sender since it is 
         * a retransmission for a lost ACK */
		if (fwd == 0) {
			if ((seen > 1) && (addr.sin_addr.s_addr == m.id.ip)) {
				andlog("[++] Re-sending original message %08x%08x%04x%04x\n",
					mc.id.ip, mc.id.sec, mc.id.usec, mc.id.sum);
				fwd = 1;
			}
		}

		/*
         * Forward a message which have been seen before
         * with decreasing probability based on the number
		 * of times it has been seen.
         */
		if (fwd == 0) {
			if ((seen > 1) && (seen <= MSG_RESEND_TIMES)) {
				int p = (seen * 10);
				int r = rand() % 100;

				if (r <= p) {
					andlog("Forwarding (p=%d) message %08x%08x%04x%04x\n", 
						p, mc.id.ip, mc.id.sec, mc.id.usec, mc.id.sum);
					fwd = 1;
				}
			}
		}


		/* Only forward messages from other nodes */
		if (fromself == 1)
			fwd = 0;

		if (1) {
			char buf[1024];
			snprintf(buf, sizeof(buf), "%s", inet_ntoa(sad));
			andlog("Received message from %s forwarded by %s\n",
				buf, inet_ntoa(addr.sin_addr));
		}

		/* Forward message */
		if (fwd) {
			if (sendto(s_sock, &mc, sizeof(struct message), 0, 
					(struct sockaddr *)&toaddr, addrlen) < 0) {
				anderrs("Failed to forward multicast message");
			}	
		}

	   /* If this was a discovery message, respond with our
   	   	* initial discovery message so that new clients can
	 	* discover us */
		if ((m.type == CHAT_DISCOVER) && (seen == 1))
			send_discover = 1;


		/* Attempt to add client to the list of IPv4 addresses 
         * if it is a neighbour (we see the source IPv4) */
		if (ntohl(addr.sin_addr.s_addr) != r.myip) {
		
			thread_memlock_lock(statlock);
			if (iplist_add(addr.sin_addr.s_addr, &r.iplist, r.num_ips)) {
				andlog("Added new IPv4 %s address to neighbor list\n", inet_ntoa(addr.sin_addr));
				r.num_ips = r.num_ips + 1;
				//iplist_print(r.iplist, r.num_ips);
			}
			thread_memlock_unlock(statlock);
		}

		/* Send discover to new client */
		if (send_discover)  {
			andlog("Sending discovery message\n");
			if (sendto(s_sock, &dc, sizeof(struct message), 0, 
					(struct sockaddr *)&toaddr, addrlen) < 0) {
				anderrs("Failed to send multicast discover message");
			}
		}
	}


	finished:
		if (sock > 0)
			close(sock);
		if (s_sock > 0)
			close(s_sock);
		return NULL;
}


/*
 * Run the multicast reader thread.
 * Return 0 on success, -1 on error.
 */
int
chat_mcast_reader_start(uint32_t ipv4, uint32_t mask)
{
	unsigned int seed;

	/* Init global variables */
	memset(&r, 0x00, sizeof(struct clients));
	memset(&d, 0x00, sizeof(struct discover));

	/* Seed the random number generator */
	getrand_nonblock((uint8_t *)&seed, sizeof(seed));
	srand(seed);

	/* Initialize locks */
	thread_memlock_init(statlock);

	/* Set our ip as an integer in host byte order */
	r.myip = ntohl(ipv4);
	myipv4 = ipv4;
	mymask = mask;

	/* Spawn the multicast reader thread */
	if (thread_spawn(mcast_read, NULL) != 0)
		return -1;

	andlog("Message size is %u bytes\n", MSGSIZE);

	usleep(200000);
	d.type = CHAT_DISCOVER;
	msgbuf_setid((struct message *)&d);

	/* Encrypt discovery message for sending later
     * in response to clients */
	memcpy(&dc, &d, sizeof(struct message));	
	chat_crypto_encrypt((struct message *)&dc);

	/* Send initial discover message */
	if (mcast_send((struct message *)&d, 0) != 0)
		return -1;

	usleep(200000);

	/* Send a second one just for comfort ... */
	if (mcast_send((struct message *)&d, 0) != 0)
		return -1;

	/* Give clients some time to respond */
	sleep(1);

	/* Spawn the synchronization thread */
	thread_spawn(mcast_sync_thread, NULL);
	sleep(1);
	return 0;
}

void *
mcast_sync_thread(void *arg)
{
	uint32_t ip = 0;
	int i = 0;

	/* Attempt to synchronize from another client */
	andlog("[SYNC] Sync thread started\n");
	i = 0;

	/* Wait for at least one IP to be discovered */
	while (r.num_ips == 0) 
		sleep(1);

	do {
		thread_memlock_lock(statlock);
		if (i < r.num_ips) 
			ip = htonl(r.iplist[i]);
		else
			ip = 0;
		thread_memlock_unlock(statlock);

		/* Avoid our IP if that for some weird reason
		 * ended up in the list */
		if ((ip != myipv4) && (ip > 0)) {

			/* Attempt to connect to client and synchronize */
			if (msgbuf_sync(ip, ntohs(CHAT_RECV_PORT)) > 0) 
				return NULL;
		}

		i++;
	} while (ip > 0);

	andlog("[SYNC] ** Could not find an IP with at least one message\n");
	return NULL;
}

