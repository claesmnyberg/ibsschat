 !!
 !!  File: bfish_encrypt_sparc.S
 !!  Author: Claes M. Nyberg <pocpon@fuzzpoint.com>
 !!  Description: Blowfish encryption routine for sparc
 !!  Version: 1.0
 !!  Date: Thu Oct 17 19:40:50 CEST 2002
 !!
 !!  Copyright (c) 2002 Claes M. Nyberg <pocpon@fuzzpoint.com>
 !!  All rights reserved, all wrongs reversed.
 !!
 !!  Redistribution and use in source and binary forms, with or without
 !!  modification, are permitted provided that the following conditions
 !!  are met:
 !!
 !!  1. Redistributions of source code must retain the above copyright
 !!     notice, this list of conditions and the following disclaimer.
 !!  2. Redistributions in binary form must reproduce the above copyright
 !!     notice, this list of conditions and the following disclaimer in the
 !!     documentation and/or other materials provided with the distribution.
 !!  3. The name of author may not be used to endorse or promote products
 !!     derived from this software without specific prior written permission.
 !!
 !!  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 !!  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 !!  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 !!  THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 !!  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 !!  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 !!  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 !!  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 !!  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 !!  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 !!
 !!

.section ".text"
    .align 4
    .global bfish_encrypt_swap

!!
!! bfish_encrypt_swap()
!! We try to speed things up by unfolding the loop and swap
!! the usage of the registers instead of the values in them.
!!
!! Argumets:
!! i0  - Address to left side 32 bit block to encrypt.
!! i1  - Address to right side 32 bit block to encrypt.
!! i2  - The blowfish key initialized by bfish_keyinit().
!! i3  - Little endian swap flag, ignored.
!!

! Register usage
! *left   = %l0
! *right  = %l1
! &sbox   = %l2
! Pbox[i] = %l3
! fxl     = %l4
! sbox[i] = %l5
! tmp     = %l6
! tmp2    = %l7

    bfish_encrypt_swap:
        save    %sp, -120, %sp          ! Prologue

        ! Load left and right into registers
        ld      [%i0], %l0              ! *left
        ld      [%i1], %l1              ! *right

        ! Keep address of S boxes in %l2
        add      %i2, 72, %l2           ! &sbox = &pbox + 18

        !! Start of loop 1
        ld      [%i2], %l3              ! %l3 = P[0]
        xor     %l0, %l3, %l0           ! *left = *left ^ P[0];

        !! The "F" function
        srl     %l0, 24, %l6            ! a = (*left >> 24)
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4        ! fxl = s[0][a]

        srl     %l0, 16, %l6            ! b = (*left >> 16)
        and     %l6, 0xff, %l6          ! b &= 0xff
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7      ! S[1][b]
        add     %l4, %l7, %l4           ! fxl += S[1][b]
        
        srl     %l0, 8, %l6             ! c = (*left >> 8)
        and     %l6, 0xff, %l6          ! c &= 0xff
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7      ! S[2][c] 
        xor     %l4, %l7, %l4           ! fxl = fxl ^ S[2][c]
        
        and     %l0, 0xff, %l6          ! d = (*left & 0xff)
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7      ! S[3][d]
        add     %l4, %l7, %l4           ! fxl += S[3][d]    

        xor     %l1, %l4, %l1           ! *right = *right ^ fxl

        !! Start of loop 2, right = %l0 left = %l1
        ld      [%i2+4], %l3            ! %l3 = P[1]  
        xor     %l1, %l3, %l1          
        
        !! The "F" function
        srl     %l1, 24, %l6          
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4     
        
        srl     %l1, 16, %l6        
        and     %l6, 0xff, %l6     
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7 
        add     %l4, %l7, %l4     

        srl     %l1, 8, %l6      
        and     %l6, 0xff, %l6  
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7     
        xor     %l4, %l7, %l4         

        and     %l1, 0xff, %l6       
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7  
        add     %l4, %l7, %l4      

        xor     %l0, %l4, %l0     

        !! Start of loop 3
        ld      [%i2+8], %l3              ! %l3 = P[2]
        xor     %l0, %l3, %l0   

        !! The "F" function
        srl     %l0, 24, %l6   
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4    

        srl     %l0, 16, %l6       
        and     %l6, 0xff, %l6    
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7 
        add     %l4, %l7, %l4     

        srl     %l0, 8, %l6      
        and     %l6, 0xff, %l6  
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7  
        xor     %l4, %l7, %l4      

        and     %l0, 0xff, %l6    
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7  
        add     %l4, %l7, %l4      

        xor     %l1, %l4, %l1     

        !! Start of loop 4, right = %l0 left = %l1
        ld      [%i2+12], %l3            ! %l3 = P[3]
        xor     %l1, %l3, %l1

        !! The "F" function
        srl     %l1, 24, %l6
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4

        srl     %l1, 16, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7
        add     %l4, %l7, %l4

        srl     %l1, 8, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7
        xor     %l4, %l7, %l4

        and     %l1, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7
        add     %l4, %l7, %l4

        xor     %l0, %l4, %l0

        !! Start of loop 5
        ld      [%i2+16], %l3              ! %l3 = P[4]
        xor     %l0, %l3, %l0           

        !! The "F" function
        srl     %l0, 24, %l6            
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4        

        srl     %l0, 16, %l6            
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7      
        add     %l4, %l7, %l4           

        srl     %l0, 8, %l6             
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7      
        xor     %l4, %l7, %l4           

        and     %l0, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7      
        add     %l4, %l7, %l4           

        xor     %l1, %l4, %l1           

        !! Start of loop 6, right = %l0 left = %l1
        ld      [%i2+20], %l3            ! %l3 = P[5]
        xor     %l1, %l3, %l1

        !! The "F" function
        srl     %l1, 24, %l6
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4

        srl     %l1, 16, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7
        add     %l4, %l7, %l4

        srl     %l1, 8, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7
        xor     %l4, %l7, %l4

        and     %l1, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7
        add     %l4, %l7, %l4

        xor     %l0, %l4, %l0

        !! Start of loop 7
        ld      [%i2+24], %l3              ! %l3 = P[6]
        xor     %l0, %l3, %l0           

        !! The "F" function
        srl     %l0, 24, %l6            
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4        

        srl     %l0, 16, %l6            
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7      
        add     %l4, %l7, %l4           

        srl     %l0, 8, %l6             
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7      
        xor     %l4, %l7, %l4           

        and     %l0, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7      
        add     %l4, %l7, %l4           

        xor     %l1, %l4, %l1           

        !! Start of loop 8, right = %l0 left = %l1
        ld      [%i2+28], %l3            ! %l3 = P[7]
        xor     %l1, %l3, %l1

        !! The "F" function
        srl     %l1, 24, %l6
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4

        srl     %l1, 16, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7
        add     %l4, %l7, %l4

        srl     %l1, 8, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7
        xor     %l4, %l7, %l4

        and     %l1, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7
        add     %l4, %l7, %l4

        xor     %l0, %l4, %l0

        !! Start of loop 9
        ld      [%i2+32], %l3             ! %l3 = P[8]
        xor     %l0, %l3, %l0           

        !! The "F" function
        srl     %l0, 24, %l6            
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4        

        srl     %l0, 16, %l6            
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7      
        add     %l4, %l7, %l4           

        srl     %l0, 8, %l6             
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7      
        xor     %l4, %l7, %l4           

        and     %l0, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7      
        add     %l4, %l7, %l4           

        xor     %l1, %l4, %l1           

        !! Start of loop 10, right = %l0 left = %l1
        ld      [%i2+36], %l3            ! %l3 = P[9]
        xor     %l1, %l3, %l1

        !! The "F" function
        srl     %l1, 24, %l6
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4

        srl     %l1, 16, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7
        add     %l4, %l7, %l4

        srl     %l1, 8, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7
        xor     %l4, %l7, %l4

        and     %l1, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7
        add     %l4, %l7, %l4

        xor     %l0, %l4, %l0

        !! Start of loop 11
        ld      [%i2+40], %l3              ! %l3 = P[10]
        xor     %l0, %l3, %l0           

        !! The "F" function
        srl     %l0, 24, %l6            
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4        

        srl     %l0, 16, %l6            
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7      
        add     %l4, %l7, %l4           

        srl     %l0, 8, %l6             
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7      
        xor     %l4, %l7, %l4           

        and     %l0, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7      
        add     %l4, %l7, %l4           

        xor     %l1, %l4, %l1           

        !! Start of loop 12, right = %l0 left = %l1
        ld      [%i2+44], %l3            ! %l3 = P[11]
        xor     %l1, %l3, %l1

        !! The "F" function
        srl     %l1, 24, %l6
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4

        srl     %l1, 16, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7
        add     %l4, %l7, %l4

        srl     %l1, 8, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7
        xor     %l4, %l7, %l4

        and     %l1, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7
        add     %l4, %l7, %l4

        xor     %l0, %l4, %l0

        !! Start of loop 13
        ld      [%i2+48], %l3              ! %l3 = P[12]
        xor     %l0, %l3, %l0           

        !! The "F" function
        srl     %l0, 24, %l6            
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4        

        srl     %l0, 16, %l6            
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7      
        add     %l4, %l7, %l4           

        srl     %l0, 8, %l6             
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7      
        xor     %l4, %l7, %l4           

        and     %l0, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7      
        add     %l4, %l7, %l4           

        xor     %l1, %l4, %l1           

        !! Start of loop 14, right = %l0 left = %l1
        ld      [%i2+52], %l3            ! %l3 = P[13]
        xor     %l1, %l3, %l1

        !! The "F" function
        srl     %l1, 24, %l6
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4

        srl     %l1, 16, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7
        add     %l4, %l7, %l4

        srl     %l1, 8, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7
        xor     %l4, %l7, %l4

        and     %l1, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7
        add     %l4, %l7, %l4

        xor     %l0, %l4, %l0

        !! Start of loop 15
        ld      [%i2+56], %l3              ! %l3 = P[14]
        xor     %l0, %l3, %l0           

        !! The "F" function
        srl     %l0, 24, %l6            
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4        

        srl     %l0, 16, %l6            
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7      
        add     %l4, %l7, %l4           

        srl     %l0, 8, %l6             
        and     %l6, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7      
        xor     %l4, %l7, %l4           

        and     %l0, 0xff, %l6          
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7      
        add     %l4, %l7, %l4           

        xor     %l1, %l4, %l1           

        !! Start of loop 16, right = %l0 left = %l1
        ld      [%i2+60], %l3            ! %l3 = P[15]
        xor     %l1, %l3, %l1

        !! The "F" function
        srl     %l1, 24, %l6
        sll     %l6, 2, %l6
        ld      [%l2 + %l6], %l4

        srl     %l1, 16, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(256*4)], %l7
        add     %l4, %l7, %l4

        srl     %l1, 8, %l6
        and     %l6, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(512*4)], %l7
        xor     %l4, %l7, %l4

        and     %l1, 0xff, %l6
        sll     %l6, 2, %l6
        add     %l6, %l2, %l6
        ld      [%l6+(768*4)], %l7
        add     %l4, %l7, %l4

        xor     %l0, %l4, %l0

        !! End of all loops

        ld      [%i2+64], %l3           ! %l3 = P[16]
        xor     %l0, %l3, %l0           ! *right ^= P[16];
        ld      [%i2+68], %l3           ! %l3 = P[17]
        xor     %l1, %l3, %l1           ! *left ^= P[17];

        ! Write encrypted value back to address
        st      %l1, [%i0]
        st      %l0, [%i1]
        ret
        restore                        ! Epilogue

