#ifndef __MD5_HEADER__
#define __MD5_HEADER__

#include "CString.h"
#include <cstdint>

typedef unsigned char *POINTER;

/*
 * MD5 context.
 */
typedef struct
{
	uint32_t state[4];   	      /* state (ABCD), MD5 uses 32-bit words */
	uint32_t count[2]; 	      /* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64];	          /* input buffer */
} MD5_CTX;


/*
 * MD5 class
 */
class EIB_STD_EXPORT MD5
{
private:

	void MD5Transform (uint32_t state[4], unsigned char block[64]);
	void Encode (unsigned char*, uint32_t*, unsigned int);
	void Decode (uint32_t*, unsigned char*, unsigned int);
	void MD5_memcpy (POINTER, POINTER, unsigned int);
	void MD5_memset (POINTER, int, unsigned int);

public:
	void MD5Init (MD5_CTX*);
	void MD5Update (MD5_CTX*, unsigned char*, unsigned int);
	void MD5Final (unsigned char [16], MD5_CTX*);

	MD5(){};
	virtual ~MD5(){};
};

#endif
