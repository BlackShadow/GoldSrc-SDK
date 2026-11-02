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
#ifndef ISPECTATORINTERFACE_H
#define ISPECTATORINTERFACE_H

#include "vgui/VGUI.h"

class ISpectatorInterface
{
public:
	virtual void Activate() = 0;	// shows spectator stuff (black bars etc...)
	virtual void ShowBottomBar() = 0;
	virtual void HideGUI() = 0; // hide user interface again
	virtual void DeactivateGUI() = 0;
	virtual bool IsGUIVisible() = 0;
	virtual bool IsActiveGUIVisible() = 0;
	virtual void EnableInsetView( int enabled ) = 0;
	virtual void SetImage( const char* image ) = 0;
	virtual void Update( const char* bottomText, int player, int mode, bool spectatorOnly, int spectatorCount,
		const char* mapName ) = 0;
	virtual void UpdateSpectatorPlayerList() = 0; // update spectator list panel
	virtual void UpdateTimer() = 0; // update only timer 
	virtual void SetVisible( bool visible ) = 0;
	virtual bool IsVisible() = 0;	// true if spectator panel is visible
	virtual void SetParent( vgui2::VPANEL parent ) = 0;
	virtual int GetTopBarHeight() = 0;
	virtual int GetBottomBarHeight() = 0;
};

#endif // ISPECTATORINTERFACE_H
