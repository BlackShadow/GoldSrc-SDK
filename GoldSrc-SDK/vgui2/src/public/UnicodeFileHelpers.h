#ifndef UNICODEFILEHELPERS_H
#define UNICODEFILEHELPERS_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/basetypes.h"

class CUtlBuffer;

// helper functions for parsing unicode file buffers
ucs2*	AdvanceOverWhitespace( ucs2* start );
ucs2*	ReadUnicodeToken( ucs2* start, ucs2* token, int tokenBufferSize, bool& quoted );
ucs2*	ReadToEndOfLine( ucs2* start );
void	WriteUnicodeString( CUtlBuffer& buf, const wchar_t* string, bool addQuotes );
void	WriteAsciiStringAsUnicode( CUtlBuffer& buf, const char* string, bool addQuotes );

#endif // UNICODEFILEHELPERS_H
