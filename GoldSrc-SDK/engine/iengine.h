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
// $NoKeywords: $
//=============================================================================
#pragma once

class IEngine
{
public:
	virtual ~IEngine() = 0;

	virtual bool Load( bool dedicated, char* basedir, char* cmdline ) = 0;
	virtual void Unload() = 0;

	virtual void SetState( int state ) = 0;
	virtual int GetState() = 0;
	virtual void SetSubState( int state ) = 0;
	virtual int GetSubState() = 0;

	virtual int Frame() = 0;
	virtual double GetFrameTime() = 0;
	virtual double GetCurTime() = 0;

	virtual void TrapKey_Event( int key, bool down ) = 0;
	virtual void TrapMouse_Event( int buttons, bool down ) = 0;
	virtual void StartTrapMode() = 0;
	virtual bool IsTrapping() = 0;
	virtual bool CheckDoneTrapping( int& buttons, int& key ) = 0;

	virtual int GetQuitting() = 0;
	virtual void SetQuitting( int quitting ) = 0;
};

extern IEngine* eng;
