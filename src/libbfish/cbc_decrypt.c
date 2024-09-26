/*
 *  File: bfish_cbc_decrypt.c
 *  Author: Claes M. Nyberg <pocpon@fuzzpoint.com>
 *  Description: Blowfish Cipher Block Chaining Mode decryption routine.
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
 * Decrypt a string of bytes using CBC mode.
 * Arguments:
 * str   - The CBC encrypted string of bytes to decrypt.
 * slen  - The length of str in bytes.
 * iv    - The eight byte long initial vector.
 * bk    - The blowfish key initialized by bfish_keyinit()
 */
void
bfish_cbc_decrypt(u_char *str, u_long slen, u_char *iv, struct bfish_key *bk)
{
    register u_long *xl;
    register u_long *xr;
    u_long fedbk[6];    /* Feedback register */
    u_long tmp;

    /* Special case, str is shorter than one block.
     * Fix this later. */
    if (slen < 8) {
        fprintf(stderr, "bfish_cbc_decrypt(): Input buffer to short"
            " (<8 bytes), aborting!\n");
        return;
    }

    /* Save the second last full size block if we are not 
     * aligned to eight byte.
     * We need this later to decrypt since we use 
     * ciphertext stealing for the last short block. */
    tmp = slen % 8;
    if (tmp != 0) {
        
        if ((slen - tmp) >= 16) {
            fedbk[4] = *((u_long *)&str[(slen - tmp) - 16]);
            fedbk[5] = *((u_long *)&str[(slen - tmp) - 12]);
        }
        else {
            fedbk[4] = *((u_long *)&iv[0]);
            fedbk[5] = *((u_long *)&iv[4]);
        }
    }
    
    xl = (u_long *)str;
    xr = (u_long *)(str +4);

    /* The next block needs the previous 
     * block unencrypted for XOR */
    fedbk[0] = *xl;
    fedbk[1] = *xr;
    
    bfish_decrypt(xl, xr, bk);
    *xl ^= *((u_long *)&iv[0]);
    *xr ^= *((u_long *)&iv[4]);

    xl += 2;
    xr += 2;
    slen -= 8;

    /* Decrypt all full size blocks */
    while (slen >= 8) {

        fedbk[2] = *xl;
        fedbk[3] = *xr;

        bfish_decrypt(xl, xr, bk);
        *xl ^= fedbk[0]; 
        *xr ^= fedbk[1]; 

        fedbk[0] = fedbk[2];
        fedbk[1] = fedbk[3];

        xl += 2;
        xr += 2;
        slen -= 8;
    }

    /* Decrypt the last short block encrypted using "ciphertext stealing".
     * Applied Cryptography Sec. edition page 196 */
    if (slen > 0) {
        u_char last[8];    /* The last short block */

        memset(last, 0x00, sizeof(last));
        memcpy(last, xl, slen);
        
        bfish_decrypt(&fedbk[0], &fedbk[1], bk);        

        fedbk[0] ^= *((u_long *)&last[0]);
        fedbk[1] ^= *((u_long *)&last[4]);

        memcpy(last + slen, (u_char *)&fedbk[0] + slen, sizeof(last) - slen);
        bfish_decrypt((u_long *)&last[0], (u_long *)&last[4], bk);

        *(u_long *)&last[0] ^= fedbk[4];
        *(u_long *)&last[4] ^= fedbk[5];

        memcpy(xl - 2, last, sizeof(last));
        memcpy(xl, &fedbk[0], slen);
    }
}
