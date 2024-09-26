/*
 *    File: conf_thread.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * This file implements the daemon that execute the
 * IBSS configuration received on the unix socket as root. 
 * It should be started as a service by the Android system upon boot.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "ibsschat.h"

/* Local routines */
static void *handle_client(void *);
static int write_msg(int, uint8_t, void *, size_t);
static void start_chat_process(char *);

/* Local variables */
static pid_t chatpid = 0;

struct arg {
	int cfd;
	char *iface;
};

/*
 * Write message to socket.
 * Return 0 on success, -1 on failure.
 */
static int
write_msg(int cfd, uint8_t code, void *buf, size_t len)
{
	if (writen(cfd, &code, 1) != 1) {
		anderrs("Failed to write conf message code");
		return -1;
	}
	andlog("write_msg() wrote message code (1 bytes)\n");

	if (len == 0) {
		andlog("write_msg: Data length is zero bytes, exiting\n");
		return 0;
	}

	if (writen(cfd, buf, len) != len) {
		anderrs("Failed to write message");
		return -1;
	}
	andlog("write_msg() wrote message  (%u bytes)\n", len);

	return 0;
}


/*
 * Handle connected client.
 * This function is called from a forked
 * process and will exit instead of return.
 */
static void *
handle_client(void *arg)
{
	struct arg *a;
	uint8_t code;
	size_t len;
	int ifd;
	union msgbuf buf;
	int cfd;
	char *iface;

	a = (struct arg *)arg;
	cfd = a->cfd;
	iface = a->iface;

	/* Open IOCTL socket */
	if ( (ifd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		anderrs("Failed to open conf IOCTL socket");
		return NULL;
	}

	/*
	 * Our protocol is simple, the client 
	 * send a single byte with the request
	 * code followed by the structure for 
	 * that code, and the server respond
	 * in the same way.
	 * One request and one reply per connection.
	 */

	/* Read code */
	if (read(cfd, &code, sizeof(code)) < 0) {
		anderrs("Failed to read conf message code");
		goto err;
	}	
	
	memset(&buf, 0x00, sizeof(buf));

	switch (code) {
		case MSG_REQ_STATUS: 
			andlog("Read conf MSG_REQ_STATUS [%d]\n", getpid());
			len = sizeof(struct msg_req_status); 
			break;

		case MSG_WI_REQ_CONF:
			andlog("Read conf MSG_WI_REQ_CONF [%d]\n", getpid());
			len = sizeof(struct msg_req_conf);
			break;		

		default:
			snprintf(buf.err.str, sizeof(buf.err.str), 
				"unrecognized conf message code: %d", code);
			anderrs(buf.err.str);
			goto err;
	}

	/*
	 * Read the buffer sent from client
	 */
	if (readn(cfd, &buf, len) != len) {
		snprintf(buf.err.str, sizeof(buf.err.str), 
			"** Error: Failed to read conf message [code=%d]", code);
		anderrs(buf.err.str);
		goto err;
	}


	switch (code) {

		/* Handle wireless interface status request */
		case MSG_REQ_STATUS: 
			if (wiface_status(ifd, &buf.wi_status.wi) < 0) {
				snprintf(buf.err.str, sizeof(buf.err.str), 
					"Failed to retrieve interface status");
				anderrs(buf.err.str);
				goto err;
			}
			len = sizeof(buf.wi_status);
			code = MSG_WI_STATUS;
			break;

		/* Handle wireless interface configuration request */
		case MSG_WI_REQ_CONF:
#ifdef __ANDROID__
			/* Disable WiFi 
             * TODO: Disable Wi-Fi programatically instead to avoid
             * calling external programs */
			system("svc wifi disable");
#endif
			if (wiface_conf(ifd, &buf.wi_conf.wi) < 0) {
				snprintf(buf.err.str, sizeof(buf.err.str), 
					"Failed to configure interface");
				anderrs(buf.err.str);
				goto err;
			}
			len = 0;
			code = MSG_CODE_OK;

			/* Restart chat process */
			start_chat_process(iface);

			break;

		/* Should never happen since we check this above */
		default:
			snprintf(buf.err.str, sizeof(buf.err.str), 
				"unrecognized conf message code: %d", code);
			anderrs(buf.err.str);
			goto err;

	}

	/* Send reply to client */
	andlog("Writing %u bytes to client\n", len);
	write_msg(cfd, code, &buf, len);

	if (ifd >= 0)
		close(ifd);

	sleep(1);
	andlog("conf client finished [%d]\n", getpid());

	close(cfd);
	free(arg);
	return NULL;

	err:
		write_msg(cfd, MSG_CODE_ERR, 
			&buf.err, sizeof(buf.err));
		close(cfd);

		if (ifd >= 0)
			close(ifd);

		free(arg);
		return NULL;
}

/*
 * Signal handler.
 */
static void
sighandler(int signo)
{
	int status;

    andlog("[*] (%u) Received signal %u\n", 
		getpid(), signo);

    if (chatpid != 0) {
    	andlog("[*] (%u) Killing chat process (%u)\n", 
			getpid(), chatpid);
		kill(chatpid, SIGKILL);
		waitpid(chatpid, &status, 0);		
		chatpid = 0;
    }
	else if (chatpid == getpid()) {
		andlog("[*] (%u) This is the chat process\n", getpid());
	}
	else {
		andlog("[*] Chat process not running\n");
	}

	andlog("[*] Exiting.\n");
	exit(EXIT_SUCCESS);
}


static void
start_chat_process(char *iface)
{
	/* Kill current process */
	if (chatpid != 0) {
		int status;
		andlog("Sending SIGKILL to %u\n", chatpid);
		kill(chatpid, SIGKILL);	
		waitpid(chatpid, &status, 0);		
		chatpid = 0;
	}

	/* Start the chat process */
	chatpid = fork();

	if (chatpid < 0) 
		anderrs("failed to fork the chat process");

	/* Child process, call chat function */
	if (chatpid == 0) {

		/* Run chat process */
		chat_proc_run(iface);

		/* Unreached */
		exit(EXIT_SUCCESS);
	}
}


/*
 * Thread entry point.
 * Start the configuration thread.
 * Returns -1 on error.
 */
extern void *
conf_thread_run(void *iface)
{
	int sd = -1;
	int cfd;

	/* Make sure we got root! */
	if (getuid() != 0) {
		fprintf(stderr, "** Error: root privileges required!\n");
		goto err;
	}

	/* Initialize the crypto system */
	chat_crypto_init();

	/* Listen on configuration port */
	if ( (sd = tcp_listen(inet_addr("127.0.0.1"), 
			ntohs(CONF_PORT))) < 0) {
		fprintf(stderr, "** Error: Failed to listen to 127.0.0.1:%d "
			"for configuration (already running?), terminating\n", CONF_PORT);
		goto err;
	}

	/* Ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);

	/* Start the chat thread if the interface is up and
	 * running in ad-hoc mode and has an IPv4 address */
	if (wiface_running_adhoc((char *)iface)) 
		start_chat_process((char *)iface);	

	/* Register signal handlers */
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	andlog("(%u) conf thread accepting clients on 127.0.0.1:%u\n", 
		getpid(), CONF_PORT);
	while ( (cfd = accept(sd, NULL, NULL)) > 0) {
		struct arg *a;

		/* Spawn a thread that handle the client */
		andlog("conf client connected\n");
		a = calloc(1, sizeof(struct arg));
		a->cfd = cfd;
		a->iface = iface;
		thread_spawn(handle_client, a);
	}

	andlog("conf thread finished\n");
	return NULL;

	err:
		if (sd >= 0)
			close(sd);

		return NULL;
}

