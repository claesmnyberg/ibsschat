#
#    File: Android.mk
# Version: 1.0
#    What: Part of Android ad-hoc configuration program
#  Author: Claes M. Nyberg
#   Where: Naval Postgraduate School
#    When: Spring 2018
#
# Android make file for building ibsschat
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	ibsschat.c \
	ifconfig.c \
	wifconf.c \
	utils.c \
	fwpaths.c \
	net.c \
	thread.c \
	iplist.c \
	msgbuf.c \
	libbfish/keyinit.c \
	libbfish/encrypt.c \
	libbfish/decrypt.c \
	libbfish/cbc_encrypt.c \
	libbfish/cbc_decrypt.c \
	linkedlist.c \
	conf_thread.c \
	conf_client.c \
	chat_crypto.c \
	chat_proc.c \
	chat_client.c \
	chat_mcast.c

LOCAL_CFLAGS := -O2 -Wall
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE := ibsschat
LOCAL_LDFLAGS += -Wl,--no-fatal-warnings -llog

include $(BUILD_EXECUTABLE)

