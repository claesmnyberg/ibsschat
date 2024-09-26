/*
 *    File: ifconfig.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * Android network interface configuration routines
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>

#include "ibsschat.h"


/*
 * Open socket for ioctl
 */
int
socket_open(void)
{
    int fd;

    if ( (fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        anderrs("Failed to create socket");
        return(-1);
    }

    return(fd);
}


/*
 * Bring down interface.
 */
int
ifconfig_down(const char *ifname)
{
    int ret;
    ret = ifconfig_clr_flags(ifname, IFF_UP);
    if (ret < 0)
        anderrs("Failed to bring interface down");

    return(ret);
}

/*
 * Bring interface up.
 */
int
ifconfig_up(const char *ifname)
{
    int ret;
    ret = ifconfig_set_flags(ifname, (IFF_UP | IFF_RUNNING));
    if (ret < 0)
        anderrs("Failed to bring interface up");

    return(ret);
}

/*
 * Check if interface is up and running.
 * Return 1 if interface is ip, 0 if it  is down
 * and -1 on error.
 */
int
ifconfig_is_up(const char *ifname)
{
    struct ifreq ifr;
    int ret = 0;
	int fd;

	snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);
	if ( (fd = socket_open()) < 0) {
		ret = -1;
		goto finished;
	}

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        anderrs("ioctl failed on interface");
        ret = -1;
        goto finished;
    }

	/* Interface is up and running */
	if ( ((ifr.ifr_flags) & (IFF_UP|IFF_RUNNING)) == (IFF_UP|IFF_RUNNING)) {
		andlog("Interface %s is up and running\n", ifname);
		ret = 1;
	}

	finished:
		if (fd >= 0)
			close(fd);

    return(ret);
}


/*
 * Set interface flags
 */
int
ifconfig_set_flags(const char *ifname, unsigned short flags)
{
    struct ifreq ifr;
    int fd = -1;
    int ret = 0;

    /* Create IOCTL socket */
    if ( (fd = socket_open()) < 0) {
        ret = -1;
        goto finished;
    }

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        anderrs("ioctl failed on interface");
        ret = -1;
        goto finished;
    }

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);
    ifr.ifr_flags |= flags;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        anderrs("Failed to set flags on interface");
        ret = -1;
        goto finished;
    }

    finished:
        if (fd >= 0)
            close(fd);

        return(ret);
}

/*
 * Clear interface flags.
 */
int
ifconfig_clr_flags(const char *ifname, unsigned short flags)
{
    struct ifreq ifr;
    int fd = -1;
    int ret = 0;

    /* Create IOCTL socket */
    if ( (fd = socket_open()) < 0) {
        ret = -1;
        goto finished;
    }

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        anderrs("ioctl failed on interface");
        ret = -1;
        goto finished;
    }

    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname);
    ifr.ifr_flags &= ~flags;

    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        anderrs("Failed to clear flags on interface");
        ret = -1;
        goto finished;
    }

    finished:
        if (fd >= 0)
            close(fd);

        return(ret);
}

/*
 * Set interface IPv4 address and network mask.
 * Returns 0 on success, -1 on error.
 */
int
ifconfig_set(const char *ifname, uint32_t ipv4, uint32_t netmask)
{
    struct ifreq ifr;
    struct sockaddr sa;
    struct sockaddr_in sin;
    int ret;
    int fd;

    memset(&ifr, 0x00, sizeof(ifr));
    memset(&sa, 0x00, sizeof(sa));
    memset(&sin, 0x00, sizeof(sin));
    ret = 0;

    /* Set interface name */
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);

    /* Create IOCTL socket */
    if ( (fd = socket_open()) < 0) {
        ret = -1;
        goto finished;
    }

    /* Set IPv4 */
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ipv4;
    memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr_in));

    if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) {
        anderrs("Failed to set IPv4 address");
        ret = -1;
    }

    /* Set netmask */
    if (netmask != 0) {
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = netmask;
        memcpy(&ifr.ifr_netmask, &sin, sizeof(struct sockaddr));

        if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0) {
            anderrs("Failed to set network mask");
            ret = -1;
        }
    }

    finished:
        if (fd > 0)
            close(fd);

    return ret;
}


/*
 * Get interface IPv4 address and network mask
 * Return 0 on success, -1 on error.
 */
int
ifconfig_get(const char *iface, uint32_t *ipv4, uint32_t *mask)
{
	struct ifreq ifr;
	int ret;
	int fd;

    memset(&ifr, 0x00, sizeof(ifr));
    ret = 0;

	if (iface == NULL) {
        anderrs("Interface name argument is NULL");
		return -1;
	}

	if (ipv4 != NULL)
		*ipv4 = 0;

	if (mask != NULL)
		*mask = 0;

    /* Set interface name */
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", iface);

    /* Create IOCTL socket */
    if ( (fd = socket_open()) < 0) {
        ret = -1;
        goto finished;
    }

    /* Get IPv4 */
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        anderrs("Failed to get IPv4 address");
        ret = -1;
    }

	if (ipv4 != NULL)
		*ipv4 = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

    /* Get netmask */
	if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
		anderrs("Failed to get network mask");
		ret = -1;
	}

	if (mask != NULL)
		*mask = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

    finished:
        if (fd > 0)
            close(fd);

	return ret;
}
