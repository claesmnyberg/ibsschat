/*
 *    File: net.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * This file contain network related routines
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "ibsschat.h"

#ifndef __ANDROID__
#define andlog printf
#endif

/*
 * Generates header checksum.
 * From W. Richard Stevens TCP/IP illustrated
 */
uint16_t
chksum(uint16_t *addr, int len)
{
    const uint16_t *w;
    uint16_t answer;
    uint32_t sum;
    int nleft;

	w = addr;
	sum = 0;
	nleft = len;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while (nleft > 1)  {
        sum += *w++;
        nleft -= 2;
    }
    if (nleft == 1)
        sum += htons(*(u_char *)w<<8);

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);         /* add carry */
    answer = ~sum;              /* truncate to 16 bits */
    return (answer);
}


/*
 * Create a listening UNIX socket.
 * Returns a socket descriptor on success, -1 on error.
 */
int
unix_socket_listen(const char *dir, const char *file)
{
    struct sockaddr_un un;
    int sd;

    sd = -1;

    /* Create the socket */
    andlog("Creating UNIX socket\n");
    if ( (sd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
        anderrs("Failed to create socket");
        goto err;
    }

    /* Set close on exec for this file descriptor */
    if (fcntl(sd, F_SETFD, FD_CLOEXEC)) {
        anderrs("Failed to set close on exec on socket");
        goto err;
    }

    /* Set up the socket path */
    memset(&un, 0x00, sizeof(struct sockaddr_un));
    un.sun_family = AF_LOCAL;
    snprintf(un.sun_path, sizeof(un.sun_path),
        "%s/%s", dir, file);


    /* Remove any old socket */
    unlink(un.sun_path);
    unlink(dir);

    /* Create the directory, assumes that the 
     * directory structure already exist */
    mkdir(dir, 0755);

    /* Bind path to socket */
    andlog("Bind socket to %s\n", un.sun_path);
    if (bind(sd, (struct sockaddr *)&un, sizeof(un)) < 0) {
        anderrs("Failed to bind socket");
        goto err;
    }

    /* Set permissions */
    chmod(dir, 0755);
    chmod(un.sun_path, 0666);

    /* Start listening on the socket with a queue of 5 clients */
    if (listen(sd, 5) < 0) {
        anderrs("Failed to listen on socket");
        goto err;
    }

    return sd;
    err:
        if (sd >= 0)
            close(sd);

        return -1;
}


/*
 * Connect to UNIX socket.
 * Returns file descriptor on success, -1 on error.
 */
int
unix_socket_connect(const char *dir, const char *sockname)
{
    struct sockaddr_un un;
    int sfd = -1;

    /* Create socket */
    if ( (sfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "** Error: Failed to create socket: %s\n",
            strerror(errno));
        goto err;
    }

    /* Set up address, path to the socket */
    memset(&un, 0x00, sizeof(un));
    un.sun_family = AF_LOCAL;
    snprintf(un.sun_path, sizeof(un.sun_path),
        "%s/%s", dir, sockname);

    /* Set close on exec */ 
    if (fcntl(sfd, F_SETFD, FD_CLOEXEC)) {
        fprintf(stderr, "** Error: Failed to set close on exec on fd: %s\n",
            strerror(errno));
    }

    /* Connect */
    if (connect(sfd, (struct sockaddr *)&un, sizeof(un)) != 0) {
        fprintf(stderr, "** Error: Failed to connect to %s: %s\n",
            un.sun_path, strerror(errno));
        fprintf(stderr, "** Error: is daemon started? (ibsschat --daemon)\n");
        goto err;
    }

    return sfd;
    err:
        if (sfd >= 0)
            close(sfd);
 
        return -1;
}

/*
 * Create a TCP listening socket.
 * IP address and Port in network byte order.
 * Returns a socket descriptor on success, 
 * -1 on error.
 */
int
tcp_listen(uint32_t ipv4, uint16_t port)
{
    struct sockaddr_in sin;
    int sock; 
    int yes = 1;

    memset(&sin, 0x00, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = port;
    sin.sin_addr.s_addr = ipv4;

    if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        anderrs("Failed to create socket");
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
            (void *)&yes, sizeof(yes)) < 0) {
        anderrs("Failed to set SO_REUSEADDR");
    }

    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
            (void *)&yes, sizeof(yes)) < 0) {
        anderrs("Failed to set SO_REUSEADDR");
    }

    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        anderrs("bind() failed");
		andlog("Could not bind to %s:%u\n", 
			inet_ntoa(sin.sin_addr), ntohs(port));
        close(sock);
        return -1;
    }

    if (listen(sock, 5)) {
        anderrs("Failed to listen on socket!");
        return -1;
    }

    return sock;
}




/*
 * Connect to ip:port.
 * Ip and port is assumed to be in network byte order.
 * Returns socket descriptor on success, -1 on error.
 */
int
tcp_connect(uint32_t ip, uint16_t port, uint32_t srcip, uint16_t srcport)
{
	struct sockaddr_in taddr;	
	struct sockaddr_in laddr;	
	unsigned int sfd;

	memset(&taddr, 0x00, sizeof(taddr));
	taddr.sin_family = AF_INET;
	taddr.sin_addr.s_addr = ip;
	taddr.sin_port = port;
	
	/* Create socket */
	if ( (sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		anderrs("Failed to create socket");
		return(-1);
	}

	/* Bind address to socket */
	memset(&laddr, 0x00, sizeof(laddr));
    laddr.sin_family = AF_INET;
    laddr.sin_addr.s_addr = srcip;
    laddr.sin_port = srcport;

	if (srcip || srcport) {
		int yes = 1;

		/* Reuse address */
		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes)) < 0) {
			anderrs("SO_REUSEADDR failed");
			close(sfd);
			return(-1);
		}

		/* Keep Alive */
		if (setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&yes, sizeof(yes)) < 0) {
			anderrs("SO_KEEPALIVE failed");
			close(sfd);
			return(-1);
		}

		/* Bind socket to address */
		if (bind(sfd, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
			anderrs("bind() failed");
			close(sfd);
			return(-1);
		}
	}
	
	/* Connect to target */
	if (connect(sfd, (struct sockaddr *)&taddr, sizeof(taddr)) < 0) {
		anderrs("Failed to connect");
		close(sfd);
		return(-1);
	}

	return(sfd);
}



/*
 * Check for data to read from file descriptor.
 * Returns 1 if there is data to read, 0 otherwise
 * and -1 on error.
 */
int
data_to_read(int fd)
{
    fd_set readset;
    struct timeval tv;

    memset(&tv, 0x00, sizeof(tv));
    tv.tv_usec = 100;

    for (;;) {
        FD_ZERO(&readset);
        FD_SET(fd, &readset);
        
        if (select(fd+1, &readset, NULL, NULL, &tv) < 0) {
            if (errno == EINTR)
                continue;

			  anderrs("select() failed");
              return(-1);
        }

        break;
    }

      if (FD_ISSET(fd, &readset))
        return(1);

    return(0);
}


/*
 * Convert IP into string.
 */
const char *
ipstr(uint32_t ip)
{
	static char buf[20];
	struct in_addr ipa;

	ipa.s_addr = ip;
	snprintf((char *)buf, sizeof(buf), "%s", inet_ntoa(ipa));
	return buf;
}



