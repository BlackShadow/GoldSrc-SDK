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
#ifndef ISCOREBOARDINTERFACE_H
#define ISCOREBOARDINTERFACE_H

#include "vgui/VGUI.h"

//-----------------------------------------------------------------------------
// Purpose: interface for apps to update the scoreboard
//-----------------------------------------------------------------------------
class IScoreBoardInterface
{
public:
	virtual void Update( const char* serverName, bool teamplay, bool allowSpectators ) = 0;
	virtual void RebuildTeams( const char* serverName, bool teamplay, bool allowSpectators ) = 0;
	virtual void DeathMsg( int killer, int victim ) = 0;
	virtual void Activate( bool mouseInput ) = 0;
	virtual void MoveToFront() = 0;
	virtual bool IsVisible() = 0;
	virtual void SetVisible( bool visible ) = 0;
	virtual void SetParent( vgui2::VPANEL parent ) = 0;
	virtual void SetMouseInput( bool mouseInput ) = 0;
};

#endif
