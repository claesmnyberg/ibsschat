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
static int write_msg(int, uint8_t, void *, size_t);
static int read_msg(int);
//static int gotroot(int, union msgbuf *);
static int parse_wifconf(union msgbuf *, int, char **);
static void mkbssid(uint8_t *, char *);


/*
 * Write message to socket.
 * Return 0 on success, -1 on failure.
 */
static int
write_msg(int sfd, uint8_t code, void *buf, size_t len)
{
    if (writen(sfd, &code, 1) != 1) {
        fprintf(stderr, "Failed to write message code: %s\n", 
			strerror(errno));
        return -1;
    }
    if (writen(sfd, buf, len) != len) {
        fprintf(stderr, "Failed to write message: %s\n", 
			strerror(errno));
        return -1;
    }

    return 0;
}


/*
 * Print information for wireless card
 */
static void
print_wiface(struct wiface *w)
{
	struct in_addr ina;
	struct in_addr inm;
	
	memset(&ina, 0x00, sizeof(ina));
	memset(&inm, 0x00, sizeof(inm));
	ina.s_addr = w->ipv4;
	inm.s_addr = w->mask;
	printf("\n");
	printf("  Iface: %s (%s)\n", w->iface, wimode(w->mode));
	printf("   IPv4: %s\n", inet_ntoa(ina));
	printf("   Mask: %s\n", inet_ntoa(inm));
	printf("  ESSID: %s\n", w->essid);
	printf("Channel: %d\n", w->channel);
	printf("  BSSID: %s\n", net_macstr(w->mac_bssid));
	printf("    Key: '%s'\n", (char *)w->key);
	printf("\n");
}

/*
 * Read server response and write output to terminal.
 * Return 0 on success and -1 on error.
 */
static int
read_msg(int sfd)
{
	uint8_t code;
	union msgbuf buf;
	size_t len;
	ssize_t n;

    /* Read code */
    if (read(sfd, &code, sizeof(code)) < 0) {
        fprintf(stderr, "** Error: Failed to read message code: %s\n",
			strerror(errno));
        return -1;
    }

	switch (code) {
		case MSG_CODE_OK:
			return 0;
			break;

		case MSG_CODE_ERR:
			len = sizeof(buf.err);
			break;

		case MSG_WI_STATUS:
			len = sizeof(buf.wi_status);
			break;

		default:
			fprintf(stderr, "** Error: Received invalid code [%d]\n", code);
			return -1;
	}

	/* Read data */
	if ( (n = readn(sfd, &buf, len)) != len) {
		fprintf(stderr, "** Error: Failed to read, returned %d out of %u bytes: %s\n", (int)n, (unsigned int)len, strerror(errno));
		return -1;
	}
	

	switch (code) {
		case MSG_CODE_ERR:
			fprintf(stderr, "** Error: %s\n", buf.err.str);
			break;

		case MSG_WI_STATUS:
			print_wiface(&buf.wi_status.wi);
			break;

		/* Should never happen since we checked this above */
		default:
			fprintf(stderr, " ** Error: Received invalid code [%d]\n", code);
			return -1;
	}

	return 0;
}


#if 0
/*
 * We got root privileges, go ahead
 * and run this.
 */
static int
gotroot(int argc, union msgbuf *buf)
{
	int ifd;
	int ret = 0;

	/* Open IOCTL socket */
	if ( (ifd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "** Error: Failed to open IOCTL socket");
		return -1;
	}

	/* Get configuration */
	if (argc == 2) {
		if (wiface_status(ifd, &buf->wi_status.wi) < 0) {
			ret = -1;
		}

		print_wiface(&buf->wi_status.wi);
	}

	/* Set configuration */
	else if (argc == 6) {

#ifdef __ANDROID__
		/* Disable WiFi */
		system("svc wifi disable");
#endif

		if (wiface_conf(ifd, &buf->wi_status.wi) < 0) {
			ret = -1;
		}
	}

	close(ifd);
	return ret;
}
#endif 


/*
 * Create a BSSID based on the string by XOR
 * BSSID must be 6 bytes.
 */
static void
mkbssid(uint8_t *bssid, char *str)
{
	size_t len;
	size_t i;

	/* Set default value */
	memcpy(bssid, "\x8a\x64\x2d\x92\x93\x69", 6);

	len = strlen(str);
	for (i=0; i < len; i++) {
		bssid[i%6] = (uint8_t)bssid[i%6] ^ (uint8_t)str[i];
	}

	/* Use a fixed value for first byte */
	bssid[0] = 0x8a;
}


/*
 * Parse command line and fill in wireless
 * configuration structure.
 * Returns 0 on success, -1 on error.
 */
static int
parse_wifconf(union msgbuf *buf, int argc, char **argv)
{
	memset(buf, 0x00, sizeof(union msgbuf));

	/* Interface */	
	if (argc >= 2) {
		snprintf(buf->wi_status.wi.iface,
			sizeof(buf->wi_status.wi.iface), "%s", argv[1]);
	}

	if (argc == 2)
		return 0;

	/* Should never happen since we check
	 * this in main */
	if (argc != 7) {
		anderr("** Error: Invalid number of configuration arguments!\n");
		return -1;
	}

	/* Set mode */
	buf->wi_status.wi.mode = IW_MODE_ADHOC;

	/* IPv4 */
	buf->wi_status.wi.ipv4 = inet_addr(argv[2]);

	/* Netmask */
	buf->wi_status.wi.mask = inet_addr(argv[3]);

	/* Network name, ESSID */
	snprintf(buf->wi_status.wi.essid, IW_ESSID_MAX_SIZE, "%s", argv[4]);

	/* Channel */
	buf->wi_status.wi.channel = atoi(argv[5]);
	if (buf->wi_status.wi.channel < 1 || buf->wi_status.wi.channel > 14) {
		fprintf(stderr, "** Error: Invalid channel \n");
		return -1;
	}

	/* Encryption key */
	snprintf((char *)buf->wi_status.wi.key, 
		sizeof(buf->wi_status.wi.key), "%s", argv[6]);

	/* Since some devices seem to make up their own AP
	 * in ad-hoc mode and not find each other, we
	 * construct a BSSID based on the network name */
	mkbssid(buf->wi_status.wi.mac_bssid, buf->wi_status.wi.essid);	

	return 0;
}


/*
 * IBSS configuration client.
 * Returns 0 on success, -1 on error.
 */
int
conf_client(int argc, char **argv)
{
	union msgbuf buf;
	int sfd;

	if (parse_wifconf(&buf, argc, argv) != 0)
		return -1;

	/*
     * Even though we already have root, we should
     * connect to the server for the configuration
     * since the service will restart the chat process
     * for us.
     */

#if 0
	/* We already got r00t, no need to connect */
	if (getuid() == 0) {
		return gotroot(argc, &buf);
	}
#endif
	
	if ( (sfd = tcp_connect(inet_addr("127.0.0.1"), 
			htons(CONF_PORT), 0, 0)) < 0) {
		fprintf(stderr, "** Error: Wireless ad-hoc configuration service not running\n");
		return -1;
	}

	/* Display status for interface */
	if (argc == 2) {
		if (write_msg(sfd, MSG_REQ_STATUS, 
				&buf, sizeof(buf.wi_status)) < 0) {
			close(sfd);
			return -1;
		}
	}	

	/* Send configure request */
	if (argc == 7) {
		if (write_msg(sfd, MSG_WI_REQ_CONF,
				&buf, sizeof(buf.wi_status)) < 0) {
			close(sfd);
			return -1;
		}
	}

	read_msg(sfd);
	close(sfd);
	return 0;
}

