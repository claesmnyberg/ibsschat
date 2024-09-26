

libbfish 1.0
Author: Claes M. Nyberg <pocpon@fuzzpoint.com>

Blowfish encryption library in C and SPARC assembly for integration in
other code. It has been verified against the Blowfish implementation in OpenSSL.


Compiling: -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

To compile this library type 'make' or 'make use_sparc_asm'.


Library functions: -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

struct bfish_key *
bfish_keyinit(u_char *key, u_short klen);
    
    This is the "init" function that sets up all the P and S boxes for
    encryption/decryption.
    
	Arguments:
        key   - The user supplied key.
        klen  - The length of key in bytes.

    Returns NULL on error and a pointer to a bfish_key structure on
    success.

void
bfish_encrypt(u_long *xl, u_long *xr, struct bfish_key *bk)
    
    This is in fact a macro that calls bfish_encrypt_swap() with
    swap set to SWAP_LIL_ENDIAN to enable byte order compability.
    Use this for encryption of xl and xr in Electronic Code Book Mode (EBC).
    
	Arguments:
        xl    - Address off left side 32 bit block to encrypt.
        xr    - Address off right side 32 bit block to encrypt.
        bk    - The blowfish key initialized by bfish_keyinit().


void 
bfish_encrypt_swap(u_long *xl, u_long *xr, struct bfish_key *bk, int swap);
    
    This is the "real" encryption routine called by bfish_keyinit() with swap
    set to NO_ENDIAN_SWAP/false to disable byte order swapping of the P and S
    boxes. You should not call this routine directly, use the macro bfish_encrypt()
    instead.
    
	Arguments:
        xl    - Address off left side 32 bit block to encrypt.
        xr    - Address off right side 32 bit block to encrypt.
        bk    - The blowfish key to use.
        swap  - Flag to enable an output of big endian ordered blocks.


void 
bfish_decrypt(u_long *xl, u_long *xr, struct bfish_key *bk);
    
    Decipher routine.
	
    Arguments:
        xl    - Address off left side 32 bit block to decrypt.
        xr    - Address off right side 32 bit block to decrypt.
        bk    - The blowfish key received by bfish_keyinit().
    
void
bfish_cbc_encrypt(u_char *str, u_long slen, u_char *iv, struct bfish_key *bk)
    
    Cipher Block Chaining (CBC) mode encryption routine.
    This mode XOR's the next block of plaintext with the previous block
    of cipher text to add confusion and diffusion.
    The initial vector (iv below) is a 64 bit block used to XOR the first 
    block of plaintext, and should be uniq for each message (but this is not
    a requirement). The IV can be sent in clear along with the ciphertext.
    
    Arguments:
        str    - The string of bytes to encrypt.
        slen   - The length of str in bytes.
        iv     - The 64 bit initial vector.
        bk     - The blowfish key received by bfish_keyinit().

void
bfish_cbc_decrypt(u_char *str, u_long slen, u_char *iv, struct bfish_key *bk)

    Cipher Block Chaining (CBC) mode decryption routine.
    
    Arguments:
        str    - The string of bytes (encrypted in CBC mode) to decrypt.
        slen   - The length of str in bytes.
        iv     - The 64 bit initial vector.
        bk     - The blowfish key received by bfish_keyinit().
        
void
bfish_ofb_encrypt(u_char *str, u_long slen, u_char *iv, u_char bsize, struct bfish_key *bk)

    Output FeedBack Mode encryption routine.
    This mode creates a Pseudo Random Number Generator by (re)encrypting the
    initial vector each round and XOR the leftmost subblock of this with the next
    subblock of the plaintext.
    Make sure that you use different initial vectors for the same key to avoid 
    generating the same keystream.
    
    Arguments:
        str   - The string of bytes to encrypt.
        slen  - The length of str in bytes.
        iv    - The eight byte long initial vector.
        bsize - The size of the subblock, 8, 16 or 32 bits.
                Note that the input array needs to be aligned to
                the number of bits used in the subblock.
        bk    - The blowfish key initialized by bfish_keyinit()
    
void
bfish_ofb_decrypt(u_char *str, u_long slen, u_char *iv, u_char bsize, struct bfish_key *bk)

    Output FeedBack Mode decryption routine.

    Arguments:
        str   - The string of bytes encrypted by bfish_ofb_encrypt().
        slen  - The length of str in bytes.
        iv    - The eight byte long initial vector used by bfish_ofb_encrypt().
        bsize - The size of the subblock used by bfish_ofb_encrypt().
        bk    - The blowfish key initialized by bfish_keyinit()

void
bfish_cfb_encrypt(u_char *str, u_long slen, u_char *iv, u_char bsize, struct bfish_key *bk)
	
	Cipher FeedBack Mode encryption routine.
	This mode uses the ciphertext to generate a new block and XOR's the leftmost
	subblock in this with the next subblock in the plaintext.
	Make sure that you use different initial vectors (that not necessarily needs
	to be secret) for the same key to avoid generating the same keystream.
	
	Arguments:
		str   - The string of bytes to encrypt.
		slen  - The length of str in bytes.
		iv    - The eight byte long initial vector.
		bsize - The size of the subblock, 8, 16 or 32 bits.
				Note that the input array needs to be aligned to
				the number of bits used in the subblock.
		bk    - The blowfish key initialized by bfish_keyinit()
		
void
bfish_cfb_decrypt(u_char *str, u_long slen, u_char *iv, u_char bsize, struct bfish_key *bk)
	
	Cipher FeedBack Mode decryption routine.

	Arguments:
		str   - The string of bytes encrypted by bfish_cfb_encrypt().
		slen  - The length of str in bytes.
		iv    - The eight byte long initial vector used by bfish_cfb_encrypt().
		bsize - The size of the subblock used by bfish_cfb_encrypt().
		bk    - The blowfish key initialized by bfish_keyinit()
		
	
