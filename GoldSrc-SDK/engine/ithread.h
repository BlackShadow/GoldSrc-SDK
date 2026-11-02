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

#include "cd.h"

// Typedef to point at member functions of CCDAudio
typedef void ( CCDAudio::*vfunc )( int, int );

//-----------------------------------------------------------------------------
// Purpose: CD Audio thread processing
//-----------------------------------------------------------------------------
class IThread
{
public:
	virtual ~IThread() = default;

	virtual bool Init() = 0;
	virtual bool Shutdown() = 0;
	// Add specified item to thread for processing
	virtual bool AddThreadItem( vfunc function, int first, int second ) = 0;
};

extern IThread* thread;
