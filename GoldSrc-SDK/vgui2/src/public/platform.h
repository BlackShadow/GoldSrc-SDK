//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Extremely low-level platform-specific stuff
//=============================================================================


#ifndef VGUI_PLATFORM_H
#define VGUI_PLATFORM_H

#ifdef _WIN32
#include <string.h>

inline int strcasecmp( const char* left, const char* right )
{
	return _stricmp( left, right );
}

inline int strncasecmp( const char* left, const char* right, size_t count )
{
	return _strnicmp( left, right, count );
}
#else
#include <strings.h>
#endif

#endif /* PLATFORM_H */
