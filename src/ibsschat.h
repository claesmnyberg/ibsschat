/*
 *    File: ibsschat.h
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * This file is the main header file for
 * the IBSS configuration program.
 */

#ifndef _IBSSCONF_H
#define _IBSSCONF_H

/* The current version */
#define IBSSCHAT_VERSION "1.2"

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

#include "conf.h"
#include "thread.h"
#include "chat.h"

/* iplist.c */
extern int iplist_add(uint32_t, uint32_t **, size_t);
extern void iplist_print(uint32_t *, size_t);

/* utils.c */
#define ANDROID_LOG_TAG "ibsschat"
extern void andlog(const char *, ...);
extern void anderr(const char *, ...);
extern void anderrs(const char *);
extern ssize_t writen(int, void *, size_t);
extern ssize_t readn(int, void *, size_t);
extern const char *net_macstr(const unsigned char *);
extern int fork_twice();
extern int data_to_read(int);
extern int getrand_nonblock(uint8_t *, size_t);

/* ifconfig.c */
extern int socket_open(void);
extern int ifconfig_set_flags(const char *, unsigned short);
extern int ifconfig_clr_flags(const char *, unsigned short);
extern int ifconfig_up(const char *);
extern int ifconfig_is_up(const char *);
extern int ifconfig_down(const char *);
extern int ifconfig_set(const char *, uint32_t, uint32_t);
extern int ifconfig_get(const char *, uint32_t *, uint32_t *);

/* wifconf.c */
extern int wiface_status(int sd, struct wiface *);
extern int wiface_conf(int sd, struct wiface *);
extern int wiface_running_adhoc(const char *);
extern int ifconfig_up(const char *);
extern int wiface_adhoc(const char *);
extern const char *wimode(int);

/* fwpaths.c */
extern int write_paths(void);
extern int nv_setpath(const char *);
extern int fw_setpath(const char *);

/* net.c */
extern int unix_socket_listen(const char *, const char *);
extern int unix_socket_connect(const char *, const char *);
extern int data_to_read(int);
extern int tcp_connect(uint32_t, uint16_t, uint32_t, uint16_t);
extern int tcp_listen(uint32_t, uint16_t);
extern const char *ipstr(uint32_t);
extern uint16_t chksum(uint16_t *, int);

#endif /* _IBSSCONF_H */
