//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Shared code for parsing / searching for characters in a string
//			using lookup tables
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef CHARACTERSET_H
#define CHARACTERSET_H

typedef struct characterset_s
{
	unsigned char set[256];
} characterset_t;

void CharacterSetBuild( characterset_t* set, const char* characters );

#endif // CHARACTERSET_H
