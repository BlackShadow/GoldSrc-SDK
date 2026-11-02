#include "engine_apiproxy.h"
#include "client.h"
#include "server.h"
#include "cmd.h"
#include "cvar.h"
#include "zone.h"
#include "sound.h"
#include "conprint.h"
#include "vid.h"
#include "usermsg.h"
#include "vgui_intwrap2.h"
#include <cstdio>

extern vec3_t r_origin;
#if defined( SW_RENDERER )
int vid_stretched;
#endif

int Draw_StringLen( const char* text, unsigned int font );
void SCR_CenterPrint( char* text );

// player info handler
void hudGetPlayerInfo( int ent_num, hud_player_info_t* pinfo )
{
	player_info_t* player;

	g_engdstAddrs.pfnGetPlayerInfo( &ent_num, &pinfo );

	if ( ent_num > cl.maxclients || ent_num < 1 )
	{
		pinfo->name = NULL;
		pinfo->thisplayer = 0;
		return;
	}

	ent_num -= 1; // player list if offset by 1 from ents
	player = &cl.players[ent_num];
	if ( !player->name[0] )
	{
		pinfo->name = NULL;
		pinfo->thisplayer = 0;
		return;
	}

	pinfo->name = player->name;
	pinfo->ping = (short)player->ping;
	pinfo->spectator = (byte)player->spectator;
	pinfo->packetloss = (byte)player->packet_loss;
	pinfo->thisplayer = ent_num == cl.playernum;
	pinfo->topcolor = (short)player->topcolor;
	pinfo->bottomcolor = (short)player->bottomcolor;
	pinfo->model = player->model;
	if ( g_bIsCStrike || g_bIsCZero )
		pinfo->m_nSteamID = player->m_nSteamID;
}

cvar_t* hudRegisterVariable( char* szName, char* szValue, int flags )
{
	cvar_t* cv;

	g_engdstAddrs.pfnRegisterVariable( &szName, &szValue, &flags );

	cv = (cvar_t*)Z_Malloc( sizeof( cvar_t ) );
	cv->name = szName;
	cv->string = szValue;
	cv->flags = flags | FCVAR_CLIENTDLL;
	Cvar_RegisterVariable( cv );
	return cv;
}

float hudGetCvarFloat( char* szName )
{
	cvar_t* cv;

	if ( !szName )
		return 0.0f;

	g_engdstAddrs.pfnGetCvarFloat( &szName );

	cv = Cvar_FindVar( szName );
	if ( !cv )
		return 0.0f;

	return cv->value;
}

char* hudGetCvarString( char* szName )
{
	cvar_t* cv;

	if ( !szName )
		return NULL;

	g_engdstAddrs.pfnGetCvarString( &szName );

	cv = Cvar_FindVar( szName );
	if ( !cv )
		return NULL;

	return cv->string;
}

int hudAddCommand( char* cmd_name, xcommand_t function )
{
	g_engdstAddrs.pfnAddCommand( &cmd_name, &function );
	Cmd_AddHUDCommand( cmd_name, function );
	return 1;
}

int hudHookUserMsg( char* szMsgName, pfnUserMsgHook pfn )
{
	g_engdstAddrs.pfnHookUserMsg( &szMsgName, &pfn );
	return HookServerMsg( szMsgName, pfn ) != NULL;
}

int hudServerCmd( char* pszCmdString )
{
	char buf[2048];

	g_engdstAddrs.pfnServerCmd( &pszCmdString );

	// just like the client typed "cmd xxxxx" at the console
	snprintf( buf, sizeof( buf ), "cmd %s", pszCmdString );
	Cmd_TokenizeString( buf );
	Cmd_ForwardToServer();
	return 0;
}

int hudClientCmd( char* pszCmdString )
{
	g_engdstAddrs.pfnClientCmd( &pszCmdString );

	if ( !pszCmdString )
		return 0;

	Cbuf_AddText( pszCmdString );
	Cbuf_AddText( "\n" );
	return 1;
}

int hudServerCmdUnreliable( char* pszCmdString )
{
	char buf[2048];

	g_engdstAddrs.pfnServerCmdUnreliable( &pszCmdString );

	snprintf( buf, sizeof( buf ), "cmd %s", pszCmdString );
	Cmd_TokenizeString( buf );
	return Cmd_ForwardToServerUnreliable();
}

static float hudClampSoundVolume( float volume )
{
	if ( !( volume > 0.0f ) )
		return 0.0f;
	return volume < 1.0f ? volume : 1.0f;
}

void hudPlaySoundByName( char* szSound, float volume )
{
	sfx_t* sfx;

	g_engdstAddrs.pfnPlaySoundByName( &szSound, &volume );

	volume = hudClampSoundVolume( volume );
	sfx = S_PrecacheSound( szSound );
	if ( !sfx )
	{
		Con_DPrintf( "invalid sound %s\n", szSound );
		return;
	}

	S_StartDynamicSound( cl.viewentity, CHAN_ITEM, sfx, r_origin, volume, 1.0f, 0, PITCH_NORM );
}

void hudPlaySoundByIndex( int iSound, float volume )
{
	g_engdstAddrs.pfnPlaySoundByIndex( &iSound, &volume );

	volume = hudClampSoundVolume( volume );
	if ( (unsigned int)iSound >= MAX_SOUNDS )
	{
		Con_DPrintf( "invalid sound %i\n", iSound );
		return;
	}

	S_StartDynamicSound( cl.viewentity, CHAN_ITEM, cl.sound_precache[iSound], r_origin, volume, 1.0f, 0, PITCH_NORM );
}

void hudPlaySoundByNameAtLocation( char* szSound, float volume, float* origin )
{
	sfx_t* sfx;

	g_engdstAddrs.pfnPlaySoundByNameAtLocation( &szSound, &volume, &origin );

	volume = hudClampSoundVolume( volume );
	sfx = CL_LookupSound( szSound );
	if ( sfx )
		S_StartDynamicSound( cl.viewentity, CHAN_AUTO, sfx, origin, volume, 1.0f, 0, PITCH_NORM );
}

void hudPlaySoundVoiceByName( char* szSound, float volume, int pitch )
{
	sfx_t* sfx;

	g_engdstAddrs.pfnPlaySoundVoiceByName( &szSound, &volume );

	volume = hudClampSoundVolume( volume );
	sfx = S_PrecacheSound( szSound );
	if ( !sfx )
	{
		Con_DPrintf( "invalid sound %s\n", szSound );
		return;
	}

	S_StartDynamicSound( cl.viewentity, CHAN_BOT, sfx, r_origin, volume, 1.0f, 0, pitch > 0 ? pitch : 0 );
}

void hudPlaySoundByNameAtPitch( char* szSound, float volume, int pitch )
{
	sfx_t* sfx;

	g_engdstAddrs.pfnPlaySoundByName( &szSound, &volume );

	volume = hudClampSoundVolume( volume );
	sfx = S_PrecacheSound( szSound );
	if ( !sfx )
	{
		Con_DPrintf( "invalid sound %s\n", szSound );
		return;
	}

	S_StartDynamicSound( cl.viewentity, CHAN_ITEM, sfx, r_origin, volume, 1.0f, 0, pitch > 0 ? pitch : 0 );
}

int hudGetScreenInfo( SCREENINFO* pscrinfo )
{
	g_engdstAddrs.pfnGetScreenInfo( &pscrinfo );

	if ( !pscrinfo || pscrinfo->iSize != sizeof( SCREENINFO ) )
		return 0;

	pscrinfo->iWidth = (int)vid.width;
	pscrinfo->iHeight = (int)vid.height;
#if defined( SW_RENDERER )
	pscrinfo->iFlags = vid_stretched ? SCRINFO_STRETCHED : 0;
#else
	pscrinfo->iFlags = SCRINFO_SCREENFLASH;
#endif
	pscrinfo->iCharHeight = VGUI2_MessageFontInfo( pscrinfo->charWidths, VGUI2_GetCreditsFont() );
	return sizeof( SCREENINFO );
}

void hudDrawConsoleStringLen( const char* string, int* length, int* height )
{
	g_engdstAddrs.pfnDrawConsoleStringLen( &string, &length, &height );

	*length = Draw_StringLen( string, VGUI2_GetConsoleFont() );
	*height = VGUI2_GetFontTall( VGUI2_GetConsoleFont() );
}

void hudConsolePrint( const char* string )
{
	g_engdstAddrs.pfnConsolePrint( &string );
	Con_Printf( "%s", string );
}

void hudCenterPrint( const char* string )
{
	g_engdstAddrs.pfnConsolePrint( &string );
	SCR_CenterPrint( (char*)string );
}

int hudFilteredClientCmd( char* pszCmdString )
{
	g_engdstAddrs.pfnFilteredClientCmd( &pszCmdString );

	if ( !pszCmdString || !ValidStuffText( pszCmdString ) )
		return 0;

	Cbuf_AddTextToUntrustedBuffer( pszCmdString );
	Cbuf_AddTextToUntrustedBuffer( "\n" );
	return 1;
}

void hudGetClientOrigin( float* origin )
{
	cl_entity_t* ent;

	ent = &cl_entities[cl.viewentity];
	if ( ent->index - 1 == cl.playernum )
	{
		VectorCopy( cl.simorg, origin );
	}
	else
	{
		VectorCopy( ent->origin, origin );
	}
}
