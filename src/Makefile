#
#    File: Makefile
# Version: 1.0
#    What: Part of Android ad-hoc configuration program
#  Author: Claes M. Nyberg
#   Where: Naval Postgraduate School
#    When: Spring 2018
#
# make(1) file for building Android ibsschat program
#

# The NDK to use
NDK_ROOT=/home/cmn/nps.large/Thesis/android-ndk-r11c


all: libs/armeabi/ibsschat linux

libs/armeabi/ibsschat: ibsschat.c Android.mk
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. \
		TARGET_PLATFORM=android-21

install_all:
	@for device in `adb devices | grep device | grep -v List | cut -f1 | tr '\n' ' '`; \
	do \
		echo; \
		echo "[+] Installing $$device"; \
		make PARAM="-s $$device" DEVICE="$$device" install; \
	done

uninstall_all:
	@for device in `adb devices | grep device | grep -v List | cut -f1 | tr '\n' ' '`; \
	do \
		echo; \
		echo "[+] Un-installing $$device"; \
		make PARAM="-s $$device" DEVICE="$$device" uninstall; \
	done

PARAM=
install:
	adb ${PARAM} push libs/armeabi/ibsschat /sdcard/
	adb ${PARAM} shell 'su -c "mount -o remount,rw /system"'
	adb ${PARAM} shell 'su -c "cp -f /sdcard/ibsschat /system/bin/ibsschat"'
	adb ${PARAM} shell 'su -c "chmod 0755 /system/bin/ibsschat"'
	adb ${PARAM} shell 'su -c "chown shell:shell /system/bin/ibsschat"'
	adb ${PARAM} shell 'su -c "rm -f /sdcard/ibsschat"'
	adb ${PARAM} push ./scripts/ibsschat-start.sh /sdcard/
	adb ${PARAM} shell 'su -c "cp -f /sdcard/ibsschat-start.sh /system/bin/ibsschat-start.sh"'
	adb ${PARAM} shell 'su -c "chmod 0755 /system/bin/ibsschat-start.sh"'
	adb ${PARAM} shell 'su -c "chown shell:shell /system/bin/ibsschat-start.sh"'
	adb ${PARAM} shell 'su -c "ln -s /system/bin/ibsschat-start.sh /system/bin/is"'
	adb ${PARAM} shell 'su -c "rm -f /sdcard/ibsschat-start.sh"'
	adb ${PARAM} push ./scripts/ibsschat-conf.sh /sdcard/
	adb ${PARAM} shell 'su -c "cp -f /sdcard/ibsschat-conf.sh /system/bin/ibsschat-conf.sh"'
	adb ${PARAM} shell 'su -c "chmod 0755 /system/bin/ibsschat-conf.sh"'
	adb ${PARAM} shell 'su -c "chown shell:shell /system/bin/ibsschat-conf.sh"'
	adb ${PARAM} shell 'su -c "rm -f /sdcard/ibsschat-conf.sh"'
	adb ${PARAM} shell 'su -c "ln -s /system/bin/ibsschat-conf.sh /system/bin/ic"'
	adb ${PARAM} push ./scripts/ibsschat-prompt.sh /sdcard/
	adb ${PARAM} shell 'su -c "cp -f /sdcard/ibsschat-prompt.sh /system/bin/ibsschat-prompt.sh"'
	adb ${PARAM} shell 'su -c "chmod 0755 /system/bin/ibsschat-prompt.sh"'
	adb ${PARAM} shell 'su -c "chown shell:shell /system/bin/ibsschat-prompt.sh"'
	adb ${PARAM} shell 'su -c "rm -f /sdcard/ibsschat-conf.sh"'
	adb ${PARAM} shell 'su -c "ln -s /system/bin/ibsschat-prompt.sh /system/bin/ipr"'
	adb ${PARAM} push ./scripts/fw-block.sh /sdcard/
	adb ${PARAM} shell 'su -c "cp -f /sdcard/fw-block.sh /system/bin/fw-block.sh"'
	adb ${PARAM} shell 'su -c "chmod 0755 /system/bin/fw-block.sh"'
	adb ${PARAM} shell 'su -c "chown shell:shell /system/bin/fw-block.sh"'
	adb ${PARAM} shell 'su -c "rm -f /sdcard/fw-block.sh"'
	adb ${PARAM} shell 'su -c "ln -s /system/bin/fw-block.sh /system/bin/fwb"'
	adb ${PARAM} push ./scripts/ibsschat-msgtest.sh /sdcard/
	adb ${PARAM} shell 'su -c "cp -f /sdcard/ibsschat-msgtest.sh /system/bin/ibsschat-msgtest.sh"'
	adb ${PARAM} shell 'su -c "chmod 0755 /system/bin/ibsschat-msgtest.sh"'
	adb ${PARAM} shell 'su -c "chown shell:shell /system/bin/ibsschat-msgtest.sh"'
	adb ${PARAM} shell 'su -c "rm -f /sdcard/ibsschat-msgtest.sh"'
	adb ${PARAM} shell 'su -c "ln -s /system/bin/ibsschat-msgtest.sh /system/bin/mtst"'
	# Install device specific multi-hop simulation firewall rules
	#adb ${PARAM} push ./scripts/${DEVICE}/mhop.sh /sdcard/
	#adb ${PARAM} shell 'su -c "cp -f /sdcard/mhop.sh /system/bin/mhop.sh"'
	#adb ${PARAM} shell 'su -c "chmod 0755 /system/bin/mhop.sh"'
	#adb ${PARAM} shell 'su -c "chown shell:shell /system/bin/mhop.sh"'
	#adb ${PARAM} shell 'su -c "rm -f /sdcard/mhop.sh"'
	#adb ${PARAM} shell 'su -c "ln -s /system/bin/mhop.sh /system/bin/mhop"'

uninstall:
	adb ${PARAM} shell 'su -c "mount -o remount,rw /system"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/ibsschat"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/ibsschat-start.sh"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/is"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/ibsschat-conf.sh"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/ic"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/ibsschat-prompt.sh"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/ipr"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/fw-block.sh"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/fwb"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/ibsschat-msgtest.sh"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/mtst"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/mhop.sh"'
	adb ${PARAM} shell 'su -c "rm -f /system/bin/mhop"'

linux:
	gcc -Wall -o ibsschat *.c libbfish/*.c -lm -lpthread

new: clean libs/armeabi/ibsschat linux

clean:
	rm -Rf libs
	rm -Rf local
	rm -f ibsschat
