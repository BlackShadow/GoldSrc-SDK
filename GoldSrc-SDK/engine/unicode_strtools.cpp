#include "unicode_strtools.h"
#include "common.h"

#include <cstdlib>
#include <cwctype>
#include <climits>

int Q_UChar32ToUTF8Len( uchar32 uVal )
{
	if ( uVal < 0x80 )
		return 1;
	if ( uVal < 0x800 )
		return 2;
	if ( uVal < 0x10000 )
		return 3;
	return 4;
}

int Q_UChar32ToUTF8( uchar32 uVal, char* pUTF8Out )
{
	if ( uVal < 0x80 )
	{
		pUTF8Out[0] = (char)uVal;
		return 1;
	}

	if ( uVal < 0x800 )
	{
		pUTF8Out[0] = (char)( ( uVal >> 6 ) | 0xC0 );
		pUTF8Out[1] = (char)( ( uVal & 0x3F ) | 0x80 );
		return 2;
	}

	if ( uVal < 0x10000 )
	{
		pUTF8Out[0] = (char)( ( uVal >> 12 ) | 0xE0 );
		pUTF8Out[1] = (char)( ( ( uVal >> 6 ) & 0x3F ) | 0x80 );
		pUTF8Out[2] = (char)( ( uVal & 0x3F ) | 0x80 );
		return 3;
	}

	pUTF8Out[0] = (char)( ( ( uVal >> 18 ) & 0x07 ) | 0xF0 );
	pUTF8Out[1] = (char)( ( ( uVal >> 12 ) & 0x3F ) | 0x80 );
	pUTF8Out[2] = (char)( ( ( uVal >> 6 ) & 0x3F ) | 0x80 );
	pUTF8Out[3] = (char)( ( uVal & 0x3F ) | 0x80 );
	return 4;
}

int Q_UChar32ToUTF16Len( uchar32 uVal )
{
	return uVal < 0x10000 ? 1 : 2;
}

int Q_UChar32ToUTF16( uchar32 uVal, uchar16* pUTF16Out )
{
	if ( uVal < 0x10000 )
	{
		pUTF16Out[0] = (uchar16)uVal;
		return 1;
	}

	uVal -= 0x10000;
	pUTF16Out[0] = (uchar16)( ( uVal >> 10 ) | 0xD800 );
	pUTF16Out[1] = (uchar16)( ( uVal & 0x3FF ) | 0xDC00 );
	return 2;
}

int Q_UTF16ToUChar32( const uchar16* pUTF16, uchar32& uValueOut, bool& bErrorOut )
{
	uchar32 uVal = pUTF16[0];

	if ( uVal < 0xD800 || uVal >= 0xE000 )
	{
		if ( Q_IsValidUChar32( uVal ) )
		{
			uValueOut = uVal;
			bErrorOut = false;
			return 1;
		}
	}
	else if ( uVal < 0xDC00 )
	{
		uchar32 uTrail = pUTF16[1];
		if ( uTrail >= 0xDC00 && uTrail < 0xE000 )
		{
			uVal = 0x10000 + ( ( uVal - 0xD800 ) << 10 ) + uTrail - 0xDC00;
			bool bValid = Q_IsValidUChar32( uVal );
			uValueOut = bValid ? uVal : '?';
			bErrorOut = !bValid;
			return 2;
		}
	}

	uValueOut = '?';
	bErrorOut = true;
	return 1;
}

qboolean Q_IsMeanSpaceW( uchar32 wch )
{
	return ( wch >= 0x80 && wch <= 0xA0 )
		|| wch == 0x34F
		|| ( wch >= 0x2000 && wch <= 0x200D )
		|| wch == 0x2028
		|| wch == 0x2029
		|| wch == 0x202F
		|| wch == 0x2060
		|| wch == 0xFEFF
		|| wch == 0xFFFC;
}

qboolean Q_IsDeprecatedW( uchar32 wch )
{
	return ( wch >= 0x202A && wch <= 0x202E )
		|| ( wch >= 0x206A && wch <= 0x206F );
}

template <typename T_IN, typename T_OUT, int ( *DecodeSrc )( const T_IN*, uchar32&, bool& ),
	int ( *EncodeDstLen )( uchar32 ), int ( *EncodeDst )( uchar32, T_OUT* )>
static int Q_UnicodeConvertT( const T_IN* pIn, T_OUT* pOut, int nOutBytes, EStringConvertErrorPolicy ePolicy )
{
	int nOut = 0;

	if ( pOut )
	{
		int nMaxOut = (int)( (unsigned int)nOutBytes / sizeof( T_OUT ) );
		if ( nMaxOut <= 0 )
			return 0;

		nMaxOut--;

		while ( *pIn )
		{
			uchar32 uVal;
			bool bErr = false;
			pIn += DecodeSrc( pIn, uVal, bErr );

			int nLen = EncodeDstLen( uVal );
			if ( nOut + nLen > nMaxOut )
				break;

			nOut += EncodeDst( uVal, pOut + nOut );
			if ( bErr )
			{
				if ( ePolicy & _STRINGCONVERTFLAG_SKIP )
				{
					nOut -= nLen;
				}
				else if ( ePolicy & _STRINGCONVERTFLAG_FAIL )
				{
					pOut[0] = 0;
					return 0;
				}
			}
		}

		pOut[nOut] = 0;
	}
	else
	{
		while ( *pIn )
		{
			uchar32 uVal;
			bool bErr = false;
			pIn += DecodeSrc( pIn, uVal, bErr );

			int nLen = EncodeDstLen( uVal );
			nOut += nLen;
			if ( bErr )
			{
				if ( ePolicy & _STRINGCONVERTFLAG_SKIP )
				{
					nOut -= nLen;
				}
				else if ( ePolicy & _STRINGCONVERTFLAG_FAIL )
				{
					pOut[0] = 0;
					return 0;
				}
			}
		}
	}

	return ( nOut + 1 ) * (int)sizeof( T_OUT );
}

static int Q_UTF32ToUChar32( const uchar32* pUTF32, uchar32& uValueOut, bool& bErrorOut )
{
	uchar32 uVal = *pUTF32;

	if ( Q_IsValidUChar32( uVal ) )
	{
		uValueOut = uVal;
		bErrorOut = false;
	}
	else
	{
		uValueOut = '?';
		bErrorOut = true;
	}
	return 1;
}

static int Q_UChar32ToUTF32Len( uchar32 )
{
	return 1;
}

static int Q_UChar32ToUTF32( uchar32 uVal, uchar32* pUTF32 )
{
	pUTF32[0] = uVal;
	return 1;
}

int Q_UTF8ToUTF16( const char* pUTF8, uchar16* pUTF16, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT<char, uchar16, Q_UTF8ToUChar32, Q_UChar32ToUTF16Len, Q_UChar32ToUTF16>(
		pUTF8, pUTF16, cubDestSizeInBytes, ePolicy );
}

int Q_UTF16ToUTF8( const uchar16* pUTF16, char* pUTF8, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT<uchar16, char, Q_UTF16ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8>(
		pUTF16, pUTF8, cubDestSizeInBytes, ePolicy );
}

int Q_UTF8ToUTF32( const char* pUTF8, uchar32* pUTF32, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT<char, uchar32, Q_UTF8ToUChar32, Q_UChar32ToUTF32Len, Q_UChar32ToUTF32>(
		pUTF8, pUTF32, cubDestSizeInBytes, ePolicy );
}

int Q_UTF16ToUTF32( const uchar16* pUTF16, uchar32* pUTF32, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT<uchar16, uchar32, Q_UTF16ToUChar32, Q_UChar32ToUTF32Len, Q_UChar32ToUTF32>(
		pUTF16, pUTF32, cubDestSizeInBytes, ePolicy );
}

int Q_UTF32ToUTF8( const uchar32* pUTF32, char* pUTF8, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT<uchar32, char, Q_UTF32ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8>(
		pUTF32, pUTF8, cubDestSizeInBytes, ePolicy );
}

int Q_UTF32ToUTF16( const uchar32* pUTF32, uchar16* pUTF16, int cubDestSizeInBytes, EStringConvertErrorPolicy ePolicy )
{
	return Q_UnicodeConvertT<uchar32, uchar16, Q_UTF32ToUChar32, Q_UChar32ToUTF16Len, Q_UChar32ToUTF16>(
		pUTF32, pUTF16, cubDestSizeInBytes, ePolicy );
}

int Q_UnicodeRepair( char* pUTF8 )
{
	return Q_UnicodeConvertT<char, char, Q_UTF8ToUChar32, Q_UChar32ToUTF8Len, Q_UChar32ToUTF8>(
		pUTF8, pUTF8, 65535, STRINGCONVERT_SKIP );
}

static bool Q_IsTrimmableSpace( uchar32 wch )
{
	if constexpr ( sizeof( wint_t ) < sizeof( uchar32 ) )
	{
		if ( wch > 0xFFFF )
			return Q_IsMeanSpaceW( wch ) != 0;
	}

	return iswspace( (wint_t)wch ) || Q_IsMeanSpaceW( wch );
}

qboolean Q_StripUnprintableAndSpace( char* pch )
{
#if defined( _WIN32 )
	using StripCharacter = uchar16;
#else
	using StripCharacter = uchar32;
#endif

	int cch = Q_strlen( pch );
	int cubDest = ( cch + 1 ) * sizeof( StripCharacter );
	StripCharacter* pwch_alloced = (StripCharacter*)malloc( cubDest );
#if defined( _WIN32 )
	int cubConverted = Q_UTF8ToUTF16( pch, pwch_alloced, cubDest, STRINGCONVERT_ASSERT_REPLACE );
#else
	int cubConverted = Q_UTF8ToUTF32( pch, pwch_alloced, cubDest, STRINGCONVERT_ASSERT_REPLACE );
#endif

	StripCharacter* pwch = pwch_alloced;
	StripCharacter* pwchDest = pwch_alloced;
	while ( *pwch )
	{
		StripCharacter wch = *pwch;
		if ( wch >= ' ' && wch != 0x85 && !Q_IsDeprecatedW( wch ) )
			*pwchDest++ = wch;
		pwch++;
	}
	*pwchDest = 0;

	qboolean bStrippedAny = pwch != pwchDest;
	bool bStrippedWhitespace = false;

	int iLast = (int)( cubConverted / sizeof( StripCharacter ) ) - 2;
	while ( iLast >= 0 && Q_IsTrimmableSpace( pwch_alloced[iLast] ) )
	{
		pwch_alloced[iLast--] = 0;
		bStrippedWhitespace = true;
	}

	int iFirst = 0;
	while ( iFirst < iLast && Q_IsTrimmableSpace( pwch_alloced[iFirst] ) )
	{
		iFirst++;
		bStrippedWhitespace = true;
	}

	if ( bStrippedAny || bStrippedWhitespace )
	{
#if defined( _WIN32 )
		Q_UTF16ToUTF8( pwch_alloced + iFirst, pch, cch, STRINGCONVERT_ASSERT_REPLACE );
#else
		Q_UTF32ToUTF8( pwch_alloced + iFirst, pch, cch, STRINGCONVERT_ASSERT_REPLACE );
#endif
	}

	free( pwch_alloced );
	return bStrippedAny;
}

//-----------------------------------------------------------------------------
// Purpose: determine if a uchar32 represents a valid Unicode code point
//-----------------------------------------------------------------------------
bool Q_IsValidUChar32( uchar32 uVal )
{
	// Values > 0x10FFFF are explicitly invalid; ditto for UTF-16 surrogate halves,
	// values ending in FFFE or FFFF, or values in the 0x00FDD0-0x00FDEF reserved range
	return uVal < 0x110000 && ( uVal < 0xD800 || uVal >= 0xE000 ) && ( uVal & 0xFFFF ) < 0xFFFE
		&& ( uVal < 0xFDD0 || uVal >= 0xFDF0 );
}

int Q_UTF8ToUChar32( const char* pUTF8_, uchar32& uValueOut, bool& bErrorOut )
{
	const byte* pUTF8 = (const byte*)pUTF8_;
	uchar32 uValue = pUTF8[0];
	uchar32 uMinValue = 0x80;
	int nBytes = 1;

	// 0....... single byte
	if ( uValue < 0x80 )
		goto decodeFinishedNoCheck;

	if ( uValue < 0xC0 || uValue > 0xF7 || ( pUTF8[1] & 0xC0 ) != 0x80 )
		goto decodeError;

	uValue = ( ( uValue - 0xC0 ) << 6 ) + pUTF8[1] - 0x80;
	nBytes = 2;

	// 1110.... three-byte lead byte
	if ( uValue & ( 0x20 << 6 ) )
	{
		if ( ( pUTF8[2] & 0xC0 ) != 0x80 )
			goto decodeError;

		uValue = ( ( uValue - ( 0x20 << 6 ) ) << 6 ) + pUTF8[2] - 0x80;
		nBytes = 3;
		uMinValue = 0x800;

		if ( uValue & ( 0x10 << 12 ) )
		{
			if ( ( pUTF8[3] & 0xC0 ) != 0x80 )
				goto decodeError;

			uValue = ( ( uValue - ( 0x10 << 12 ) ) << 6 ) + pUTF8[3] - 0x80;
			nBytes = 4;
			uMinValue = 0x10000;
		}
		else if ( uValue >= 0xD800 && uValue < 0xDC00 && pUTF8[3] == 0xED )
		{
			byte trailSecond = (byte)( pUTF8[4] - 0xB0 );
			if ( trailSecond < 0x10 && ( pUTF8[5] & 0xC0 ) == 0x80 )
			{
				uValue = 0x10000 + ( ( uValue - 0xD800 ) << 10 ) + ( trailSecond << 6 ) + pUTF8[5] - 0x80;
				nBytes = 6;
				uMinValue = 0x10000;
			}
		}
	}

	if ( uValue < uMinValue || !Q_IsValidUChar32( uValue ) )
		goto decodeError;

decodeFinishedNoCheck:
	uValueOut = uValue;
	bErrorOut = false;
	return nBytes;

decodeError:
	uValueOut = '?';
	bErrorOut = true;
	return nBytes;
}

qboolean V_UTF8ToUChar32( const char* pUTF8, uchar32* uValueOut )
{
	bool bError;
	Q_UTF8ToUChar32( pUTF8, *uValueOut, bError );
	return bError;
}

const char* Q_UnicodeAdvance( const char* pUTF8, int nChars )
{
	uchar32 uVal;
	bool bError;

	while ( nChars > 0 && *pUTF8 )
	{
		pUTF8 += Q_UTF8ToUChar32( pUTF8, uVal, bError );
		nChars--;
	}

	return pUTF8;
}

int Q_UnicodeLength( const char* pUTF8 )
{
	int nChars = 0;
	uchar32 uVal;
	bool bError;

	while ( *pUTF8 )
	{
		pUTF8 += Q_UTF8ToUChar32( pUTF8, uVal, bError );
		nChars++;
	}

	return nChars;
}

qboolean Q_UnicodeValidate( const char* pUTF8 )
{
	uchar32 uVal;
	bool bError;

	while ( *pUTF8 )
	{
		int nCharSize = Q_UTF8ToUChar32( pUTF8, uVal, bError );
		if ( bError || nCharSize == 6 )
			return false;
		pUTF8 += nCharSize;
	}

	return true;
}
