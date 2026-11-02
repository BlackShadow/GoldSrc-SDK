#include "HLTV/common/Bitbuffer.h"

#include "HLTV/common/mem.h"
#include "HLTV/common/byteorder.h"

#include <stdlib.h>
#include <string.h>

#define BITBUFFER_BITS_PER_WORD	32
#define BITBUFFER_ALL_BITS		0xFFFFFFFFu

namespace
{
	unsigned int BitMask( int bit )
	{
		return bit == BITBUFFER_BITS_PER_WORD ? 0u : 1u << bit;
	}

	unsigned int LowBitMask( int count )
	{
		return count == BITBUFFER_BITS_PER_WORD ? BITBUFFER_ALL_BITS : BitMask( count ) - 1u;
	}
}

BitBuffer::BitBuffer()
{
	memset( this, 0, sizeof( BitBuffer ) );
}

BitBuffer::BitBuffer( void* newData, unsigned int size )
{
	memset( this, 0, sizeof( BitBuffer ) );
	SetBuffer( newData, (int)size );
}

BitBuffer::BitBuffer( unsigned int size )
{
	memset( this, 0, sizeof( BitBuffer ) );
	Resize( size );
}

BitBuffer::~BitBuffer()
{
#ifdef _WIN32
	Free();
#else
	if ( data != NULL && ownData )
		free( data );
#endif
}

bool BitBuffer::Resize( unsigned int size )
{
	unsigned char* block;

	if ( data != NULL && ownData )
		free( data );

	maxSize = 0;
	data = NULL;
	currentByte = NULL;
	currentBit = 0;
	ownData = false;
	sizeError = false;
	littleEndian = true;

	block = (unsigned char*)Mem_ZeroMalloc( size + 4 );
	data = block;
	if ( block != NULL )
	{
		currentByte = block;
		maxSize = (int)size;
		currentBit = 0;
		sizeError = false;
		ownData = true;
		littleEndian = true;
		return true;
	}

	maxSize = 0;
	currentByte = NULL;
	currentBit = 0;
	sizeError = false;
	ownData = false;
	return false;
}

void BitBuffer::Clear()
{
	memset( data, 0, (size_t)maxSize );
	currentBit = 0;
	sizeError = false;
	littleEndian = true;
	currentByte = data;
}

int BitBuffer::CurrentBit()
{
	return currentBit + 8 * (int)( currentByte - data );
}

void BitBuffer::Reset()
{
	currentBit = 0;
	sizeError = false;
	littleEndian = true;
	currentByte = data;
}

void BitBuffer::Free()
{
	if ( data != NULL && ownData )
		free( data );

	maxSize = 0;
	data = NULL;
	currentByte = NULL;
	currentBit = 0;
	ownData = false;
	sizeError = false;
	littleEndian = true;
}

int BitBuffer::ReadBit()
{
	unsigned char* cursor;
	int bit;

	cursor = currentByte;
	if ( (int)( cursor - data ) >= maxSize )
	{
		sizeError = true;
		return -1;
	}

	bit = currentBit;
	if ( littleEndian )
	{
		if ( bit == 7 )
		{
			currentBit = 0;
			currentByte = cursor + 1;
			return *cursor >> 7;
		}

		currentBit = bit + 1;
		return ( *cursor >> bit ) & 1;
	}

	if ( bit == 7 )
	{
		currentBit = 0;
		currentByte = cursor + 1;
		return *cursor & 1;
	}

	currentBit = bit + 1;
	return ( *cursor >> ( 7 - bit ) ) & 1;
}

unsigned int BitBuffer::ReadBits( int n )
{
	unsigned int value;
	int bit;
	int newBit;
	unsigned int first;
	unsigned int second;

	if ( littleEndian )
	{
		if ( (int)( currentByte - data ) >= maxSize )
		{
			sizeError = true;
			return (unsigned int)-1;
		}

		bit = currentBit;
		if ( n + bit > 32 )
		{
			first = *(unsigned int*)currentByte;
			currentByte += 4;
			newBit = ( n + bit ) & 7;
			second = *(unsigned int*)currentByte;
			currentBit = newBit;
			return ( first >> bit ) | ( ( LowBitMask( newBit ) & second ) << ( 32 - bit ) );
		}

		value = LowBitMask( n ) & ( *(unsigned int*)currentByte >> bit );
		newBit = bit + ( n & 7 );
		currentByte += n >> 3;
		if ( newBit <= 7 )
			currentBit = newBit;
		else
		{
			currentBit = newBit & 7;
			++currentByte;
		}

		return value;
	}

	value = 0;
	while ( n-- > 0 )
	{
		if ( ReadBit() )
			value |= 1u << n;
	}

	return value;
}

unsigned int BitBuffer::PeekBits( int numbits )
{
	int oldcurrentBit;
	unsigned char* oldcurrentByte;
	unsigned int bits;
	int n;
	int bit;
	unsigned int first;
	unsigned int second;
	int newBit;

	oldcurrentBit = currentBit;
	oldcurrentByte = currentByte;
	if ( !littleEndian )
	{
		bits = 0;
		n = numbits;
		while ( n-- > 0 )
		{
			if ( ReadBit() )
				bits |= 1u << n;
		}

		currentByte = oldcurrentByte;
		currentBit = oldcurrentBit;
		return bits;
	}

	if ( (int)( oldcurrentByte - data ) >= maxSize )
	{
		sizeError = true;
		currentByte = oldcurrentByte;
		currentBit = oldcurrentBit;
		return (unsigned int)-1;
	}

	bit = oldcurrentBit;
	if ( numbits + bit > 32 )
	{
		first = *(unsigned int*)oldcurrentByte;
		newBit = ( numbits + bit ) & 7;
		second = *( (unsigned int*)oldcurrentByte + 1 );
		bits = ( first >> bit ) | ( ( LowBitMask( newBit ) & second ) << ( 32 - bit ) );
	}
	else
		bits = LowBitMask( numbits ) & ( *(unsigned int*)oldcurrentByte >> bit );

	currentByte = oldcurrentByte;
	currentBit = oldcurrentBit;
	return bits;
}

int BitBuffer::ReadChar()
{
	return (int)ReadBits( 8 );
}

int BitBuffer::ReadByte()
{
	return (int)ReadBits( 8 );
}

int BitBuffer::ReadShort()
{
	return (int)ReadBits( 16 );
}

int BitBuffer::ReadWord()
{
	return (int)ReadBits( 16 );
}

unsigned int BitBuffer::ReadLong()
{
	return ReadBits( 32 );
}

float BitBuffer::ReadFloat()
{
	union
	{
		float	f;
		int		l;
	} dat;

	dat.l = LittleLong( (int)ReadLong() );
	return dat.f;
}

bool BitBuffer::ReadBuf( int iSize, void* pbuf )
{
	unsigned char* out;
	int done;

	if ( iSize + (int)( currentByte - data ) > maxSize )
	{
		sizeError = true;
		return false;
	}

	if ( currentBit == 0 )
	{
		memcpy( pbuf, currentByte, (size_t)iSize );
		currentByte += iSize;
		return true;
	}

	out = (unsigned char*)pbuf;
	done = 0;
	if ( iSize > 4 )
	{
		unsigned int groups;
		int dwords;
		int i;

		groups = (unsigned int)( iSize - 5 ) >> 2;
		dwords = (int)groups + 1;
		for ( i = 0; i < dwords; ++i )
		{
			*(unsigned int*)( out + done ) = ReadLong();
			done += 4;
		}
	}

	while ( done < iSize )
	{
		out[done] = (unsigned char)ReadByte();
		++done;
	}

	return true;
}

void BitBuffer::WriteBuf( BitBuffer* buf, int length )
{
	unsigned char* src;

	src = buf->currentByte;
	if ( src != NULL && !sizeError && length != 0 )
	{
		if ( length + (int)( currentByte - data ) > maxSize )
			sizeError = true;
		else if ( currentBit != 0 )
			WriteBuf( (const void*)src, length );
		else
		{
			memcpy( currentByte, src, (size_t)length );
			currentByte += length;
		}

		src = buf->currentByte;
	}

	if ( length + (int)( src - buf->data ) > buf->maxSize )
		buf->sizeError = true;

	buf->currentByte = src + length;
}

char* BitBuffer::ReadString()
{
	static char string[8192];
	int l;
	int c;

	l = 0;
	while ( 1 )
	{
		c = ReadByte();
		if ( (unsigned int)( c + 1 ) > 1u )
		{
			string[l++] = (char)c;
			if ( l != 0x1FFF )
				continue;
		}

		break;
	}

	string[l] = 0;
	return string;
}

char* BitBuffer::ReadStringLine()
{
	static char string[2048];
	int l;
	int c;

	l = 0;
	while ( 1 )
	{
		c = ReadByte();
		if ( (unsigned int)( c + 1 ) > 1u && c != '\n' )
		{
			string[l++] = (char)c;
			if ( l != 2047 )
				continue;
		}

		break;
	}

	string[l] = 0;
	return string;
}

float BitBuffer::ReadAngle()
{
	return (float)ReadByte() * ( 360.0f / 256.0f );
}

float BitBuffer::ReadHiresAngle()
{
	return (float)ReadShort() * ( 360.0f / 65536.0f );
}

void BitBuffer::WriteBit( int c )
{
	static const unsigned char masks[8] =
	{
		0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
	};
	static const unsigned char inv_masks[8] =
	{
		0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE
	};
	int bit;

	if ( (int)( currentByte - data ) >= maxSize )
	{
		sizeError = true;
		return;
	}

	bit = currentBit;
	if ( littleEndian )
	{
		if ( bit == 7 )
		{
			if ( c != 0 )
				*currentByte = (unsigned char)( *currentByte | 0x80 );
			else
				*currentByte = (unsigned char)( *currentByte & 0x7F );

			++currentByte;
			currentBit = 0;
		}
		else
		{
			currentBit = bit + 1;
			if ( c != 0 )
				*currentByte = (unsigned char)( *currentByte | BitMask( bit ) );
			else
				*currentByte = (unsigned char)( *currentByte & ~BitMask( bit ) );
		}

		return;
	}

	currentBit = bit + 1;
	if ( c != 0 )
		*currentByte = (unsigned char)( *currentByte | masks[bit] );
	else
		*currentByte = (unsigned char)( *currentByte & inv_masks[bit] );

	if ( currentBit == 8 )
	{
		currentBit = 0;
		++currentByte;
	}
}

void BitBuffer::WriteBits( unsigned int bits, int n )
{
	unsigned int value;
	int bit;
	int newBit;

	if ( sizeError )
		return;

	if ( littleEndian )
	{
		if ( (int)( currentByte - data ) + ( n >> 8 ) > maxSize )
		{
			sizeError = true;
			return;
		}

		value = LowBitMask( n ) & bits;
		bit = currentBit;
		if ( n + bit > 32 )
		{
			*(unsigned int*)currentByte |= value << bit;
			currentBit = ( n + bit ) & 7;
			*(unsigned int*)( currentByte + 4 ) |= value >> ( 32 - bit );
			currentByte += 4;
		}
		else
		{
			*(unsigned int*)currentByte |= value << bit;
			newBit = bit + ( n & 7 );
			currentByte += n >> 3;
			if ( newBit > 7 )
			{
				currentBit = newBit & 7;
				++currentByte;
			}
			else
				currentBit = newBit;
		}

		return;
	}

	if ( n <= 31 && bits >= ( 1u << n ) && bits != BITBUFFER_ALL_BITS )
		bits = ( 1u << n ) - 1;

	while ( n-- > 0 )
		WriteBit( ( bits & ( 1u << n ) ) != 0 );
}

void BitBuffer::WriteSBits( int bits, int numbits )
{
	int value;
	int limit;

	value = bits;
	if ( numbits <= 31 )
	{
		limit = ( 1 << ( numbits - 1 ) ) - 1;
		if ( value > limit )
			value = limit;
		else if ( value < -limit )
			value = -limit;
	}

	WriteBit( value < 0 );
	if ( sizeError )
		return;

	WriteBits( (unsigned int)abs( value ), numbits - 1 );
}

void BitBuffer::WriteChar( int c )
{
	WriteBits( (unsigned int)c, 8 );
}

void BitBuffer::WriteByte( int c )
{
	WriteBits( (unsigned int)c, 8 );
}

void BitBuffer::WriteShort( int c )
{
	WriteBits( (unsigned int)c, 16 );
}

void BitBuffer::WriteWord( int c )
{
	WriteBits( (unsigned int)c, 16 );
}

void BitBuffer::WriteLong( unsigned int c )
{
	WriteBits( c, 32 );
}

void BitBuffer::WriteFloat( float f )
{
	union
	{
		float	f;
		int		l;
	} dat;

	dat.f = f;
	WriteLong( (unsigned int)LittleLong( dat.l ) );
}

void BitBuffer::WriteString( const char* s )
{
	size_t n;
	unsigned int total;

	if ( s != NULL )
	{
		n = strlen( s );
		total = (unsigned int)n + 1;
		if ( n == (size_t)-1 || sizeError )
			return;

		if ( (int)( total + currentByte - data ) <= maxSize )
		{
			if ( currentBit != 0 )
				WriteBuf( s, (int)total );
			else
			{
				memcpy( currentByte, s, total );
				currentByte += total;
			}

			return;
		}

		sizeError = true;
		return;
	}

	WriteByte( 0 );
}

void BitBuffer::WriteBuf( const void* buf, int iSize )
{
	const unsigned char* in;
	int done;

	if ( buf == NULL || sizeError || iSize == 0 )
		return;

	if ( iSize + (int)( currentByte - data ) > maxSize )
	{
		sizeError = true;
		return;
	}

	if ( currentBit == 0 )
	{
		memcpy( currentByte, buf, (size_t)iSize );
		currentByte += iSize;
		return;
	}

	in = (const unsigned char*)buf;
	done = 0;
	if ( iSize > 4 )
	{
		unsigned int groups;
		int dwords;
		int i;

		groups = (unsigned int)( iSize - 5 ) >> 2;
		dwords = (int)groups + 1;
		for ( i = 0; i < dwords; ++i )
		{
			if ( !sizeError )
				WriteLong( *(const unsigned int*)( in + done ) );

			done += 4;
		}
	}

	while ( done < iSize )
	{
		if ( !sizeError )
			WriteByte( in[done] );

		++done;
	}
}

void BitBuffer::WriteBitData( void* src, int length )
{
	int i;
	unsigned char* p;

	p = (unsigned char*)src;
	for ( i = 0; i < length; ++i )
	{
		if ( !sizeError )
			WriteByte( p[i] );
	}
}

void BitBuffer::WriteAngle( float f )
{
	int c;

	if ( sizeError )
		return;

	c = (int)( f * 256.0f / 360.0f );
	WriteByte( (unsigned char)c );
}

void BitBuffer::WriteHiresAngle( float f )
{
	int c;

	if ( sizeError )
		return;

	c = (int)( f * 65536.0f / 360.0f );
	WriteShort( (unsigned short)c );
}

int BitBuffer::CurrentSize()
{
	return ( currentBit != 0 ) + (int)( currentByte - data );
}

int BitBuffer::SpaceLeft()
{
	return maxSize - (int)( currentByte - data );
}

void BitBuffer::AlignByte()
{
	if ( currentBit != 0 )
	{
		++currentByte;
		currentBit = 0;
	}
}

int BitBuffer::ReadSBits( int numbits )
{
	int sign;
	int value;

	sign = ReadBit();
	value = (int)ReadBits( numbits - 1 );
	if ( sign != 0 )
		return -value;

	return value;
}

float BitBuffer::ReadBitAngle( int numbits )
{
	unsigned int ui;
	float shift;

	ui = ReadBits( numbits );
	shift = (float)( 1 << numbits );
	return (float)ui * (float)( 360.0 / shift );
}

void BitBuffer::WriteBitAngle( float fAngle, int numbits )
{
	int shift;
	int d;
	unsigned int mask;

	if ( numbits > 31 )
	{
		sizeError = true;
		return;
	}

	if ( sizeError )
		return;

	shift = 1 << numbits;
	d = (int)( (float)(unsigned int)shift * fAngle ) / 360;
	mask = (unsigned int)shift - 1u;
	WriteBits( mask & (unsigned int)d, numbits );
}

char* BitBuffer::ReadBitString()
{
	static char buf[8192];
	char* p;
	int c;

	buf[0] = 0;
	p = buf;
	do
	{
		c = ReadByte();
		*p++ = (char)c;
	}
	while ( c != 0 );

	return buf;
}

void BitBuffer::WriteBitString( const char* s )
{
	const char* pch;

	pch = s;
	while ( *pch != 0 )
	{
		if ( !sizeError )
			WriteByte( *pch );

		++pch;
	}

	if ( !sizeError )
		WriteByte( 0 );
}

void BitBuffer::StartBitMode()
{
	if ( currentBit != 0 )
		sizeError = true;
}

void BitBuffer::EndBitMode()
{
	if ( currentBit != 0 )
	{
		++currentByte;
		currentBit = 0;
	}
}

int BitBuffer::ReadBitData( void* dest, int length )
{
	int i;
	int readlen;
	unsigned char* p;

	p = (unsigned char*)dest;
	readlen = length;
	for ( i = 0; i < length; ++i )
		p[i] = (unsigned char)ReadByte();

	return readlen;
}

void BitBuffer::SetBuffer( void* buffer, int size )
{
	if ( data != NULL && ownData )
		free( data );

	currentBit = 0;
	ownData = false;
	sizeError = false;
	littleEndian = true;
	data = (unsigned char*)buffer;
	maxSize = size;
	currentByte = (unsigned char*)buffer;
}

float BitBuffer::ReadBitCoord()
{
	int intval;
	int fractval;
	int signbit;
	float value;

	intval = ReadBit();
	fractval = ReadBit();
	if ( ( intval | fractval ) == 0 )
		return 0.0f;

	signbit = ReadBit();
	value = 0.0f;
	if ( intval != 0 )
		value = (float)(int)ReadBits( 12 );
	if ( fractval != 0 )
		value += (float)(int)ReadBits( 3 ) * 0.125f;
	if ( signbit != 0 )
		return -value;

	return value;
}

void BitBuffer::ReadBitVec3Coord( float* fa )
{
	int xflag;
	int yflag;
	int zflag;

	xflag = ReadBit();
	yflag = ReadBit();
	zflag = ReadBit();
	if ( xflag != 0 )
		fa[0] = ReadBitCoord();
	if ( yflag != 0 )
		fa[1] = ReadBitCoord();
	if ( zflag != 0 )
		fa[2] = ReadBitCoord();
}

float BitBuffer::ReadCoord()
{
	float f;

	f = (float)ReadShort() * 0.125f;
	return f;
}

void BitBuffer::SkipBytes( int n )
{
	unsigned char* cursor;

	cursor = currentByte;
	if ( n + (int)( cursor - data ) > maxSize )
		sizeError = true;

	currentByte = cursor + n;
}

void BitBuffer::SkipBits( int n )
{
	int left;
	int bit;
	int newBit;

	if ( littleEndian )
	{
		if ( (int)( currentByte - data ) >= maxSize )
		{
			sizeError = true;
			return;
		}

		bit = currentBit;
		if ( n + bit > 32 )
		{
			currentByte += 4;
			currentBit = ( n + bit ) & 7;
			return;
		}

		newBit = bit + ( n & 7 );
		currentByte += n >> 3;
		if ( newBit > 7 )
		{
			currentBit = newBit & 7;
			++currentByte;
		}
		else
			currentBit = newBit;

		return;
	}

	if ( n <= 0 )
		return;

	left = n - 1;
	bit = currentBit;
	do
	{
		while ( bit != 7 )
		{
			--left;
			++bit;
			if ( left == -1 )
				goto done;
		}

		--left;
		bit = 0;
		++currentByte;
	}
	while ( left != -1 );

done:
	currentBit = bit;
}

int BitBuffer::SkipString()
{
	int l;
	int c;

	l = 1;
	while ( 1 )
	{
		c = ReadByte();
		if ( c == -1 || c == 0 )
			return l;
		if ( ++l == 0x2000 )
			return 0x2000;
	}
}

void BitBuffer::FastClear()
{
	int count;

	count = ( currentBit != 0 ) + (int)( currentByte - data ) + 4;
	if ( count > maxSize )
		count = maxSize;

	memset( data, 0, (size_t)count );
	currentBit = 0;
	sizeError = false;
	littleEndian = true;
	currentByte = data;
}

void BitBuffer::ConcatBuffer( BitBuffer* buffer )
{
	unsigned int length;

	length = (unsigned int)( ( buffer->currentBit != 0 ) + (int)( buffer->currentByte - buffer->data ) );
	if ( buffer->data != NULL && !sizeError && length != 0 )
	{
		if ( (int)( length + currentByte - data ) > maxSize )
			sizeError = true;
		else if ( currentBit != 0 )
			WriteBuf( buffer->data, (int)length );
		else
		{
			memcpy( currentByte, buffer->data, length );
			currentByte += length;
		}
	}
}

void BitBuffer::WriteCoord( float f )
{
	int c;

	c = (int)( f * 8.0f );
	WriteShort( c );
}
