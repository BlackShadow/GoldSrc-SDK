//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
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
#include "characterset.h"

#include <cstring>

void CharacterSetBuild( characterset_t* set, const char* characters )
{
	// Test our pointers
	if ( !set || !characters )
		return;

	std::memset( set->set, 0, sizeof( set->set ) );
	for ( const char* cursor = characters; *cursor; ++cursor )
		set->set[*cursor] = 1;
}
