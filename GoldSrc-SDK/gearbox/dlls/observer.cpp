//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Functionality for the observer chase camera
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "pm_shared.h"

extern int gmsgSpectator;

void CBasePlayer::StopObserver()
{
	if ( pev->iuser1 || pev->iuser2 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
			WRITE_BYTE( entindex() );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		pev->iuser1 = 0;
		pev->iuser2 = 0;
		m_hObserverTarget = NULL;
		pev->flags &= ~FL_SPECTATOR;
	}
}

void CBasePlayer::Observer_HandleButtons()
{
}

void CBasePlayer::Observer_SetMode( int iMode )
{
	if ( pev->iuser1 != iMode )
	{
		pev->iuser1 = OBS_ROAMING;
		pev->iuser2 = 0;
		pev->maxspeed = 320;
	}
}
