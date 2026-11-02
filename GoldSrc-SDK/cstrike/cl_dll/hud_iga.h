#ifndef HUD_IGA_H
#define HUD_IGA_H
#pragma once

#include "interface.h"

namespace vgui2
{
class Panel;
}

#define ENGINE_INGAMEADVERTISING_INTERFACE_VERSION		"ENGINEINGAMEADVERTISINGVERSION001"
#define CLIENTDLL_INGAMEADVERTISING_INTERFACE_VERSION	"CLIENTDLLINGAMEADVERTISINGVERSION001"

enum
{
	IGA_SCOREBOARD = 0,
	IGA_SPECTATOR,

	IGA_NUM_SPACES
};

struct IGAImpression_t
{
	bool	bVisible;
	int		iSpace;
	int		iInfo1;
	int		iInfo2;
	int		iInfo0;
	float	flCoverage;
};

class IClientDLLInGameAdvertising : public IBaseInterface
{
public:
	virtual void ReportImpression( const IGAImpression_t* impression ) = 0;
	virtual void Reset() = 0;
};

class IEngineInGameAdvertising : public IBaseInterface
{
public:
	virtual void Init( CreateInterfaceFn factory ) = 0;
	virtual void Shutdown() = 0;
	virtual void SetTexture( int space, int info0, int info1, int info2, int wide, int tall, const unsigned char* rgba ) = 0;
};

#define IGA_SPACE_HIDDEN	0x8000

class IClientInGameAdvertisingInternal
{
public:
	virtual vgui2::Panel* CreatePanel( int space, vgui2::Panel* parent ) = 0;
	virtual void UpdatePanel( bool visible, int space ) = 0;
	virtual bool IsPanelReady( int space, bool* shown ) = 0;
	virtual void Reset() = 0;
};

extern IClientInGameAdvertisingInternal* g_pClientIGA;

#endif
