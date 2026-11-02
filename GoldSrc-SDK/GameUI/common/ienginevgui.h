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

#include "interface.h"
#include "vgui/VGUI.h"

struct TSteamProgress;
struct TSteamError;

enum VGUIPANEL
{
	PANEL_ROOT = 0,
	PANEL_CLIENTDLL,
	PANEL_GAMEUIDLL
};

class IEngineVGui : public IBaseInterface
{
public:
	virtual vgui2::VPANEL GetPanel( VGUIPANEL type ) = 0;
	virtual bool SteamRefreshLogin( const char* password, bool secure ) = 0;
	virtual bool SteamProcessCall( bool* finished, TSteamProgress* progress, TSteamError* error ) = 0;
};
