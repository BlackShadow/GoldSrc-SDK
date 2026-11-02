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

#include "modes.h"

class IVideoMode
{
public:
	virtual const char* GetName() = 0;

	virtual bool Init( void* ) = 0;
	virtual void PlayStartupSequence() = 0;
	virtual void Shutdown() = 0;

	virtual bool AddMode( int width, int height, int bitsPerPixel ) = 0;
	virtual vmode_s* GetCurrentMode() = 0;
	virtual vmode_s* GetMode( int index ) = 0;
	virtual int GetModeCount() = 0;

	virtual bool IsWindowedMode() const = 0;
	virtual bool GetInitialized() const = 0;
	virtual void SetInitialized( bool initialized ) = 0;

	virtual void UpdateWindowPosition() = 0;
	virtual void FlipScreen() = 0;
	virtual void RestoreVideo() = 0;
	virtual void ReleaseVideo() = 0;
};

extern IVideoMode* videomode;

extern "C" int	VideoMode_IsWindowed();
extern "C" void	VideoMode_GetCurrentVideoMode( int* width, int* height, int* bitsPerPixel );
void			VideoMode_GetCurrentRenderer( char* name, int nameLength, int* windowed, int* hdModels, int* addons );
void			VideoMode_GetVideoModes( vmode_s** modes, int* count );
extern "C" void	VideoMode_RestoreVideo();
extern "C" void	VideoMode_SetVideoMode( int width, int height, int bitsPerPixel );
extern "C" void	VideoMode_SwitchMode( int hardware, int windowed );
