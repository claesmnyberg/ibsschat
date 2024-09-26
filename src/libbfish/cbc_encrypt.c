/*
 *  File: bfish_cbc_encrypt.c
 *  Author: Claes M. Nyberg <pocpon@fuzzpoint.com>
 *  Description: Blowfish Cipher Block Chaining Mode encryption routine.
 *  Version: 1.0
 *  Date: Thu Oct 17 19:40:50 CEST 2002
 *
 *  Copyright (c) 2002 Claes M. Nyberg <pocpon@fuzzpoint.com>
 *  All rights reserved, all wrongs reversed.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 *  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 *  THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "bfish.h"

/*
 * Encrypt a string of bytes using CBC mode.
 * XOR's the next block of plaintext with the previous block
 * of ciphertext to add confusion and diffusion.
 * Arguments:
 * str   - The string of bytes to encrypt.
 * slen  - The length of str in bytes.
 * iv    - The eight byte long initial vector.
 * bk    - The blowfish key initialized by bfish_keyinit()
 */
void
bfish_cbc_encrypt(u_char *str, u_long slen, u_char *iv, struct bfish_key *bk)
{
    register u_long *xl;
    register u_long *xr;


    /* Special case, str is shorter than one block 
     * Fix this later. */
    if (slen < 8) {
        fprintf(stderr, "bfish_cbc_encrypt(): Input buffer to short"
            " (< 8 bytes), aborting!\n");
        return;            
    }

    xl = (u_long *)str;
    xr = (u_long *)(str +4);

    *xl ^= *((u_long *)&iv[0]);
    *xr ^= *((u_long *)&iv[4]);

    bfish_encrypt(xl, xr, bk);
    xl += 2;
    xr += 2;
    slen -= 8;

    /* Encrypt all full size blocks */
    while (slen >= 8) {
    
        *xl ^= *(xl -2);
        *xr ^= *(xr -2);
        bfish_encrypt(xl, xr, bk);
        
        xl += 2;
        xr += 2;
        slen -= 8;
    }

    /* Encrypt the last short block using "ciphertext stealing".
     * Applied Cryptography Sec. edition page 196 */
    if (slen > 0) {
        u_char prev[8];    /* The previous full size block */
        u_char last[8];    /* The last short block */

        memset(last, 0x00, sizeof(last));
        memcpy(last, xl, slen);
        memcpy(prev, (xl - 2), sizeof(prev));

        *((u_long *)&last[0]) ^= *((u_long *)&prev[0]);
        *((u_long *)&last[4]) ^= *((u_long *)&prev[4]);

        bfish_encrypt((u_long *)&last[0], (u_long *)&last[4], bk);

        memcpy(xl - 2, last, sizeof(last));
        memcpy(xl, prev, slen);
    }
}
