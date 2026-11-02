#include "HLTV/common/md5.h"

#include <stdio.h>
#include <string.h>

typedef struct MD5Context_t
{
	unsigned int	buf[4];
	unsigned int	bits[2];
	unsigned char	in[64];
} MD5Context_t;

#define F1( x, y, z )	( z ^ ( x & ( y ^ z ) ) )
#define F2( x, y, z )	F1( z, x, y )
#define F3( x, y, z )	( x ^ y ^ z )
#define F4( x, y, z )	( y ^ ( x | ~z ) )

#define MD5STEP( f, w, x, y, z, data, s ) \
	( w += f( x, y, z ) + ( data ), w = ( w << ( s ) ) | ( w >> ( 32 - ( s ) ) ), w += ( x ) )

static void MD5_Transform( unsigned int* buf, const unsigned int* in )
{
	unsigned int a;
	unsigned int b;
	unsigned int c;
	unsigned int d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP( F1, a, b, c, d, in[0] + 0xd76aa478u, 7 );
	MD5STEP( F1, d, a, b, c, in[1] + 0xe8c7b756u, 12 );
	MD5STEP( F1, c, d, a, b, in[2] + 0x242070dbu, 17 );
	MD5STEP( F1, b, c, d, a, in[3] + 0xc1bdceeeu, 22 );
	MD5STEP( F1, a, b, c, d, in[4] + 0xf57c0fafu, 7 );
	MD5STEP( F1, d, a, b, c, in[5] + 0x4787c62au, 12 );
	MD5STEP( F1, c, d, a, b, in[6] + 0xa8304613u, 17 );
	MD5STEP( F1, b, c, d, a, in[7] + 0xfd469501u, 22 );
	MD5STEP( F1, a, b, c, d, in[8] + 0x698098d8u, 7 );
	MD5STEP( F1, d, a, b, c, in[9] + 0x8b44f7afu, 12 );
	MD5STEP( F1, c, d, a, b, in[10] + 0xffff5bb1u, 17 );
	MD5STEP( F1, b, c, d, a, in[11] + 0x895cd7beu, 22 );
	MD5STEP( F1, a, b, c, d, in[12] + 0x6b901122u, 7 );
	MD5STEP( F1, d, a, b, c, in[13] + 0xfd987193u, 12 );
	MD5STEP( F1, c, d, a, b, in[14] + 0xa679438eu, 17 );
	MD5STEP( F1, b, c, d, a, in[15] + 0x49b40821u, 22 );

	MD5STEP( F2, a, b, c, d, in[1] + 0xf61e2562u, 5 );
	MD5STEP( F2, d, a, b, c, in[6] + 0xc040b340u, 9 );
	MD5STEP( F2, c, d, a, b, in[11] + 0x265e5a51u, 14 );
	MD5STEP( F2, b, c, d, a, in[0] + 0xe9b6c7aau, 20 );
	MD5STEP( F2, a, b, c, d, in[5] + 0xd62f105du, 5 );
	MD5STEP( F2, d, a, b, c, in[10] + 0x02441453u, 9 );
	MD5STEP( F2, c, d, a, b, in[15] + 0xd8a1e681u, 14 );
	MD5STEP( F2, b, c, d, a, in[4] + 0xe7d3fbc8u, 20 );
	MD5STEP( F2, a, b, c, d, in[9] + 0x21e1cde6u, 5 );
	MD5STEP( F2, d, a, b, c, in[14] + 0xc33707d6u, 9 );
	MD5STEP( F2, c, d, a, b, in[3] + 0xf4d50d87u, 14 );
	MD5STEP( F2, b, c, d, a, in[8] + 0x455a14edu, 20 );
	MD5STEP( F2, a, b, c, d, in[13] + 0xa9e3e905u, 5 );
	MD5STEP( F2, d, a, b, c, in[2] + 0xfcefa3f8u, 9 );
	MD5STEP( F2, c, d, a, b, in[7] + 0x676f02d9u, 14 );
	MD5STEP( F2, b, c, d, a, in[12] + 0x8d2a4c8au, 20 );

	MD5STEP( F3, a, b, c, d, in[5] + 0xfffa3942u, 4 );
	MD5STEP( F3, d, a, b, c, in[8] + 0x8771f681u, 11 );
	MD5STEP( F3, c, d, a, b, in[11] + 0x6d9d6122u, 16 );
	MD5STEP( F3, b, c, d, a, in[14] + 0xfde5380cu, 23 );
	MD5STEP( F3, a, b, c, d, in[1] + 0xa4beea44u, 4 );
	MD5STEP( F3, d, a, b, c, in[4] + 0x4bdecfa9u, 11 );
	MD5STEP( F3, c, d, a, b, in[7] + 0xf6bb4b60u, 16 );
	MD5STEP( F3, b, c, d, a, in[10] + 0xbebfbc70u, 23 );
	MD5STEP( F3, a, b, c, d, in[13] + 0x289b7ec6u, 4 );
	MD5STEP( F3, d, a, b, c, in[0] + 0xeaa127fau, 11 );
	MD5STEP( F3, c, d, a, b, in[3] + 0xd4ef3085u, 16 );
	MD5STEP( F3, b, c, d, a, in[6] + 0x04881d05u, 23 );
	MD5STEP( F3, a, b, c, d, in[9] + 0xd9d4d039u, 4 );
	MD5STEP( F3, d, a, b, c, in[12] + 0xe6db99e5u, 11 );
	MD5STEP( F3, c, d, a, b, in[15] + 0x1fa27cf8u, 16 );
	MD5STEP( F3, b, c, d, a, in[2] + 0xc4ac5665u, 23 );

	MD5STEP( F4, a, b, c, d, in[0] + 0xf4292244u, 6 );
	MD5STEP( F4, d, a, b, c, in[7] + 0x432aff97u, 10 );
	MD5STEP( F4, c, d, a, b, in[14] + 0xab9423a7u, 15 );
	MD5STEP( F4, b, c, d, a, in[5] + 0xfc93a039u, 21 );
	MD5STEP( F4, a, b, c, d, in[12] + 0x655b59c3u, 6 );
	MD5STEP( F4, d, a, b, c, in[3] + 0x8f0ccc92u, 10 );
	MD5STEP( F4, c, d, a, b, in[10] + 0xffeff47du, 15 );
	MD5STEP( F4, b, c, d, a, in[1] + 0x85845dd1u, 21 );
	MD5STEP( F4, a, b, c, d, in[8] + 0x6fa87e4fu, 6 );
	MD5STEP( F4, d, a, b, c, in[15] + 0xfe2ce6e0u, 10 );
	MD5STEP( F4, c, d, a, b, in[6] + 0xa3014314u, 15 );
	MD5STEP( F4, b, c, d, a, in[13] + 0x4e0811a1u, 21 );
	MD5STEP( F4, a, b, c, d, in[4] + 0xf7537e82u, 6 );
	MD5STEP( F4, d, a, b, c, in[11] + 0xbd3af235u, 10 );
	MD5STEP( F4, c, d, a, b, in[2] + 0x2ad7d2bbu, 15 );
	MD5STEP( F4, b, c, d, a, in[9] + 0xeb86d391u, 21 );

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

#undef MD5STEP
#undef F4
#undef F3
#undef F2
#undef F1

static void MD5_Init( MD5Context_t* ctx )
{
	ctx->buf[0] = 0x67452301u;
	ctx->buf[1] = 0xefcdab89u;
	ctx->buf[2] = 0x98badcfeu;
	ctx->buf[3] = 0x10325476u;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}

static void MD5_Update( MD5Context_t* ctx, const unsigned char* buf, unsigned int len )
{
	unsigned int t;
	unsigned char* p;

	t = ctx->bits[0];
	if ( ( ctx->bits[0] = t + ( len << 3 ) ) < t )
	{
		ctx->bits[1]++;
	}
	ctx->bits[1] += len >> 29;

	t = ( t >> 3 ) & 0x3f;
	if ( t )
	{
		p = ctx->in + t;
		t = 64 - t;
		if ( len < t )
		{
			memcpy( p, buf, len );
			return;
		}

		memcpy( p, buf, t );
		MD5_Transform( ctx->buf, (const unsigned int*)ctx->in );
		buf += t;
		len -= t;
	}

	while ( len >= 64 )
	{
		memcpy( ctx->in, buf, 64 );
		MD5_Transform( ctx->buf, (const unsigned int*)ctx->in );
		buf += 64;
		len -= 64;
	}

	memcpy( ctx->in, buf, len );
}

static void MD5_Final( unsigned char* digest, MD5Context_t* ctx )
{
	unsigned int count;
	unsigned char* p;

	count = ( ctx->bits[0] >> 3 ) & 0x3f;
	p = ctx->in + count;
	*p++ = 0x80;
	count = 0x3f - count;
	if ( count < 8 )
	{
		memset( p, 0, count );
		MD5_Transform( ctx->buf, (const unsigned int*)ctx->in );
		memset( ctx->in, 0, 56 );
	}
	else
	{
		memset( p, 0, count - 8 );
	}

	( (unsigned int*)ctx->in )[14] = ctx->bits[0];
	( (unsigned int*)ctx->in )[15] = ctx->bits[1];
	MD5_Transform( ctx->buf, (const unsigned int*)ctx->in );
	memcpy( digest, ctx->buf, 16 );
	memset( ctx, 0, sizeof( ctx ) );
}

extern "C" char* MD5_Print( unsigned char* hash )
{
	static char szReturn[64];
	char szChunk[10];
	unsigned int i;

	memset( szReturn, 0, sizeof( szReturn ) );
	for ( i = 0; i < 16; i++ )
	{
		sprintf( szChunk, "%02x", hash[i] );
		strcat( szReturn, szChunk );
	}

	return szReturn;
}

extern "C" char* MD5_GetCDKeyHash( const char* key )
{
	static char szHashedKeyBuffer[256];
	char szKeyBuffer[256];
	unsigned char digest[16];
	MD5Context_t ctx;
	unsigned int nKeyLength;

	nKeyLength = strlen( key );
	if ( nKeyLength - 1 > 0xFEu )
	{
		return NULL;
	}

	strncpy( szKeyBuffer, key, 0xFF );
	szKeyBuffer[255] = 0;
	szKeyBuffer[nKeyLength] = 0;

	memset( &ctx, 0, sizeof( ctx ) );
	memset( digest, 0, sizeof( digest ) );
	MD5_Init( &ctx );
	MD5_Update( &ctx, (const unsigned char*)szKeyBuffer, nKeyLength );
	MD5_Final( digest, &ctx );

	memset( szHashedKeyBuffer, 0, sizeof( szHashedKeyBuffer ) );
	strncpy( szHashedKeyBuffer, MD5_Print( digest ), 0xFF );
	szHashedKeyBuffer[255] = 0;
	return szHashedKeyBuffer;
}

extern "C" void MD5_Hash_Mem( unsigned char* digest, unsigned char* mem, int size )
{
	MD5Context_t ctx;

	memset( &ctx, 0, sizeof( ctx ) );
	MD5_Init( &ctx );
	MD5_Update( &ctx, mem, size );
	MD5_Final( digest, &ctx );
}
