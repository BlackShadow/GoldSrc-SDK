#pragma once

#ifdef _WIN32
#include "../../../HL25-Engine/engine/iengine.h"
#else

//-----------------------------------------------------------------------------
// Purpose: controls engine startup, frame execution and launcher state.
//-----------------------------------------------------------------------------
class IEngine
{
public:
	enum
	{
		QUIT_NOTQUITTING	= 0,
		QUIT_TODESKTOP	= 1,
		QUIT_RESTART	= 2
	};

	virtual ~IEngine() {}

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

#endif
