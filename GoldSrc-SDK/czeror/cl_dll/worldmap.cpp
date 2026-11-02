#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui/iczeroviewport.h"

DECLARE_MESSAGE( m_WorldMap, MapInfo )
DECLARE_COMMAND( m_WorldMap, ShowMap )
DECLARE_COMMAND( m_WorldMap, HideMap )

int CHudWorldMap::Init()
{
	gHUD.AddHudElem( this );
	HOOK_COMMAND( "+showmap", ShowMap );
	HOOK_COMMAND( "-showmap", HideMap );
	HOOK_MESSAGE( MapInfo );
	InitHUDData();
	return 1;
}

int CHudWorldMap::VidInit()
{
	return 1;
}

int CHudWorldMap::Draw( float )
{
	return 1;
}

void CHudWorldMap::Think()
{
}

int CHudWorldMap::MsgFunc_MapInfo( const char*, int size, void* buffer )
{
	m_iFlags |= HUD_ACTIVE;
	BEGIN_READ( buffer, size );
	gCzeroViewPortInterface->ShowWorldMap();
	return 1;
}

void CHudWorldMap::UserCmd_ShowMap()
{
	gCzeroViewPortInterface->ShowWorldMap();
}

void CHudWorldMap::UserCmd_HideMap()
{
	gCzeroViewPortInterface->HideWorldMap();
}
