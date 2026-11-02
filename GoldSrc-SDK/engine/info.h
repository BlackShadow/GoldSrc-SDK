//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#pragma once

#include "const.h"

#define MAX_INFO_KEY 128
#define MAX_INFO_VALUE 128
#define INFO_VALUE_BUFFERS 4

// userinfo functions
const char*		Info_ValueForKey( const char* info, const char* key );
void			Info_RemoveKey( char* info, const char* key );
void			Info_RemovePrefixedKeys( char* info, char prefix );
void			Info_SetValueForKey( char* info, const char* key, const char* value, int maxsize );
void			Info_SetValueForStarKey( char* info, const char* key, const char* value, int maxsize );
qboolean		Info_IsKeyImportant( const char* key );
qboolean		Info_IsValid( const char* info );
void			Info_Print( const char* info );

char*			Info_Serverinfo();
