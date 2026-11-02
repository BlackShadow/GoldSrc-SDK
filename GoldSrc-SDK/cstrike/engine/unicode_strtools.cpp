#include "unicode_strtools.h"

#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <limits.h>

bool Q_IsValidUChar32( uchar32 character )
{
	return character < 0x110000 && ( character < 0xD800 || character >= 0xE000 )
		&& ( character & 0xFFFF ) < 0xFFFE && ( character < 0xFDD0 || character >= 0xFDF0 );
}

int Q_UChar32ToUTF8Len( uchar32 character )
{
	if ( (int)character < 0x80 )
		return 1;
	if ( (int)character < 0x800 )
		return 2;
	if ( (int)character < 0x10000 )
		return 3;
	return 4;
}

int Q_UChar32ToUTF8( uchar32 character, char* text )
{
	if ( (int)character < 0x80 )
	{
		text[0] = (char)character;
		return 1;
	}
	if ( (int)character < 0x800 )
	{
		text[0] = (char)( ( character >> 6 ) | 0xC0 );
		text[1] = (char)( ( character & 0x3F ) | 0x80 );
		return 2;
	}
	if ( (int)character < 0x10000 )
	{
		text[0] = (char)( ( character >> 12 ) | 0xE0 );
		text[1] = (char)( ( ( character >> 6 ) & 0x3F ) | 0x80 );
		text[2] = (char)( ( character & 0x3F ) | 0x80 );
		return 3;
	}
	text[0] = (char)( ( ( character >> 18 ) & 7 ) | 0xF0 );
	text[1] = (char)( ( ( character >> 12 ) & 0x3F ) | 0x80 );
	text[2] = (char)( ( ( character >> 6 ) & 0x3F ) | 0x80 );
	text[3] = (char)( ( character & 0x3F ) | 0x80 );
	return 4;
}

int Q_UChar32ToUTF16Len( uchar32 character )
{
	return (int)character < 0x10000 ? 1 : 2;
}

int Q_UChar32ToUTF16( uchar32 character, uchar16* text )
{
	if ( (int)character < 0x10000 )
	{
		text[0] = (uchar16)character;
		return 1;
	}
	character -= 0x10000;
	text[0] = (uchar16)( ( character >> 10 ) | 0xD800 );
	text[1] = (uchar16)( ( character & 0x3FF ) | 0xDC00 );
	return 2;
}

int Q_UTF16ToUChar32( const uchar16* text, uchar32& character, bool& error )
{
	uchar32 value = text[0];
	if ( value < 0xD800 || value >= 0xE000 )
	{
		if ( Q_IsValidUChar32( value ) )
		{
			character = value;
			error = false;
			return 1;
		}
	}
	else if ( value < 0xDC00 )
	{
		uchar32 trail = text[1];
		if ( trail >= 0xDC00 && trail < 0xE000 )
		{
			value = 0x10000 + ( ( value - 0xD800 ) << 10 ) + trail - 0xDC00;
			bool valid = Q_IsValidUChar32( value );
			character = valid ? value : '?';
			error = !valid;
			return 2;
		}
	}
	character = '?';
	error = true;
	return 1;
}

int Q_UTF8ToUChar32( const char* text, uchar32& character, bool& error )
{
	const byte* input = (const byte*)text;
	uchar32 value = input[0];
	uchar32 minimum = 0x80;
	int bytes = 1;

	if ( value < 0x80 )
		goto decoded;
	if ( value < 0xC0 || value > 0xF7 || ( input[1] & 0xC0 ) != 0x80 )
		goto invalid;

	value = ( ( value - 0xC0 ) << 6 ) + input[1] - 0x80;
	bytes = 2;
	if ( value & ( 0x20 << 6 ) )
	{
		if ( ( input[2] & 0xC0 ) != 0x80 )
			goto invalid;
		value = ( ( value - ( 0x20 << 6 ) ) << 6 ) + input[2] - 0x80;
		bytes = 3;
		minimum = 0x800;
		if ( value & ( 0x10 << 12 ) )
		{
			if ( ( input[3] & 0xC0 ) != 0x80 )
				goto invalid;
			value = ( ( value - ( 0x10 << 12 ) ) << 6 ) + input[3] - 0x80;
			bytes = 4;
			minimum = 0x10000;
		}
		else if ( value >= 0xD800 && value < 0xDC00 && input[3] == 0xED )
		{
			byte trail = (byte)( input[4] - 0xB0 );
			if ( trail < 0x10 && ( input[5] & 0xC0 ) == 0x80 )
			{
				value = 0x10000 + ( ( value - 0xD800 ) << 10 ) + ( trail << 6 ) + input[5] - 0x80;
				bytes = 6;
				minimum = 0x10000;
			}
		}
	}
	if ( value < minimum || !Q_IsValidUChar32( value ) )
		goto invalid;

decoded:
	character = value;
	error = false;
	return bytes;

invalid:
	character = '?';
	error = true;
	return bytes;
}

namespace
{
int Q_UTF32ToUChar32( const uchar32* text, uchar32& character, bool& error )
{
	bool valid = Q_IsValidUChar32( text[0] );
	character = valid ? text[0] : '?';
	error = !valid;
	return 1;
}

int Q_UChar32ToUTF32Len( uchar32 )
{
	return 1;
}

int Q_UChar32ToUTF32( uchar32 character, uchar32* text )
{
	text[0] = character;
	return 1;
}

template <class Input, class Output, int ( *Decode )( const Input*, uchar32&, bool& ),
	int ( *EncodedLength )( uchar32 ), int ( *Encode )( uchar32, Output* )>
int Q_UnicodeConvertT( const Input* source, Output* destination, int destinationBytes,
	EStringConvertErrorPolicy policy )
{
	int length = 0;
	if ( destination )
	{
		int limit = (int)( (unsigned int)destinationBytes / sizeof( Output ) );
		if ( limit <= 0 )
			return 0;
		--limit;
		while ( *source )
		{
			uchar32 character;
			bool error = false;
			source += Decode( source, character, error );
			int size = EncodedLength( character );
			if ( length + size > limit )
				break;
			length += Encode( character, destination + length );
			if ( error )
			{
				if ( policy & _STRINGCONVERTFLAG_SKIP )
					length -= size;
				else if ( policy & _STRINGCONVERTFLAG_FAIL )
				{
					destination[0] = 0;
					return 0;
				}
			}
		}
		destination[length] = 0;
	}
	else
	{
		while ( *source )
		{
			uchar32 character;
			bool error = false;
			source += Decode( source, character, error );
			int size = EncodedLength( character );
			length += size;
			if ( error )
			{
				if ( policy & _STRINGCONVERTFLAG_SKIP )
					length -= size;
				else if ( policy & _STRINGCONVERTFLAG_FAIL )
				{
					destination[0] = 0;
					return 0;
				}
			}
		}
	}
	return ( length + 1 ) * (int)sizeof( Output );
}

bool Q_IsTrimmableSpace( uchar32 character )
{
	if constexpr ( sizeof( wint_t ) < sizeof( uchar32 ) )
	{
		if ( character > 0xFFFF )
			return Q_IsMeanSpaceW( character ) != 0;
	}
	return iswspace( (wint_t)character ) || Q_IsMeanSpaceW( character );
}
}

int Q_UTF8ToUTF16( const char* source, uchar16* destination, int destinationBytes, EStringConvertErrorPolicy policy )
{
	return Q_UnicodeConvertT<char, uchar16, Q_UTF8ToUChar32, Q_UChar32ToUTF16Len, Q_UChar32ToUTF16>(
		source, destination, destinationBytes, policy );
}

int Q_UTF8ToUTF32( const char* source, uchar32* destination, int destinationBytes, EStringConvertErrorPolicy policy )
{
	return Q_UnicodeConvertT<char, uchar32, Q_UTF8ToUChar32, Q_UChar32ToUTF32Len, Q_UChar32ToUTF32>(
		source, destination, destinationBytes, policy );
}

int Q_UTF16ToUTF8( const uchar16* source, char* destination, int destinationBytes, EStringConvertErrorPolicy policy )
{
	return Q_UnicodeConvertT<uchar16, char, Q_UTF16ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8>(
		source, destination, destinationBytes, policy );
}

int Q_UTF16ToUTF32( const uchar16* source, uchar32* destination, int destinationBytes, EStringConvertErrorPolicy policy )
{
	return Q_UnicodeConvertT<uchar16, uchar32, Q_UTF16ToUChar32, Q_UChar32ToUTF32Len, Q_UChar32ToUTF32>(
		source, destination, destinationBytes, policy );
}

int Q_UTF32ToUTF8( const uchar32* source, char* destination, int destinationBytes, EStringConvertErrorPolicy policy )
{
	return Q_UnicodeConvertT<uchar32, char, Q_UTF32ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8>(
		source, destination, destinationBytes, policy );
}

int Q_UTF32ToUTF16( const uchar32* source, uchar16* destination, int destinationBytes, EStringConvertErrorPolicy policy )
{
	return Q_UnicodeConvertT<uchar32, uchar16, Q_UTF32ToUChar32, Q_UChar32ToUTF16Len, Q_UChar32ToUTF16>(
		source, destination, destinationBytes, policy );
}

qboolean V_UTF8ToUChar32( const char* text, uchar32* character )
{
	bool error;
	Q_UTF8ToUChar32( text, *character, error );
	return error;
}

int Q_UnicodeLength( const char* text )
{
	int length = 0;
	while ( *text )
	{
		uchar32 character;
		bool error;
		text += Q_UTF8ToUChar32( text, character, error );
		++length;
	}
	return length;
}

char* Q_UnicodeAdvance( char* text, int count )
{
	while ( count > 0 && *text )
	{
		uchar32 character;
		bool error;
		text += Q_UTF8ToUChar32( text, character, error );
		--count;
	}
	return text;
}

qboolean Q_UnicodeValidate( const char* text )
{
	while ( *text )
	{
		uchar32 character;
		bool error;
		int bytes = Q_UTF8ToUChar32( text, character, error );
		if ( error || bytes == 6 )
			return false;
		text += bytes;
	}
	return true;
}

int Q_UnicodeRepair( char* text )
{
	return Q_UnicodeConvertT<char, char, Q_UTF8ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8>(
		text, text, 65535, STRINGCONVERT_SKIP );
}

qboolean Q_IsMeanSpaceW( uchar32 character )
{
	return ( character >= 0x80 && character <= 0xA0 ) || character == 0x34F
		|| ( character >= 0x2000 && character <= 0x200D ) || character == 0x2028
		|| character == 0x2029 || character == 0x202F || character == 0x2060
		|| character == 0xFEFF || character == 0xFFFC;
}

qboolean Q_IsDeprecatedW( uchar32 character )
{
	return ( character >= 0x202A && character <= 0x202E ) || ( character >= 0x206A && character <= 0x206F );
}

qboolean Q_StripUnprintableAndSpace( char* text )
{
	if ( !text || !*text )
		return false;
	size_t length = strlen( text );
	if ( length > INT_MAX / sizeof( uchar32 ) - 1 )
		return false;
	int bytes = (int)length;
	int capacity = ( bytes + 1 ) * (int)sizeof( uchar32 );
	uchar32* characters = (uchar32*)malloc( capacity );
	if ( !characters )
		return false;
	int converted = Q_UTF8ToUTF32( text, characters, capacity, STRINGCONVERT_ASSERT_REPLACE );
	if ( converted < (int)sizeof( uchar32 ) || converted > capacity )
	{
		free( characters );
		return false;
	}
	uchar32* input = characters;
	uchar32* output = characters;
	const uchar32* end = characters + capacity / sizeof( uchar32 ) - 1;
	while ( input < end && *input )
	{
		uchar32 character = *input++;
		if ( character >= ' ' && character != 0x85 && !Q_IsDeprecatedW( character ) )
			*output++ = character;
	}
	*output = 0;
	qboolean stripped = input != output;
	bool trimmed = false;
	int last = (int)( converted / sizeof( uchar32 ) ) - 2;
	while ( last >= 0 && Q_IsTrimmableSpace( characters[last] ) )
	{
		characters[last--] = 0;
		trimmed = true;
	}
	int first = 0;
	while ( first < last && Q_IsTrimmableSpace( characters[first] ) )
	{
		++first;
		trimmed = true;
	}
	if ( stripped || trimmed )
		Q_UTF32ToUTF8( characters + first, text, bytes, STRINGCONVERT_ASSERT_REPLACE );
	free( characters );
	return stripped;
}
