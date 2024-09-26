/*
 *    File: ibsschat.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * Main file for the Android ibsschat program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ibsschat.h"



/* Local routines*/
static void usage(const char *);
static int start_daemons(int, const char *);

/*
 * Start all the daemons.
 * Returns -1 on error.
 */
extern int
start_daemons(int dofork, const char *iface)
{
	/* Make sure we got r00t! */
	if (getuid() != 0) {
		fprintf(stderr, "** Error: root privileges required!\n");
		return -1;
	}

  	/* Fork twice to becoma a daemon.
     * When started as an Android service on LineageOS
     * we have already forked so we do not need to do that again */
    if (dofork) {
        /* Become daemon */
        if (fork_twice() > 0)
            exit(EXIT_SUCCESS);
        printf("[%u]\n", getpid());
    }

	/* Run configuration thread */
	conf_thread_run((void *)iface);
	return 0;
}


/*
 * Print command line usage
 */
static void
usage(const char *pname)
{

	printf("The IBSS Chat software, version %s\n", IBSSCHAT_VERSION);
	printf("Author: Claes M. Nyberg <cnyberg@nps.edu>\n");
	printf("Usage:\n");
	printf("   %s --daemon-nofork <iface>\n", pname);
	printf("   %s --daemon <iface>\n", pname);
	printf("   %s --status <iface>\n", pname);
	printf("   %s --conf <iface> <ipv4> <netmask> <network-name> <channel> <key>\n", pname);
	printf("   %s --chat-send <iface> <message>\n", pname);
	printf("   %s --chat-send-rand <iface> <count> <delay-sec>\n", pname);
	printf("   %s --chat-prompt <iface>\n", pname);
	exit(EXIT_SUCCESS);
}


int
main(int argc, char **argv)
{

	if (argc == 1)
		usage(argv[0]);

	/* Start as an Android service (dont fork) */
	if (strcmp(argv[1], "--daemon-nofork") == 0) {
		if (argc == 3)
			exit(start_daemons(0, argv[2]));
	}

	/* Start daemon */
	if (strcmp(argv[1], "--daemon") == 0) {
		if (argc == 3)
			exit(start_daemons(1, argv[2]));
	}

	/* Run as client */	
	if (strcmp(argv[1], "--conf") == 0) {
		if (argc == 8)
			exit(conf_client(argc-1, &argv[1]));
	}

	/* Run as client */	
	if (strcmp(argv[1], "--status") == 0) {
		if (argc == 3)
			exit(conf_client(argc-1, &argv[1]));
	}

	/* Chat client send message */	
	if (strcmp(argv[1], "--chat-send") == 0) {
		if (argc == 4)
			exit(chat_send(argv[2], argv[3]));
	}

	/* Chat client send message */	
	if (strcmp(argv[1], "--chat-send-rand") == 0) {
		if (argc == 5)
			exit(chat_send_rand(argv[2], atoi(argv[3]), atoi(argv[4])));
	}

	/* Chat client send message */	
	if (strcmp(argv[1], "--chat-prompt") == 0) {
		if (argc == 3)
			exit(chat_prompt(argv[2]));
	}

	usage(argv[0]);	

	/* Shut up the compiler */
	exit(EXIT_FAILURE);
}
