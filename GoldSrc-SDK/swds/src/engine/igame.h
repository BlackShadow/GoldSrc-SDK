#pragma once

#ifdef _WIN32
#include "../../../HL25-Engine/engine/igame.h"
#else

#include "../common/port.h"

//-----------------------------------------------------------------------------
// Purpose: supplies application window and event services to the engine.
//-----------------------------------------------------------------------------
class IGame
{
public:
	virtual ~IGame() {}

	virtual bool Init( void* instance ) = 0;
	virtual bool Shutdown() = 0;
	virtual bool CreateGameWindow() = 0;
	virtual void SleepUntilInput( int milliseconds ) = 0;
	virtual HWND GetMainWindow() = 0;
	virtual HWND* GetMainWindowAddress() = 0;
	virtual void SetWindowXY( int x, int y ) = 0;
	virtual void SetWindowSize( int width, int height ) = 0;
	virtual void GetWindowRect( int* x, int* y, int* width, int* height ) = 0;
	virtual bool IsActiveApp() = 0;
	virtual bool IsMultiplayer() = 0;

	virtual void PlayStartupVideos() = 0;
	virtual void PlayAVIAndWait( const char* filename ) = 0;
	virtual void SetCursorVisible( bool visible ) = 0;

	virtual bool OnlyLegacyRender() const
	{
		return true;
	}
};

extern IGame* game;

#endif
