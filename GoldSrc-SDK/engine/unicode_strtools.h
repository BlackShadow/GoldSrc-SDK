#pragma once

#include "const.h"

typedef std::uint32_t uchar32;
typedef std::uint16_t uchar16;

enum EStringConvertErrorPolicy
{
	_STRINGCONVERTFLAG_SKIP			= 1,
	_STRINGCONVERTFLAG_FAIL			= 2,
	_STRINGCONVERTFLAG_ASSERT		= 4,

	STRINGCONVERT_REPLACE			= 0,
	STRINGCONVERT_SKIP				= _STRINGCONVERTFLAG_SKIP,
	STRINGCONVERT_FAIL				= _STRINGCONVERTFLAG_FAIL,

	STRINGCONVERT_ASSERT_REPLACE	= _STRINGCONVERTFLAG_ASSERT,
	STRINGCONVERT_ASSERT_SKIP		= _STRINGCONVERTFLAG_ASSERT | _STRINGCONVERTFLAG_SKIP,
	STRINGCONVERT_ASSERT_FAIL		= _STRINGCONVERTFLAG_ASSERT | _STRINGCONVERTFLAG_FAIL
};

bool			Q_IsValidUChar32( uchar32 character );
qboolean		Q_IsDeprecatedW( uchar32 character );
qboolean		Q_IsMeanSpaceW( uchar32 character );
int				Q_UTF8ToUChar32( const char* text, uchar32& character, bool& error );
qboolean		V_UTF8ToUChar32( const char* text, uchar32* character );
int				Q_UTF16ToUChar32( const uchar16* text, uchar32& character, bool& error );
int				Q_UChar32ToUTF8( uchar32 character, char* text );
int				Q_UChar32ToUTF8Len( uchar32 character );
int				Q_UChar32ToUTF16( uchar32 character, uchar16* text );
int				Q_UChar32ToUTF16Len( uchar32 character );

int				Q_UTF8ToUTF16( const char* source, uchar16* destination, int destinationBytes,
					EStringConvertErrorPolicy policy );
int				Q_UTF16ToUTF8( const uchar16* source, char* destination, int destinationBytes,
					EStringConvertErrorPolicy policy );
int				Q_UnicodeLength( const char* text );
const char*		Q_UnicodeAdvance( const char* text, int count );
qboolean		Q_UnicodeValidate( const char* text );
int				Q_UnicodeRepair( char* text );
qboolean		Q_StripUnprintableAndSpace( char* text );
