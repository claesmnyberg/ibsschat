/*
 *    File: fwpaths.c
 * Version: 1.0
 *    What: Part of IBSS Chat program
 *  Author: Claes M. Nyberg
 *   Where: Naval Postgraduate School
 *    When: Spring 2018
 *
 * This file implement routines for configuring
 * the firmware path for Broadcom chipset firmware
 * files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ibsschat.h"


static char *fwpath = NULL;
static char *nvpath = NULL;

/* Local routines */
static int init_paths();


/*
 * Set firmware path to use.
 */
int
fw_setpath(const char *path)
{
	if (fwpath != NULL)
		free(fwpath);
	fwpath = strdup(path);
	andlog("Firmware path set to %s\n", fwpath);
	return 0;
}


/*
 * Set nvram path to use.
 */
int
nv_setpath(const char *path)
{
    if (nvpath != NULL)
        free(nvpath);
    nvpath = strdup(path);
	andlog("NV RAM path set to %s\n", nvpath);
	return 0;
}


/*
 * Try to locate default files.
 * Return 0 on success, -1 on error.
 * TODO: Improve this by scanning files for the version
 * string and look for the "-aibss-" tag to conclude IBSS 
 * functionality.
 */
static int
init_paths()
{	
	struct stat sb;

	/* Firmware path */
	if (fwpath == NULL) {
		if (stat("/etc/wifi/bcmdhd_ibss.bin", &sb) == 0)
			fw_setpath("/etc/wifi/bcmdhd_ibss.bin");
		else if (stat("/etc/wifi/fw_bcmdhd_ibss.bin", &sb) == 0)
			fw_setpath("/etc/wifi/fw_bcmdhd_ibss.bin");
		else if (stat("/vendor/firmware/bcmdhd_ibss.bin", &sb) == 0)
			fw_setpath("/vendor/firmware/bcmdhd_ibss.bin");
		else if (stat("/vendor/firmware/fw_bcmdhd_ibss.bin", &sb) == 0)
			fw_setpath("/vendor/firmware/fw_bcmdhd_ibss.bin");
	}

	/* NV ram path */
	if (nvpath == NULL) {
		if (stat("/etc/wifi/nvram_net.txt", &sb) == 0)
			nv_setpath("/etc/wifi/nvram_net.txt");
		else if (stat("/vendor/firmware/nvram_net.txt", &sb) == 0)
			nv_setpath("/vendor/firmware/nvram_net.txt");
	}

	return (fwpath != NULL) && (nvpath != NULL);
}


/*
 * Write Broadcom firmware paths.
 * Return 0 on success, -1 on error.
 */
int
write_paths()
{
	FILE *fp;

#ifndef __ANDROID__
	return 0;
#endif

	init_paths();

	if (fwpath == NULL) {
		anderr("** Error: Firmware path not set!");
		return -1;
	}

	if (nvpath == NULL) {
		anderr("** Error: Firmware path not set!");
		return -1;
	}

	fp = fopen("/sys/module/dhd/parameters/firmware_path", "w+");
	if (fp == NULL) {
		fp = fopen("/sys/module/bcmdhd/parameters/firmware_path", "w+");
		if (fp == NULL) {
			anderrs("Could not open firmware configuration path\n");
			return -1;
		}
	}
	fprintf(fp, "%s\n", fwpath);
	fclose(fp);

	fp = fopen("/sys/module/dhd/parameters/nvram_path", "w+");
	if (fp == NULL) {
		fp = fopen("/sys/module/bcmdhd/parameters/nvram_path", "w+");
		if (fp == NULL) {
			anderrs("Could not open nvram configuration path\n");
			return -1;
		}
	}
	fprintf(fp, "%s\n", nvpath);
	fclose(fp);

	return 0;
}

