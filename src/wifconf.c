/*
 *    File: wifconf.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * Android wirelss interface configuration for enabling 
 * IBSS (ad-hoc) mode and configuring channel.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/wireless.h>
#include <math.h>

#include "ibsschat.h"


/* Local routines */
static int ioctl_retry(int, int, void *);
static int iwchan(struct iwreq *);
static void iwsetchan(double chan, struct iw_freq *);


/*
 * Run IOCTL command
 * Return 0 on success, -1 on failure;
 */
static int 
ioctl_retry(int sd, int cmd, void *data)
{
    int ret;
    int i;

    ret = -1;

	/* Some cards need some time so we try
     * multiple times before giving up */
    for (i=0; i < 10; i++) {
        if ( (ret = ioctl(sd, cmd, data)) < 0) {
            usleep(5000);
            continue;
        }

        break;
    }
    return(ret);
}

/*
 * Translate frequency to channel
 */
static int
iwchan(struct iwreq *wrq)
{
	float f;
	int mod_chan;
	int i;

    /* 80211b frequencies to channels */
    int freqs[] = {
        2412, 2417, 2422, 2427, 2432,
        2437, 2442, 2447, 2452, 2457,
        2462, 2467, 2472, 2484,
        5180, 5200, 5210, 5220, 5240,
        5250, 5260, 5280, 5290, 5300,
        5320, 5745, 5760, 5765, 5785,
        5800, 5805, 5825,
        -1
    };

    int channels[] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 36, 40, 42, 44, 48,
        50, 52, 56, 58, 60, 64, 149, 152, 153, 157,
        160, 161, 165
    };

    f = ((float)wrq->u.freq.m) * pow(10,wrq->u.freq.e);
    mod_chan = (int) rintf(f / 1000000);

	for (i = 0; freqs[i] != -1; i++) {
		if (freqs[i] == mod_chan)
			return(channels[i]);
	}

    return(0);
}


/*
 * Set channel frequency
 */
static void
iwsetchan(double chan, struct iw_freq *wrq)
{
	wrq->e = (short) (floor(log10(chan)));

	if (wrq->e > 8) {
		wrq->m = ((long)(floor(chan / pow(10, wrq->e - 6)))) * 100;
		wrq->e -= 8;
	} else {
		wrq->m = (uint32_t)chan;
		wrq->e = 0;
	}
}

/*
 * Check if interface is running in
 * ad-hoc mode and have an IPv4 address.
 * Return 1 if interface is up and running
 * and 0  if it is down and -1 on error.
 */
int
wiface_running_adhoc(const char *iface)
{
	uint32_t ipv4;
	uint32_t mask;
	int ret = 0;

	/* Make sure interface is up */
	if ( (ret = ifconfig_up(iface)) != 1)
		return ret;

	ret = ifconfig_get(iface, &ipv4, &mask);
	if (ret != 0)
		return ret;

	/* No IPv4 address set */
	if (ipv4 == 0) {
		andlog("Interface %s has no IPv4 address set\n", iface);
		return 0;
	}


	/* Last thing to check is that we are running in ad-hoc mode */
		

	return 1;
}

/*
 * Translate mode to string.
 */
const char *
wimode(int mode)
{
    switch (mode){
        case IW_MODE_AUTO: return "auto";
        case IW_MODE_ADHOC: return "ad-hoc";
        case IW_MODE_INFRA: return "infra";
        case IW_MODE_MASTER: return "access-point";
        case IW_MODE_REPEAT: return "repeater";
        case IW_MODE_SECOND: return "secondary";
        case IW_MODE_MONITOR: return "monitor";
        case IW_MODE_MESH: return "mesh";
    }
    return "unknown";
}


/*
 * Return 1 if interface is in ad-hoc mode, 
 * 0 otherwise and -1 on error.
 */ 
int
wiface_adhoc(const char *iface)
{
	struct iwreq wrq;
	int ret = 0;
	int fd;


	/* Create IOCTL socket */
	if ( (fd = socket_open()) < 0) 
		return -1;

	snprintf(wrq.ifr_name, IFNAMSIZ, "%s", iface);
	if (ioctl_retry(fd, SIOCGIWMODE, &wrq) < 0) {
		anderrs("Failed to get interface mode");
		ret = -1;
		goto finished;
	}


	andlog("Interface %s running in mode %s\n", 
		iface, wimode(wrq.u.mode));

	if (wrq.u.mode == IW_MODE_ADHOC)
		ret = 1;


	finished:
		if (fd >= 0)
			close(fd);

		return ret;
}

/*
 * Get values from wireless interface and fill them
 * into the wiface structure.
 * Returns 0 on success, -1 on error.
 */
int
wiface_status(int ifd, struct wiface *w)
{
	struct iwreq wrq;
	struct ifreq ifr;	

	snprintf(wrq.ifr_name, IFNAMSIZ, "%s", w->iface);
	snprintf(ifr.ifr_name, IFNAMSIZ, "%s", w->iface);


	/* Fetch IPv4 */
	if (ioctl_retry(ifd, SIOCGIFADDR, &ifr) < 0) {
		anderr("** Error: Failed to get IPV4 for interface %s\n", 
			w->iface);
	}
	else
		w->ipv4 = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

	/* Fetch Network mask */
	if (ioctl_retry(ifd, SIOCGIFNETMASK, &ifr) < 0) {
		anderr("** Error: Failed to get mask for interface %s\n", 
			w->iface);
	}
	else
		w->mask = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
		

	/* Fetch essid */
	memset(w->essid, 0x00, IW_ESSID_MAX_SIZE);
	wrq.u.essid.pointer = (caddr_t)w->essid;
	wrq.u.essid.length = IW_ESSID_MAX_SIZE;	
	wrq.u.essid.flags = 0;
	if (ioctl_retry(ifd, SIOCGIWESSID, &wrq) < 0) {
		anderr("** Error: Failed to get ESSID for interface %s\n", 
			w->iface);
	}

	/* Fetch Access Point MAC address */
	if (ioctl_retry(ifd, SIOCGIWAP, &wrq) < 0) {
		anderr("** Error: Failed to get AP for interface %s\n",
			w->iface);
		memset(w->mac_bssid, 0x00, 6);
	}
	else
		memcpy(w->mac_bssid, &wrq.u.ap_addr.sa_data, 6);

    /* Fetch channel */
    if (ioctl_retry(ifd, SIOCGIWFREQ, &wrq) < 0) {
        anderr("** Error: Failed to get channel for interface %s\n", 
			w->iface);
		w->channel = 0;
    }
	else
    	w->channel = iwchan(&wrq);


    /* Fetch mode */
    if (ioctl_retry(ifd, SIOCGIWMODE, &wrq) < 0) {
        anderr("** Error: Failed to get mode for interface %s\n", 
			w->iface);
		w->mode = -1;
    }
	else
    	w->mode = wrq.u.mode;

	/* Get the encryption key */
	if (chat_crypto_get_key(w->key) == 0) {
		anderr("** Error: Failed to get encryption key for interface %s\n",
			w->iface);
	}
	return 0;
}


/*
 * Configure wireless interface.
 * Return 0 on success, -1 on error.
 */
int
wiface_conf(int ifd, struct wiface *w)
{
	static int wlock = 0;
	struct iwreq wrq;
	int ret = 0;

	snprintf(wrq.ifr_name, IFNAMSIZ, "%s", w->iface);

	/* Wait for write lock */
	while (wlock != 0)
		usleep(100);
	wlock++;

	andlog("Configuring interface\n");
	ifconfig_down(w->iface);
	ifconfig_down(w->iface);

	andlog("Writing firmware paths\n");
	write_paths();

	andlog("Configure IPv4 address and netmask\n");
	ifconfig_set(w->iface, w->ipv4, w->mask);
	ifconfig_up(w->iface);

	/* Set mode */
	wrq.u.mode = w->mode;
	andlog("Setting mode to %d for %s\n", w->mode, w->iface);
	if (ioctl_retry(ifd, SIOCSIWMODE, &wrq) < 0) {
		anderrs("Failed to set wireless mode");
		ret = -1;
		goto finished;
	}
	
	/* Set channel */
	if (w->channel != 0) {
		andlog("Setting channel to %d for %s\n", w->channel, w->iface);		
		iwsetchan(w->channel, &wrq.u.freq);

		if (ioctl_retry(ifd, SIOCSIWFREQ, &wrq) < 0) {
			anderrs("Failed to set wireless channel");
			ret = -1;
			goto finished;
		}
	}

	/* Set ESSID */
	if (w->essid[0] != '\0') {
		wrq.u.essid.pointer = (caddr_t)w->essid;
		wrq.u.essid.length = strlen((char *)w->essid);
		wrq.u.essid.flags = 1;
		andlog("Setting ESSID to \"%s\" for interface %s\n", 
			w->essid, w->iface);

		if (ioctl_retry(ifd, SIOCSIWESSID, &wrq) < 0) {
			anderrs("Failed to set ESSID");
			ret = -1;
			goto finished;
		}
	}

	/* Set BSSID */
	if (memcmp(w->mac_bssid, "\x00\x00\x00\x00\x00\x00", 6) != 0) {
		wrq.u.ap_addr.sa_family = ARPHRD_ETHER;
		memcpy(&wrq.u.ap_addr.sa_data, w->mac_bssid, 6);
		andlog("Setting BSSID to %s for interface %s\n",
			net_macstr((uint8_t *)wrq.u.ap_addr.sa_data), wrq.ifr_name);
	
		if (ioctl_retry(ifd, SIOCSIWAP, &wrq) < 0) {
			anderrs("Failed to set BSSID");
			ret = -1;
			goto finished;
		}
	}

	/* Set the encryption key */
	chat_crypto_set_key(w->key, strlen((char *)w->key));

	finished:
		wlock = 0;
		return ret;
}

