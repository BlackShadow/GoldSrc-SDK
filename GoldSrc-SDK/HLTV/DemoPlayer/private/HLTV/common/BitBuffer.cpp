#include "HLTV/common/Bitbuffer.h"
#include "HLTV/common/byteorder.h"
#include "HLTV/common/mem.h"

#include <stdlib.h>
#include <string.h>

#define kBitsPerByte		8
#define kByteBitMask		( kBitsPerByte - 1 )
#define kByteShift			3
#define kBitsPerWord		16
#define kBitsPerLong		32
#define kLongBytes			( kBitsPerLong / kBitsPerByte )

#define kWriteSizeCheckShift	8

#define kFullCircle			360.0f
#define kCircleDegrees		360
#define kByteAngleScale		256.0f
#define kHiresAngleScale	65536.0f

#define kCoordScale			8.0f
#define kCoordResolution	0.125f
#define kCoordIntegerBits	12
#define kCoordFractionBits	3

#define kStringBufferSize	8192
#define kStringLineSize		2048
#define kAllocSlack			kLongBytes

static const unsigned int BITTABLE[kBitsPerLong + 1] =
{
	0x00000001u, 0x00000002u, 0x00000004u, 0x00000008u,
	0x00000010u, 0x00000020u, 0x00000040u, 0x00000080u,
	0x00000100u, 0x00000200u, 0x00000400u, 0x00000800u,
	0x00001000u, 0x00002000u, 0x00004000u, 0x00008000u,
	0x00010000u, 0x00020000u, 0x00040000u, 0x00080000u,
	0x00100000u, 0x00200000u, 0x00400000u, 0x00800000u,
	0x01000000u, 0x02000000u, 0x04000000u, 0x08000000u,
	0x10000000u, 0x20000000u, 0x40000000u, 0x80000000u,
	0x00000000u
};

static const unsigned int INVBITTABLE[kBitsPerLong + 1] =
{
	0xFFFFFFFEu, 0xFFFFFFFDu, 0xFFFFFFFBu, 0xFFFFFFF7u,
	0xFFFFFFEFu, 0xFFFFFFDFu, 0xFFFFFFBFu, 0xFFFFFF7Fu,
	0xFFFFFEFFu, 0xFFFFFDFFu, 0xFFFFFBFFu, 0xFFFFF7FFu,
	0xFFFFEFFFu, 0xFFFFDFFFu, 0xFFFFBFFFu, 0xFFFF7FFFu,
	0xFFFEFFFFu, 0xFFFDFFFFu, 0xFFFBFFFFu, 0xFFF7FFFFu,
	0xFFEFFFFFu, 0xFFDFFFFFu, 0xFFBFFFFFu, 0xFF7FFFFFu,
	0xFEFFFFFFu, 0xFDFFFFFFu, 0xFBFFFFFFu, 0xF7FFFFFFu,
	0xEFFFFFFFu, 0xDFFFFFFFu, 0xBFFFFFFFu, 0x7FFFFFFFu,
	0xFFFFFFFFu
};

static const unsigned int ROWBITTABLE[kBitsPerLong + 1] =
{
	0x00000000u, 0x00000001u, 0x00000003u, 0x00000007u,
	0x0000000Fu, 0x0000001Fu, 0x0000003Fu, 0x0000007Fu,
	0x000000FFu, 0x000001FFu, 0x000003FFu, 0x000007FFu,
	0x00000FFFu, 0x00001FFFu, 0x00003FFFu, 0x00007FFFu,
	0x0000FFFFu, 0x0001FFFFu, 0x0003FFFFu, 0x0007FFFFu,
	0x000FFFFFu, 0x001FFFFFu, 0x003FFFFFu, 0x007FFFFFu,
	0x00FFFFFFu, 0x01FFFFFFu, 0x03FFFFFFu, 0x07FFFFFFu,
	0x0FFFFFFFu, 0x1FFFFFFFu, 0x3FFFFFFFu, 0x7FFFFFFFu,
	0xFFFFFFFFu
};

static unsigned int ReadUnalignedLong( const unsigned char* source )
{
	unsigned int value;
	memcpy( &value, source, sizeof( value ) );
	return value;
}

static void OrUnalignedLong( unsigned char* destination, unsigned int bits )
{
	const unsigned int value = ReadUnalignedLong( destination ) | bits;
	memcpy( destination, &value, sizeof( value ) );
}

BitBuffer::BitBuffer()
{
	memset( this, 0, sizeof( BitBuffer ) );
}

BitBuffer::BitBuffer( unsigned int size )
{
	memset( this, 0, sizeof( BitBuffer ) );
	Resize( size );
}

BitBuffer::BitBuffer( void* newData, unsigned int size )
{
	memset( this, 0, sizeof( BitBuffer ) );
	SetBuffer( newData, (int)size );
}

BitBuffer::~BitBuffer()
{
#if defined( _WIN32 )
	Free();
#else
	if ( data != NULL && ownData )
	{
		free( data );
	}
#endif
}

bool BitBuffer::Resize( unsigned int size )
{
	unsigned char* block;

	if ( data != NULL && ownData )
	{
		free( data );
	}

	maxSize = 0;
	data = NULL;
	currentByte = NULL;
	currentBit = 0;
	ownData = false;
	sizeError = false;
	littleEndian = true;

	block = (unsigned char*)Mem_ZeroMalloc( size + kAllocSlack );
	data = block;

	if ( block != NULL )
	{
		maxSize = (int)size;
		currentByte = block;
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
	memset( data, 0, maxSize );
	currentBit = 0;
	sizeError = false;
	littleEndian = true;
	currentByte = data;
}

void BitBuffer::FastClear()
{
	int count;

	count = CurrentSize() + kAllocSlack;

	if ( count > maxSize )
	{
		count = maxSize;
	}

	memset( data, 0, count );
	currentBit = 0;
	sizeError = false;
	littleEndian = true;
	currentByte = data;
}

int BitBuffer::CurrentBit()
{
	return currentBit + kBitsPerByte * (int)( currentByte - data );
}

int BitBuffer::CurrentSize()
{
	int size;

	size = (int)( currentByte - data );

	if ( currentBit != 0 )
	{
		size++;
	}

	return size;
}

int BitBuffer::SpaceLeft()
{
	return maxSize - (int)( currentByte - data );
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
	{
		free( data );
	}

	maxSize = 0;
	data = NULL;
	currentByte = NULL;
	currentBit = 0;
	ownData = false;
	sizeError = false;
	littleEndian = true;
}

void BitBuffer::SetBuffer( void* buffer, int size )
{
	if ( data != NULL && ownData )
	{
		free( data );
	}

	currentBit = 0;
	ownData = false;
	sizeError = false;
	littleEndian = true;
	data = (unsigned char*)buffer;
	maxSize = size;
	currentByte = (unsigned char*)buffer;
}

void BitBuffer::AlignByte()
{
	if ( currentBit != 0 )
	{
		currentByte++;
		currentBit = 0;
	}
}

void BitBuffer::StartBitMode()
{
	if ( currentBit != 0 )
	{
		sizeError = true;
	}
}

void BitBuffer::EndBitMode()
{
	if ( currentBit != 0 )
	{
		currentByte++;
		currentBit = 0;
	}
}

void BitBuffer::SkipBytes( int n )
{
	if ( (int)( currentByte - data ) + n > maxSize )
	{
		sizeError = true;
	}

	currentByte += n;
}

void BitBuffer::SkipBits( int n )
{
	int bit;
	int next;

	if ( littleEndian )
	{
		if ( currentByte - data >= maxSize )
		{
			sizeError = true;
			return;
		}

		bit = currentBit;

		if ( n + bit > kBitsPerLong )
		{
			currentByte += kLongBytes;
			currentBit = ( n + bit ) & kByteBitMask;
			return;
		}

		next = bit + ( n & kByteBitMask );
		currentByte += n >> kByteShift;

		if ( next > kByteBitMask )
		{
			currentBit = next & kByteBitMask;
			currentByte++;
		}
		else
		{
			currentBit = next;
		}

		return;
	}

	while ( n-- > 0 )
	{
		if ( currentBit == kByteBitMask )
		{
			currentBit = 0;
			currentByte++;
		}
		else
		{
			currentBit++;
		}
	}
}

int BitBuffer::ReadBit()
{
	int bit;
	unsigned char value;

	if ( currentByte - data >= maxSize )
	{
		sizeError = true;
		return -1;
	}

	value = *currentByte;

	if ( littleEndian )
	{
		if ( currentBit == kByteBitMask )
		{
			currentBit = 0;
			currentByte++;
			return value >> kByteBitMask;
		}

		bit = ( value >> currentBit ) & 1;
		currentBit++;
		return bit;
	}

	if ( currentBit == kByteBitMask )
	{
		currentBit = 0;
		currentByte++;
		return value & 1;
	}

	bit = ( value >> ( kByteBitMask - currentBit ) ) & 1;
	currentBit++;
	return bit;
}

void BitBuffer::WriteBit( int c )
{
	static const unsigned char inv_masks[kBitsPerByte] = { 0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE };
	static const unsigned char masks[kBitsPerByte] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
	int bit;

	if ( currentByte - data >= maxSize )
	{
		sizeError = true;
		return;
	}

	bit = currentBit;

	if ( littleEndian )
	{
		if ( bit == kByteBitMask )
		{
			if ( c != 0 )
			{
				*currentByte |= 0x80;
			}
			else
			{
				*currentByte &= 0x7F;
			}

			currentByte++;
			currentBit = 0;
		}
		else
		{
			currentBit = bit + 1;

			if ( c != 0 )
			{
				*currentByte |= (unsigned char)BITTABLE[bit];
			}
			else
			{
				*currentByte &= (unsigned char)INVBITTABLE[bit];
			}
		}

		return;
	}

	currentBit = bit + 1;

	if ( c != 0 )
	{
		*currentByte |= masks[bit];
	}
	else
	{
		*currentByte &= inv_masks[bit];
	}

	if ( currentBit == kBitsPerByte )
	{
		currentByte++;
		currentBit = 0;
	}
}

unsigned int BitBuffer::ReadBits( int n )
{
	unsigned int first;
	unsigned int second;
	unsigned int value;
	int bit;
	int next;
	unsigned int result;

	if ( littleEndian )
	{
		if ( currentByte - data >= maxSize )
		{
			sizeError = true;
			return (unsigned int)-1;
		}

		bit = currentBit;

		if ( n + bit > kBitsPerLong )
		{
			first = ReadUnalignedLong( currentByte );
			second = ReadUnalignedLong( currentByte + kLongBytes );
			next = ( n + bit ) & kByteBitMask;
			currentByte += kLongBytes;
			currentBit = next;
			return ( first >> bit ) | ( ( ROWBITTABLE[next] & second ) << ( kBitsPerLong - bit ) );
		}

		value = ROWBITTABLE[n] & ( ReadUnalignedLong( currentByte ) >> bit );
		next = bit + ( n & kByteBitMask );
		currentByte += n >> kByteShift;

		if ( next > kByteBitMask )
		{
			currentBit = next & kByteBitMask;
			currentByte++;
		}
		else
		{
			currentBit = next;
		}

		return value;
	}

	result = 0;

	while ( n-- > 0 )
	{
		if ( ReadBit() )
		{
			result |= 1u << n;
		}
	}

	return result;
}

unsigned int BitBuffer::PeekBits( int numbits )
{
	int oldBit;
	unsigned char* oldByte;
	unsigned int value;

	oldBit = currentBit;
	oldByte = currentByte;
	value = ReadBits( numbits );
	currentByte = oldByte;
	currentBit = oldBit;
	return value;
}

void BitBuffer::WriteBits( unsigned int bits, int n )
{
	unsigned int value;
	int bit;
	int next;

	if ( sizeError )
	{
		return;
	}

	if ( littleEndian )
	{
		if ( (int)( currentByte - data ) + ( n >> kWriteSizeCheckShift ) > maxSize )
		{
			sizeError = true;
			return;
		}

		value = bits & ROWBITTABLE[n];
		bit = currentBit;

		if ( n + bit > kBitsPerLong )
		{
			OrUnalignedLong( currentByte, value << bit );
			OrUnalignedLong( currentByte + kLongBytes, value >> ( kBitsPerLong - bit ) );
			currentBit = ( n + bit ) & kByteBitMask;
			currentByte += kLongBytes;
			return;
		}

		OrUnalignedLong( currentByte, value << bit );
		next = currentBit + ( n & kByteBitMask );
		currentByte += n >> kByteShift;

		if ( next > kByteBitMask )
		{
			currentBit = next & kByteBitMask;
			currentByte++;
		}
		else
		{
			currentBit = next;
		}

		return;
	}

	if ( n < kBitsPerLong && bits >= ( 1u << n ) && bits != 0xFFFFFFFFu )
	{
		bits = ( 1u << n ) - 1;
	}

	while ( n-- > 0 )
	{
		WriteBit( (int)( ( bits >> n ) & 1u ) );
	}
}

void BitBuffer::WriteSBits( int bits, int numbits )
{
	int limit;
	unsigned int magnitude;

	if ( numbits < kBitsPerLong )
	{
		limit = ( 1 << ( numbits - 1 ) ) - 1;

		if ( bits > limit )
		{
			bits = limit;
		}
		else if ( bits < -limit )
		{
			bits = -limit;
		}
	}

	WriteBit( bits < 0 );
	magnitude = (unsigned int)( bits < 0 ? -bits : bits );
	WriteBits( magnitude, numbits - 1 );
}

int BitBuffer::ReadSBits( int numbits )
{
	int sign;
	int value;

	sign = ReadBit();
	value = (int)ReadBits( numbits - 1 );

	if ( sign )
	{
		value = -value;
	}

	return value;
}

int BitBuffer::ReadChar()
{
	return (int)ReadBits( kBitsPerByte );
}

int BitBuffer::ReadByte()
{
	return (int)ReadBits( kBitsPerByte );
}

int BitBuffer::ReadShort()
{
	return (int)ReadBits( kBitsPerWord );
}

int BitBuffer::ReadWord()
{
	return (int)ReadBits( kBitsPerWord );
}

unsigned int BitBuffer::ReadLong()
{
	return ReadBits( kBitsPerLong );
}

float BitBuffer::ReadFloat()
{
	int bits;
	float value;

	bits = LittleLong( (int)ReadLong() );
	memcpy( &value, &bits, sizeof( value ) );
	return value;
}

void BitBuffer::WriteChar( int c )
{
	WriteBits( (unsigned int)c, kBitsPerByte );
}

void BitBuffer::WriteByte( int c )
{
	WriteBits( (unsigned int)c, kBitsPerByte );
}

void BitBuffer::WriteShort( int c )
{
	WriteBits( (unsigned int)c, kBitsPerWord );
}

void BitBuffer::WriteWord( int c )
{
	WriteBits( (unsigned int)c, kBitsPerWord );
}

void BitBuffer::WriteLong( unsigned int c )
{
	WriteBits( c, kBitsPerLong );
}

void BitBuffer::WriteFloat( float f )
{
	int bits;

	memcpy( &bits, &f, sizeof( bits ) );
	WriteLong( (unsigned int)LittleLong( bits ) );
}

bool BitBuffer::ReadBuf( int iSize, void* pbuf )
{
	unsigned char* out;
	int offset;
	unsigned int value;

	if ( iSize + (int)( currentByte - data ) > maxSize )
	{
		sizeError = true;
		return false;
	}

	out = (unsigned char*)pbuf;

	if ( currentBit != 0 )
	{
		offset = 0;

		while ( iSize - offset > kLongBytes )
		{
			value = ReadLong();
			memcpy( out + offset, &value, sizeof( value ) );
			offset += kLongBytes;
		}

		while ( offset < iSize )
		{
			out[offset] = (unsigned char)ReadByte();
			offset++;
		}

		return true;
	}

	memcpy( out, currentByte, iSize );
	currentByte += iSize;
	return true;
}

void BitBuffer::WriteBuf( const void* buf, int iSize )
{
	const unsigned char* in;
	int offset;
	unsigned int value;

	if ( buf == NULL || sizeError || iSize == 0 )
	{
		return;
	}

	if ( iSize + (int)( currentByte - data ) > maxSize )
	{
		sizeError = true;
		return;
	}

	in = (const unsigned char*)buf;

	if ( currentBit != 0 )
	{
		offset = 0;

		while ( iSize - offset > kLongBytes )
		{
			memcpy( &value, in + offset, sizeof( value ) );
			WriteLong( value );
			offset += kLongBytes;
		}

		while ( offset < iSize )
		{
			WriteByte( in[offset] );
			offset++;
		}

		return;
	}

	memcpy( currentByte, in, iSize );
	currentByte += iSize;
}

void BitBuffer::WriteBuf( BitBuffer* buf, int length )
{
	unsigned char* src;

	src = buf->currentByte;

	if ( src != NULL && !sizeError && length != 0 )
	{
		WriteBuf( (const void*)src, length );
		src = buf->currentByte;
	}

	if ( length + (int)( src - buf->data ) > buf->maxSize )
	{
		buf->sizeError = true;
	}

	buf->currentByte = src + length;
}

void BitBuffer::ConcatBuffer( BitBuffer* buffer )
{
	int length;

	length = buffer->CurrentSize();

	if ( buffer->data == NULL || sizeError || length == 0 )
	{
		return;
	}

	WriteBuf( buffer->data, length );
}

void BitBuffer::WriteString( const char* s )
{
	size_t length;

	if ( s != NULL )
	{
		length = strlen( s );

		if ( length == (size_t)-1 || sizeError )
		{
			return;
		}

		WriteBuf( s, (int)( length + 1 ) );
		return;
	}

	WriteByte( 0 );
}

char* BitBuffer::ReadString()
{
	static char string[kStringBufferSize];
	int length;
	int c;

	length = 0;

	while ( length != kStringBufferSize - 1 )
	{
		c = ReadChar();

		if ( c == -1 || c == 0 )
		{
			break;
		}

		string[length] = (char)c;
		length++;
	}

	string[length] = 0;
	return string;
}

char* BitBuffer::ReadStringLine()
{
	static char string[kStringLineSize];
	int length;
	int c;

	length = 0;

	while ( length != kStringLineSize - 1 )
	{
		c = ReadChar();

		if ( c == -1 || c == 0 || c == '\n' )
		{
			break;
		}

		string[length] = (char)c;
		length++;
	}

	string[length] = 0;
	return string;
}

char* BitBuffer::ReadBitString()
{
	static char buf[kStringBufferSize];
	int length;
	int c;

	length = 0;
	buf[0] = 0;

	do
	{
		c = ReadChar();
		buf[length] = (char)c;
		length++;
	} while ( c != 0 );

	return buf;
}

void BitBuffer::WriteBitString( const char* s )
{
	while ( *s != 0 )
	{
		WriteChar( (signed char)*s );
		s++;
	}

	WriteChar( 0 );
}

int BitBuffer::ReadBitData( void* dest, int length )
{
	unsigned char* out;
	int i;

	out = (unsigned char*)dest;

	for ( i = 0; i < length; i++ )
	{
		out[i] = (unsigned char)ReadByte();
	}

	return length;
}

void BitBuffer::WriteBitData( void* src, int length )
{
	unsigned char* in;
	int i;

	in = (unsigned char*)src;

	for ( i = 0; i < length; i++ )
	{
		WriteByte( in[i] );
	}
}

int BitBuffer::SkipString()
{
	int count;
	int c;

	count = 1;

	while ( 1 )
	{
		c = ReadChar();

		if ( c == -1 || c == 0 )
		{
			break;
		}

		count++;

		if ( count == kStringBufferSize )
		{
			break;
		}
	}

	return count;
}

float BitBuffer::ReadAngle()
{
	return (float)ReadByte() * ( kFullCircle / kByteAngleScale );
}

float BitBuffer::ReadHiresAngle()
{
	return (float)ReadWord() * ( kFullCircle / kHiresAngleScale );
}

void BitBuffer::WriteAngle( float f )
{
	int encoded;

	encoded = (int)( f * kByteAngleScale / kFullCircle );
	WriteBits( (unsigned char)encoded, kBitsPerByte );
}

void BitBuffer::WriteHiresAngle( float f )
{
	int encoded;

	encoded = (int)( f * kHiresAngleScale / kFullCircle );
	WriteBits( (unsigned short)encoded, kBitsPerWord );
}

float BitBuffer::ReadCoord()
{
	return (float)ReadWord() * kCoordResolution;
}

void BitBuffer::WriteCoord( float f )
{
	WriteShort( (int)( f * kCoordScale ) );
}

float BitBuffer::ReadBitCoord()
{
	int hasInteger;
	int hasFraction;
	int sign;
	float value;

	hasInteger = ReadBit();
	hasFraction = ReadBit();
	value = 0.0f;

	if ( hasInteger || hasFraction )
	{
		sign = ReadBit();

		if ( hasInteger )
		{
			value = (float)(int)ReadBits( kCoordIntegerBits );
		}

		if ( hasFraction )
		{
			value += (float)(int)ReadBits( kCoordFractionBits ) * kCoordResolution;
		}

		if ( sign )
		{
			value = -value;
		}
	}

	return value;
}

void BitBuffer::ReadBitVec3Coord( float* fa )
{
	int hasX;
	int hasY;
	int hasZ;

	hasX = ReadBit();
	hasY = ReadBit();
	hasZ = ReadBit();

	if ( hasX )
	{
		fa[0] = ReadBitCoord();
	}

	if ( hasY )
	{
		fa[1] = ReadBitCoord();
	}

	if ( hasZ )
	{
		fa[2] = ReadBitCoord();
	}
}

float BitBuffer::ReadBitAngle( int numbits )
{
	float span;
	unsigned int bits;

	span = (float)( 1 << numbits );
	bits = ReadBits( numbits );
	return (float)bits * ( kFullCircle / span );
}

void BitBuffer::WriteBitAngle( float fAngle, int numbits )
{
	unsigned int shift;
	int value;

	if ( numbits >= kBitsPerLong )
	{
		sizeError = true;
		return;
	}

	if ( sizeError )
	{
		return;
	}

	shift = 1u << numbits;
	value = (int)( fAngle * (float)shift ) / kCircleDegrees;
	value &= (int)( shift - 1u );
	WriteBits( (unsigned int)value, numbits );
}
