/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "dod_shared.h"
#include "demo.h"
#include "demo_api.h"
#include "r_studioint.h"
#include "event_api.h"
#include "r_efx.h"
#pragma warning( push )
#pragma warning( disable: 4100 )
#include "../../game_shared/voice_status.h"
#pragma warning( pop )
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "../../vgui2/controls/controls.h"
#include "../public/game_controls/iviewport.h"
#include "../public/game_controls/iviewportmsgs.h"
#include "vgui/idodviewport.h"
#include "vgui/idodviewportmsgs.h"
#include "in_buttons.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern engine_studio_api_t IEngineStudio;
extern Vector v_origin;
extern cvar_t *sensitivity;
extern int g_iDeadFlag;
extern float g_demozoom;
extern const char *GetStringTeamColor( int team );
extern cvar_t *cl_dmsmallmap, *cl_dmshowmarkers, *cl_dmshowplayers, *cl_dmshowflags;
extern cvar_t *cl_dmshowobjects, *cl_dmshowgrenades, *cl_numshotrubble, *cl_weatherdis;
extern unsigned int _engineFont;

unsigned int hudfont;
float g_fUser4;
int g_iVuser1x;
int g_iVuser1z;
int g_iMovetype;
int g_iEffects;
int g_iOnlyClientDraw;
float g_lastFOV;
float g_fStamina;
int g_iWeaponBits2;
hud_player_info_t g_PlayerInfoList[65];
extra_player_info_t g_PlayerExtraInfo[65];
pmodel_fx_t g_PModelFxInfo[65];
cvar_t *cl_autoreload;
static cvar_t *violence_hblood;

class CDoDVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	void GetPlayerTextColor( int index, int color[3] ) override
	{
		color[0] = color[1] = color[2] = 255;
		if ( (unsigned int)index <= 64 )
		{
			vgui2::IScheme *scheme = vgui2::scheme()->GetIScheme( gViewPortInterface->GetViewPortScheme() );
			Color teamColor = scheme->GetColor( GetStringTeamColor( g_PlayerExtraInfo[index].teamnumber ),
				Color( 255, 255, 255, 0 ) );
			color[0] = teamColor.r();
			color[1] = teamColor.g();
			color[2] = teamColor.b();
		}
	}
	int GetAckIconHeight() override { return 3 * gHUD.m_iFontHeight + 6; }
	bool CanShowSpeakerLabels() override
	{
		return gViewPortInterface && !gViewPortInterface->IsScoreBoardVisible();
	}
};

static CDoDVoiceStatusHelper g_VoiceStatusHelper;

#define HUD_MESSAGE( name ) \
	int __MsgFunc_##name( const char *message, int size, void *buffer ) \
	{ return gHUD.MsgFunc_##name( message, size, buffer ); }

HUD_MESSAGE( Logo )
HUD_MESSAGE( ResetHUD )
HUD_MESSAGE( YouDied )
HUD_MESSAGE( SetFOV )
HUD_MESSAGE( HLTV )
HUD_MESSAGE( RoundState )
HUD_MESSAGE( TimeLeft )
HUD_MESSAGE( UseSound )

int __MsgFunc_InitHUD( const char *name, int size, void *buffer )
{
	gHUD.MsgFunc_InitHUD( name, size, buffer );
	return 1;
}

#define VIEWPORT_MESSAGE( name ) \
	int __MsgFunc_##name( const char *message, int size, void *buffer ) \
	{ return gViewPortMsgs ? gViewPortMsgs->MsgFunc_##name( message, size, buffer ) : 0; }

VIEWPORT_MESSAGE( MOTD )
VIEWPORT_MESSAGE( RandomPC )
VIEWPORT_MESSAGE( ServerName )
VIEWPORT_MESSAGE( Spectator )
VIEWPORT_MESSAGE( AllowSpec )
VIEWPORT_MESSAGE( TeamNames )
VIEWPORT_MESSAGE( VGUIMenu )

#define DOD_MESSAGE( name ) \
	int __MsgFunc_##name( const char *message, int size, void *buffer ) \
	{ return gDoDViewPortMsgs ? gDoDViewPortMsgs->MsgFunc_##name( message, size, buffer ) : 0; }

DOD_MESSAGE( ScoreInfo )
DOD_MESSAGE( ScoreInfoLong )
DOD_MESSAGE( TeamScore )
DOD_MESSAGE( MapMarker )
DOD_MESSAGE( WaveTime )
DOD_MESSAGE( WaveStatus )
DOD_MESSAGE( WideScreen )
DOD_MESSAGE( Frags )
DOD_MESSAGE( ObjScore )
DOD_MESSAGE( PStatus )
DOD_MESSAGE( ScoreShort )
DOD_MESSAGE( PClass )
DOD_MESSAGE( PTeam )
DOD_MESSAGE( CurMarker )

void __CmdFunc_ShowCommandMenu()
{
	if ( gViewPortInterface )
		gViewPortInterface->ShowCommandMenu();
}

void __CmdFunc_HideCommandMenu()
{
	if ( gViewPortInterface )
		gViewPortInterface->HideCommandMenu();
}

void COM_FileBase( const char *in, char *out )
{
	int end = (int)strlen( in ) - 1;
	int extension = end;
	while ( extension > 0 && in[extension] != '.' && in[extension] != '/' && in[extension] != '\\' )
		--extension;
	if ( in[extension] == '.' )
		--extension;
	else
		extension = end;
	int start = end;
	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		--start;
	++start;
	int length = extension - start + 1;
	strncpy( out, in + start, length );
	out[length] = 0;
}

int HUD_IsGame( const char *game )
{
	const char *directory = gEngfuncs.pfnGetGameDirectory();
	if ( !directory || !directory[0] )
		return 0;
	char name[1024];
	COM_FileBase( directory, name );
	return !stricmp( name, game );
}

float HUD_GetFOV()
{
	if ( gEngfuncs.pDemoAPI->IsRecording() )
	{
		float fov = g_lastFOV;
		Demo_WriteBuffer( 1, sizeof( fov ), (unsigned char *)&fov );
	}
	if ( gEngfuncs.pDemoAPI->IsPlayingback() )
		g_lastFOV = g_demozoom;
	return g_lastFOV;
}

void ClientSetSensitivity( int level )
{
	gHUD.m_iSensLevel = level;
}

int CHud::GetSpriteIndex( const char *name )
{
	// look through the loaded sprite name list for SpriteName
	for ( int i = 0; i < m_iSpriteCount; ++i )
	{
		if ( !strncmp( name, m_rgszSpriteNames + i * MAX_SPRITE_NAME_LENGTH, MAX_SPRITE_NAME_LENGTH ) )
			return i;
	}
	return -1; // invalid sprite
}

void CHud::AddHudElem( CHudBase *element )
{
	if ( !element )
		return;
	HUDLIST *node = (HUDLIST *)malloc( sizeof( HUDLIST ) );
	if ( !node )
		return;
	node->pNext = NULL;
	node->p = element;
	if ( !m_pHudList )
		m_pHudList = node;
	else
	{
		HUDLIST *last = m_pHudList;
		while ( last->pNext )
			last = last->pNext;
		last->pNext = node;
	}
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud::~CHud()
{
	delete[] m_rghSprites;
	delete[] m_rgrcRects;
	delete[] m_rgszSpriteNames;
	while ( m_pHudList )
	{
		HUDLIST *node = m_pHudList;
		m_pHudList = node->pNext;
		free( node );
	}
	g_RubbleQueue.Update( 99999 );
}

int CHud::MsgFunc_Logo( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );

	// update Train data
	m_iLogo = READ_BYTE();
	return 1;
}

int CHud::MsgFunc_SetFOV( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	if ( g_iVuser1z )
		return m_iFOV;
	int fov = READ_BYTE();
	g_lastFOV = (float)fov;
	m_iFOV = fov ? fov : 90;
	if ( m_iFOV == 90 )
		m_flMouseSensitivity = 0;
	else
		m_flMouseSensitivity = (float)((zoom_sensitivity_ratio ? zoom_sensitivity_ratio->value : 1.0) *
			(fov / 90.0 * sensitivity->value));
	return 1;
}

int CHud::MsgFunc_RoundState( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	m_iRoundState = READ_BYTE();
	return 1;
}

int CHud::MsgFunc_TimeLeft( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	float time = gHUD.m_flTime;
	m_fRoundEndsTime = READ_SHORT() + time;
	return 1;
}

int CHud::MsgFunc_HLTV( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	int player = READ_BYTE();
	int value = READ_BYTE();
	if ( !gEngfuncs.IsSpectateOnly() )
	{
		gEngfuncs.Con_DPrintf( "Warning! Received unexpected HLTV user message.\n" );
		return 1;
	}
	if ( value & 0x80 )
	{
		value &= 0x7F;
		if ( player )
			g_PlayerExtraInfo[player].health = value;
		else
			for ( int i = 0; i < 64; ++i )
				g_PlayerExtraInfo[i].health = value;
	}
	else
	{
		if ( !value )
			value = 90;
		if ( player )
			m_PlayerFOV[player] = value;
		else
			for ( int i = 0; i < 64; ++i )
				m_PlayerFOV[i] = value;
	}
	return 1;
}

int CHud::MsgFunc_UseSound( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	gEngfuncs.pfnPlaySoundByName( READ_BYTE() ? "common/wpn_select.wav" : "common/wpn_denyselect.wav", 0.5f );
	return 1;
}

float CHud::GetSensitivity()
{
	return m_iSensLevel == 1 ? 1.0f : m_flMouseSensitivity;
}

bool CHud::IsTeamPara( int team )
{
	return team == 1 ? m_bAllieParatrooper : m_bAxisParatrooper;
}

char *CHud::GetTeamName( int team )
{
	return m_szTeamNames[team == 1 ? 1 : team == 2 ? 2 : 3];
}

char *CHud::GetPlayerClassName( int playerclass )
{
	bool paratrooper = (unsigned int)(playerclass - 10) <= 10 ? m_bAxisParatrooper : m_bAllieParatrooper;
	return paratrooper ? g_ParaClassInfo[playerclass].classname : g_ClassInfo[playerclass].classname;
}

int CHud::GetCurrentWeaponId() { return m_Ammo.GetCurrentWeaponId(); }
void CHud::SetWaterLevel( int level ) { m_iWaterLevel = level; }
int CHud::GetWaterLevel() { return m_iWaterLevel; }
bool CHud::IsInMGDeploy() { return g_iUser3 == 2 || g_iVuser1x == 2; }
bool CHud::IsProneDeployed() { return g_iUser3 == 2; }
bool CHud::IsSandbagDeployed() { return g_iVuser1x == 2; }
bool CHud::IsProne() { return (unsigned int)(g_iUser3 - 1) <= 1; }
bool CHud::IsDucking() { return (gHUD.m_iKeyBits & IN_DUCK) != 0; }
bool CHud::IsInMortarDeploy() { return !g_iDeadFlag && g_iUser3 == 3; }
void CHud::SetMortarDeployTime() { m_fMortarDeployTime = gEngfuncs.GetClientTime(); }
float CHud::GetMortarDeployTime() { return m_fMortarDeployTime; }
void CHud::SetMortarUnDeployTime() { m_fMortarDeployTime = gEngfuncs.GetClientTime(); }
float CHud::GetMortarUnDeployTime() { return m_fMortarDeployTime; }
void CHud::PostMortarValue( float value ) { gDoDViewPortInterface->PostMortarValue( value ); }

void CHud::PlaySoundOnChan( char *name, float volume, int )
{
	gEngfuncs.pfnPlaySoundByName( name, volume );
}

void CHud::VGUI2HudPrint( char *text, int x, int y, float red, float green, float blue )
{
	m_VGUI2Print.VGUI2HudPrint( text, x, y, red, green, blue );
}

int CHud::GetMinimapZoomLevel() { return (int)_cl_minimapzoom->value; }

int CHud::ZoomMinimap()
{
	_cl_minimapzoom->value += 1;
	if ( _cl_minimapzoom->value >= 3 )
		_cl_minimapzoom->value = 0;
	return (int)_cl_minimapzoom->value;
}

int CHud::GetMinimapState()
{
	return IEngineStudio.IsHardware() && m_iFOV == 90 ? (int)_cl_minimap->value : 0;
}

void CHud::SetMinimapState( int state )
{
	if ( IEngineStudio.IsHardware() )
		gEngfuncs.Cvar_SetValue( "_cl_minimap", (float)state );
	else
		_cl_minimap->value = 0;
}

void CHud::InitMapBounds()
{
	m_iMapWidth = (int)(ScreenWidth * 0.625);
	m_iMapHeight = (int)(ScreenHeight * 0.625);
	m_iMapX = ScreenWidth / 2 - m_iMapWidth / 2;
	m_iMapY = ScreenHeight / 2 - m_iMapHeight / 2;
	m_iSmallMapWidth = (int)(ScreenWidth * 0.24);
	m_iSmallMapHeight = (int)(ScreenHeight * 0.24);
	m_iSmallMapX = ScreenWidth - m_iSmallMapWidth - (int)(float)(ScreenWidth / 640.0 + ScreenWidth / 640.0 + 0.5);
	m_iSmallMapY = (int)(float)(ScreenHeight / 480.0 + ScreenHeight / 480.0 + 0.5);
}

void CHud::GetMapBounds( int &x, int &y, int &width, int &height )
{
	int state = GetMinimapState();
	if ( state == 1 )
	{
		x = m_iMapX;
		y = m_iMapY;
		width = m_iMapWidth;
		height = m_iMapHeight;
	}
	else if ( state == 2 )
	{
		x = m_iSmallMapX;
		y = m_iSmallMapY;
		if ( g_iUser1 )
			y += (int)(float)(ScreenHeight / 480.0 * 54.0 + 0.5);
		width = m_iSmallMapWidth;
		height = m_iSmallMapHeight;
	}
	else
		x = y = width = height = 0;
}

void CHud::GetWeaponRecoilAmount( int weapon, float &pitch, float &yaw )
{
	pitch = 0;
	yaw = 2;
	switch ( weapon )
	{
	case WEAPON_COLT:
	case WEAPON_LUGER:
	case WEAPON_M1CARBINE:
	case WEAPON_WEBLEY: pitch = 1.4f; yaw = 0.35f; break;
	case WEAPON_GARAND:
	case WEAPON_KAR: pitch = 8; yaw = 2; break;
	case WEAPON_SCOPEDKAR: pitch = 6; yaw = 1.5f; break;
	case WEAPON_THOMPSON:
	case WEAPON_GREASEGUN: pitch = 2.15f; yaw = 0.5375f; break;
	case WEAPON_MP44: pitch = 5; yaw = 1.25f; break;
	case WEAPON_SPRING: pitch = 5.6f; yaw = 1.4f; break;
	case WEAPON_BAR:
	case WEAPON_BREN: pitch = 6.72f; yaw = 1.3f; break;
	case WEAPON_MP40:
	case WEAPON_STEN: pitch = 2.2f; yaw = 0.55f; break;
	case WEAPON_MG42:
	case WEAPON_30CAL:
	case WEAPON_MG34: pitch = 20; yaw = 5; break;
	case WEAPON_FG42: pitch = 5.3f; yaw = 1.325f; break;
	case WEAPON_K43: pitch = 7; yaw = 1.75f; break;
	case WEAPON_ENFIELD:
		pitch = (g_iWeaponFlags & 1) ? 6.0f : 8.0f;
		yaw = (g_iWeaponFlags & 1) ? 1.5f : 2.0f;
		break;
	case WEAPON_BAZOOKA:
	case WEAPON_PSCHRECK:
	case WEAPON_PIAT: pitch = 10; yaw = 2.5f; break;
	default: pitch = yaw = 0; break;
	}
	bool machinegun = weapon == WEAPON_MG42 || weapon == WEAPON_30CAL || weapon == WEAPON_MG34;
	if ( IsInMGDeploy() && (machinegun || weapon == WEAPON_FG42 || weapon == WEAPON_BAR || weapon == WEAPON_BREN) )
		pitch = yaw = 0;
	else if ( IsProne() && !machinegun )
	{
		pitch *= 0.25f;
		yaw *= 0.25f;
	}
	else if ( IsDucking() )
	{
		pitch *= 0.5f;
		yaw *= 0.5f;
	}
}

void CHud::SetRecoilAmount( float pitch, float yaw )
{
	m_flPitchRecoilAccumulator = pitch;
	float amount = gEngfuncs.pfnRandomFloat( 0.8f, 1.1f ) * yaw;
	if ( gEngfuncs.pfnRandomLong( 0, 1 ) > 0 )
		amount = -amount;
	m_flYawRecoilAccumulator = amount;
	m_flRecoilTimeRemaining = 0.1f;
}

void CHud::DoRecoil( int weapon )
{
	float pitch, yaw;
	GetWeaponRecoilAmount( weapon, pitch, yaw );
	SetRecoilAmount( pitch, yaw );
}

void CHud::PopRecoil( float time, float &pitch, float &yaw )
{
	if ( m_flRecoilTimeRemaining <= 0 )
	{
		pitch = yaw = 0;
		return;
	}
	float amount = (float)((time <= m_flRecoilTimeRemaining ? time : m_flRecoilTimeRemaining) / 0.1);
	pitch = m_flPitchRecoilAccumulator * amount;
	yaw = amount * m_flYawRecoilAccumulator;
	m_flRecoilTimeRemaining -= time;
}

bool ShouldShowBlood()
{
	if ( !violence_hblood )
		violence_hblood = gEngfuncs.pfnGetCvarPointer( "violence_hblood" );
	return violence_hblood && violence_hblood->value == 1;
}

void EV_BloodPuff( float *org )
{
	int sprite = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/shot-dust.spr" );
	Vector origin( org );
	Vector velocity( 0, 0, 2 );
	float scale = gEngfuncs.pfnRandomFloat( 0.7f, 0.8f );
	gEngfuncs.pfnRandomFloat( 0.4f, 0.45f );
	float distance = (v_origin - origin).Length();
	if ( distance < 32 )
		return;
	float multiplier = distance > 1200 ? 1.5f : distance > 600 ? (distance - 600) / 600 * 0.5f + 1 : 1;
	TEMPENTITY *dust = gEngfuncs.pEfxAPI->R_TempSprite( origin, velocity, multiplier * scale, sprite,
		kRenderTransAlpha, kRenderFxNone, 0.15f, 30, FTENT_SPRANIMATE );
	dust->entity.curstate.renderamt = 800;
	dust->entity.curstate.framerate = 45;
	dust->entity.curstate.rendercolor.r = 180;
	dust->entity.curstate.rendercolor.g = 176;
	dust->entity.curstate.rendercolor.b = 148;
	dust->entity.angles.z = (float)gEngfuncs.pfnRandomLong( 0, 90 );
	Vector toView = v_origin - origin;
	toView = toView.Normalize();
	Vector closerOrigin = origin + toView * 2;
	sprite = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/blood-narrow.spr" );
	TEMPENTITY *blood = gEngfuncs.pEfxAPI->R_TempSprite( closerOrigin, velocity, scale * multiplier, sprite,
		kRenderTransAlpha, kRenderFxNone, 0.35f, 30, FTENT_SPRANIMATE );
	blood->entity.curstate.framerate = 20;
	blood->entity.curstate.renderamt = 800;
	blood->entity.curstate.rendercolor.r = 120;
	blood->entity.curstate.rendercolor.g = 0;
	blood->entity.curstate.rendercolor.b = 0;
	blood->entity.angles.z = (float)gEngfuncs.pfnRandomLong( 0, 90 );
}

int EV_BloodPuffMsg( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	Vector origin;
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	if ( ShouldShowBlood() )
		EV_BloodPuff( origin );
	return 1;
}

int __MsgFunc_BloodPuff( const char *name, int size, void *buffer )
{
	return EV_BloodPuffMsg( name, size, buffer );
}

int EV_HandSignalMsg( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	int player = READ_BYTE();
	unsigned int signal = READ_BYTE();
	if ( (unsigned int)(player - 1) <= 63 &&
		signal < sizeof( s_HandSignalSubtitles ) / sizeof( s_HandSignalSubtitles[0] ) )
	{
		int team = g_PlayerExtraInfo[player].teamId;
		gEngfuncs.pfnGetPlayerInfo( player, &g_PlayerInfoList[player] );
		char pattern[256];
		sprintf( pattern, "%c%s%s%s\n", 2, "(%s1) ", g_PlayerInfoList[player].name, ": %s2" );
		char *subtitle = s_HandSignalSubtitles[signal][0];
		if ( team == 2 && s_HandSignalSubtitles[signal][1][0] )
			subtitle = s_HandSignalSubtitles[signal][1];
		else if ( team == 1 && gHUD.m_bBritish && s_HandSignalSubtitles[signal][2][0] )
			subtitle = s_HandSignalSubtitles[signal][2];
		gHUD.m_SayText.SayTextPrint( pattern, 256, player, "#Handsignal", subtitle, NULL, NULL );
		gHUD.m_Spectator.AddVoiceIconToPlayerEnt( player );
	}
	return 1;
}

int __MsgFunc_HandSignal( const char *name, int size, void *buffer )
{
	return EV_HandSignalMsg( name, size, buffer );
}

// This is called every time the DLL is loaded
void CHud::Init()
{
	HOOK_MESSAGE( Logo );
	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( YouDied );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( SetFOV );
	HOOK_MESSAGE( HLTV );
	HOOK_MESSAGE( BloodPuff );
	HOOK_MESSAGE( HandSignal );
	HOOK_MESSAGE( UseSound );
	HOOK_COMMAND( "+commandmenu", ShowCommandMenu );
	HOOK_COMMAND( "-commandmenu", HideCommandMenu );
	HOOK_MESSAGE( TeamNames );
	HOOK_MESSAGE( MOTD );
	HOOK_MESSAGE( RandomPC );
	HOOK_MESSAGE( ServerName );
	HOOK_MESSAGE( ScoreInfo );
	HOOK_MESSAGE( ScoreInfoLong );
	HOOK_MESSAGE( TeamScore );
	HOOK_MESSAGE( Spectator );
	HOOK_MESSAGE( AllowSpec );
	HOOK_MESSAGE( MapMarker );

	// VGUI Menus
	HOOK_MESSAGE( VGUIMenu );
	HOOK_MESSAGE( WaveTime );
	HOOK_MESSAGE( WaveStatus );
	HOOK_MESSAGE( WideScreen );
	HOOK_MESSAGE( Frags );
	HOOK_MESSAGE( ObjScore );
	HOOK_MESSAGE( PStatus );
	HOOK_MESSAGE( ScoreShort );
	HOOK_MESSAGE( PClass );
	HOOK_MESSAGE( PTeam );
	HOOK_MESSAGE( RoundState );
	HOOK_MESSAGE( CurMarker );
	HOOK_MESSAGE( TimeLeft );
	hud_takesshots = CVAR_CREATE( "hud_takesshots", "0", FCVAR_ARCHIVE );
	max_rubble = CVAR_CREATE( "max_rubble", "240", 0 );
	cl_corpsestay = CVAR_CREATE( "cl_corpsestay", "10", FCVAR_ARCHIVE );
	cl_dmsmallmap = CVAR_CREATE( "cl_dmsmallmap", "1", FCVAR_ARCHIVE );
	cl_dmshowmarkers = CVAR_CREATE( "cl_dmshowmarkers", "1", FCVAR_ARCHIVE );
	cl_dmshowplayers = CVAR_CREATE( "cl_dmshowplayers", "1", FCVAR_ARCHIVE );
	cl_dmshowflags = CVAR_CREATE( "cl_dmshowflags", "1", FCVAR_ARCHIVE );
	cl_dmshowobjects = CVAR_CREATE( "cl_dmshowobjects", "1", FCVAR_ARCHIVE );
	cl_dmshowgrenades = CVAR_CREATE( "cl_dmshowgrenades", "1", FCVAR_ARCHIVE );
	cl_numshotrubble = CVAR_CREATE( "cl_numshotrubble", "5", FCVAR_ARCHIVE );
	cl_weatherdis = CVAR_CREATE( "cl_weatherdis", "1700", FCVAR_ARCHIVE );
	_cl_minimap = CVAR_CREATE( "_cl_minimap", "2", FCVAR_ARCHIVE | FCVAR_USERINFO );
	_cl_minimapzoom = CVAR_CREATE( "_cl_minimapzoom", "1", FCVAR_ARCHIVE );
	zoom_sensitivity_ratio = CVAR_CREATE( "zoom_sensitivity_ratio", "1.2", FCVAR_ARCHIVE );
	_ah = CVAR_CREATE( "_ah", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );
	cl_hudfont = CVAR_CREATE( "cl_hudfont", "1", FCVAR_ARCHIVE );
	hud_fastswitch = CVAR_CREATE( "hud_fastswitch", "0", FCVAR_ARCHIVE );
	cl_lw = gEngfuncs.pfnGetCvarPointer( "cl_lw" );
	r_drawentities = gEngfuncs.pfnGetCvarPointer( "r_drawentities" );
	cl_pitchdown = gEngfuncs.pfnGetCvarPointer( "cl_pitchdown" );
	cl_pitchup = gEngfuncs.pfnGetCvarPointer( "cl_pitchup" );
	crosshair = gEngfuncs.pfnGetCvarPointer( "crosshair" );
	developer = gEngfuncs.pfnGetCvarPointer( "developer" );
	m_iFOV = 0;
	m_iLogo = 0;
	m_bAutoReloadComplete = true;
	m_bAllieParatrooper = false;
	m_bAllieInfiniteLives = true;
	m_bAxisParatrooper = false;
	m_bAxisInfiniteLives = true;
	m_bParatrooper = false;
	m_bInfiniteLives = true;
	m_bBritish = false;
	m_iRoundState = 1;
	m_pCvarStealMouse = CVAR_CREATE( "hud_capturemouse", "1", FCVAR_ARCHIVE );
	m_pCvarDraw = CVAR_CREATE( "hud_draw", "1", FCVAR_ARCHIVE );
	cl_autoreload = CVAR_CREATE( "cl_autoreload", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );
	m_pSpriteList = NULL;

	// Clear any old HUD list
	while ( m_pHudList )
	{
		HUDLIST *node = m_pHudList;
		m_pHudList = node->pNext;
		free( node );
	}
	m_iClipSize = 0;
	i_Recoil = 0;
	m_iWaterLevel = 0;
	m_flTime = 1;
	m_Scope.Init();
	m_DoDCommon.Init();
	m_Icons.Init();
	m_DoDMap.Init();
	m_ObjectiveIcons.Init();
	m_PShooter.Init();
	m_CEnvModel.Init();
	m_Weather.Init();
	m_Ammo.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Train.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	m_Menu.Init();
	m_DoDCrossHair.Init();
	m_DoDMap.Init();
	m_MortarHud.Init();
	m_VGUI2Print.Init();
	GetClientVoice()->Init( &g_VoiceStatusHelper );
	MsgFunc_ResetHUD( NULL, 0, NULL );
	m_iSensLevel = 0;
	m_szTeamNames[0][0] = 0;
	g_RubbleQueue.m_flDuration = 2;
	strcpy( m_szTeamNames[1], CHudTextMessage::BufferedLocaliseTextString( "#Teamname_allies" ) );
	strcpy( m_szTeamNames[2], CHudTextMessage::BufferedLocaliseTextString( "#Teamname_axis" ) );
	strcpy( m_szTeamNames[3], CHudTextMessage::BufferedLocaliseTextString( "#Teamname_spectators" ) );
	strcpy( m_szTeamNames[4], CHudTextMessage::BufferedLocaliseTextString( "#Teamname_british" ) );
	InitMapBounds();
}

void CHud::VidInit()
{
	m_scrinfo.iSize = sizeof( m_scrinfo );
	gEngfuncs.pfnGetScreenInfo( &m_scrinfo );

	// ----------
	// Load Sprites
	// ---------
//	m_hsprFont = LoadSprite("sprites/%d_font.spr");

	m_hsprLogo = 0;
	m_hsprCursor = 0;
	m_iRoundState = 1;
	m_bAutoReloadComplete = true;
	m_iRes = ScreenWidth <= 639 ? 320 : 640;
	m_bAllieParatrooper = false;
	m_bAllieInfiniteLives = true;
	m_bAxisParatrooper = false;
	m_bAxisInfiniteLives = true;
	m_bParatrooper = false;
	m_bInfiniteLives = true;
	m_bBritish = false;
	m_flPlaySprintSoundTime = 0;
	m_fRoundEndsTime = 0;
	m_iFOV = 0;
	g_lastFOV = 0;
	m_fMortarDeployTime = 0;
	m_flPitchRecoilAccumulator = 0;
	m_flYawRecoilAccumulator = 0;
	m_flRecoilTimeRemaining = 0;
	char path[256];
	if ( m_pSpriteList )
	{
		int index = 0;
		for ( int i = 0; i < m_iSpriteCountAllRes; ++i )
		{
			if ( m_pSpriteList[i].iRes == m_iRes )
			{
				sprintf( path, "sprites/%s.spr", m_pSpriteList[i].szSprite );
				m_rghSprites[index++] = SPR_Load( path );
			}
		}
	}
	else
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList( "sprites/hud.txt", &m_iSpriteCountAllRes );
		if ( m_pSpriteList )
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			for ( int i = 0; i < m_iSpriteCountAllRes; ++i )
				if ( m_pSpriteList[i].iRes == m_iRes )
					++m_iSpriteCount;

			// allocated memory for sprite handle arrays
			m_rghSprites = new HSPRITE[m_iSpriteCount];
			m_rgrcRects = new wrect_t[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];
			int index = 0;
			for ( int i = 0; i < m_iSpriteCountAllRes; ++i )
			{
				client_sprite_t *sprite = &m_pSpriteList[i];
				if ( sprite->iRes != m_iRes )
					continue;
				sprintf( path, "sprites/%s.spr", sprite->szSprite );
				m_rghSprites[index] = SPR_Load( path );
				m_rgrcRects[index] = sprite->rc;
				strncpy( m_rgszSpriteNames + index * MAX_SPRITE_NAME_LENGTH, sprite->szName, MAX_SPRITE_NAME_LENGTH );
				++index;
			}
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex( "number_0" );
	m_MG_number_0 = GetSpriteIndex( "mg_0" );
	m_Ammo.VidInit();
	m_Scope.VidInit();
	m_DoDCommon.VidInit();
	m_Icons.VidInit();
	m_DoDMap.VidInit();
	m_ObjectiveIcons.VidInit();
	m_CEnvModel.VidInit();
	m_PShooter.VidInit();
	m_Weather.VidInit();
	m_Train.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_StatusIcons.VidInit();
	m_DoDCrossHair.VidInit();
	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;
	m_iFontEngineHeight = vgui2::surface()->GetFontTall( _engineFont );
	GetClientVoiceHud()->VidInit();
	m_MortarHud.VidInit();
	m_Spectator.VidInit();
	m_VGUI2Print.VidInit();
	for ( int i = 1; i <= 64; ++i )
		g_PlayerExtraInfo[i].teamnumber = 3;
	m_iSensLevel = 0;
	memset( g_PModelFxInfo, 0, sizeof( g_PModelFxInfo ) );
}
