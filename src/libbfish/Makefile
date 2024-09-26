 #
 #  File: Makefile
 #  Author: Claes M. Nyberg <pocpon@fuzzpoint.com>
 #  Description: Compiling rules for libbfish.a
 #  Version: 1.0
 #  Date: Thu Oct 17 19:40:50 CEST 2002
 #
 #  Copyright (c) 2002 Claes M. Nyberg <pocpon@fuzzpoint.com>
 #  All rights reserved, all wrongs reversed.
 #
 #  Redistribution and use in source and binary forms, with or without
 #  modification, are permitted provided that the following conditions
 #  are met:
 #
 #  1. Redistributions of source code must retain the above copyright
 #     notice, this list of conditions and the following disclaimer.
 #  2. Redistributions in binary form must reproduce the above copyright
 #     notice, this list of conditions and the following disclaimer in the
 #     documentation and/or other materials provided with the distribution.
 #  3. The name of author may not be used to endorse or promote products
 #     derived from this software without specific prior written permission.
 #
 #  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 #  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 #  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 #  THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 #  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 #  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 #  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 #  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 #  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 #  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 #
 #

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
SHELL       = /bin/sh
CC          = gcc
CFLAGS      = -Wall -O3 -s
AS          = as
ASFLAGS     =
AR          = ar
AROPTS      = -qcv
OBJS        = keyinit.o decrypt.o encrypt.o keyinit.o \
              cbc_encrypt.o cbc_decrypt.o ofb.o cfb.o
FILES       = keyinit.c decrypt.c encrypt.c keyinit.c \
              cbc_encrypt.c cbc_decrypt.c ofb.c cfb.c
SPARC_OBJS  = keyinit.o decrypt_sparc.o encrypt_sparc.o \
              cbc_encrypt.o cbc_decrypt.o ofb.o cfb.o
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

none: all

all: libbfish.a

libbfish.a: ${OBJS}
	${AR} ${AROPTS} libbfish.a ${OBJS}

use_sparc_asm: ${SPARC_OBJS}
	${AR} ${AROPTS} libbfish.a ${SPARC_OBJS}
	
encrypt_sparc.o: encrypt_sparc.S
	${AS} ${ASFLAGS} -o encrypt_sparc.o encrypt_sparc.S

decrypt_sparc.o: decrypt_sparc.S
	${AS} ${ASFLAGS} -o decrypt_sparc.o decrypt_sparc.S

decrypt.o: decrypt.c
encrypt.o: encrypt.c
keyinit.o: keyinit.c
cbc_encrypt.o: cbc_encrypt.c
cbc_decrypt.o: cbc_decrypt.c
ofb.o: ofb.c
cfb.o: cfb.c

clean:
	rm -f *.o libbfish.a
