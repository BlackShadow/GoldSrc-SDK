#include "HLTV/common/byteorder.h"

#include <string.h>

short ShortSwap( short value )
{
	unsigned short bits = (unsigned short)value;
	return (short)( ( bits << 8 ) | ( bits >> 8 ) );
}

short ShortNoSwap( short value )
{
	return value;
}

int LongSwap( int value )
{
	unsigned char b1, b2, b3, b4;

	b1 = value & 255;
	b2 = ( value >> 8 ) & 255;
	b3 = ( value >> 16 ) & 255;
	b4 = ( value >> 24 ) & 255;

	return ( (int)b1 << 24 ) + ( (int)b2 << 16 ) + ( (int)b3 << 8 ) + b4;
}

int LongNoSwap( int value )
{
	return value;
}

float FloatSwap( float value )
{
	int bits;

	memcpy( &bits, &value, sizeof( bits ) );
	bits = LongSwap( bits );
	memcpy( &value, &bits, sizeof( value ) );
	return value;
}

float FloatNoSwap( float value )
{
	return value;
}

short BigShort( short value )
{
	return ShortSwap( value );
}

int BigLong( int value )
{
	return LongSwap( value );
}

float BigFloat( float value )
{
	return FloatSwap( value );
}

short LittleShort( short value )
{
	return value;
}

int LittleLong( int value )
{
	return value;
}

float LittleFloat( float value )
{
	return value;
}
