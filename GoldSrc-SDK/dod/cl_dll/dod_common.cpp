#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "in_buttons.h"
#include "event_api.h"
#include "vgui/idodviewport.h"

extern int i_dodmusic;
extern int g_iinjump;
extern int g_ionground;
extern int g_iDeadFlag;
extern cvar_t *cl_hud_health;
extern cvar_t *cl_hud_reinforcements;
extern cvar_t *cl_hud_ammo;
extern cvar_t *cl_hud_objtimer;
extern cvar_t *cl_hud_objectives;
extern void ClientSetSensitivity( int level );
extern void DoD_ReadCameraSettings( const char *targetname );

bool b_cutscene_return;
int i_lastspectoggle;
float fl_DeadTargetSwitch;
int g_ihidexhair;
float i_ProneCounter;

DECLARE_MESSAGE( m_DoDCommon, GameRules );
DECLARE_MESSAGE( m_DoDCommon, CameraView );
DECLARE_MESSAGE( m_DoDCommon, ResetSens );

void ClientProne()
{
	if ( g_iDeadFlag || g_iVuser1x == 2 || (gHUD.m_iKeyBits & IN_ATTACK) )
		return;
	if ( gHUD.m_iKeyBits & IN_DUCK )
	{
		if ( g_iUser3 <= 0 )
			return;
	}
	else if ( !g_iUser3 && gHUD.GetWaterLevel() > 1 )
		return;
	if ( g_iMovetype != MOVETYPE_FLY && gHUD.m_flTime > i_ProneCounter )
	{
		gEngfuncs.pfnServerCmd( "sprone" );
		if ( g_iUser3 <= 1 )
		{
			i_ProneCounter = gHUD.m_flTime + 1.5f;
			if ( !g_iUser3 )
			{
				cl_entity_t *player = gEngfuncs.GetLocalPlayer();
				gEngfuncs.pEventAPI->EV_PlaySound( player->index, player->origin, CHAN_VOICE,
					"player/goprone.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			}
		}
	}
}

int CHudDoDCommon::Init()
{
	HOOK_MESSAGE( GameRules );
	HOOK_MESSAGE( CameraView );
	HOOK_MESSAGE( ResetSens );
	gEngfuncs.pfnAddCommand( "prone", ClientProne );
	gHUD.i_specmenutoggle = 1;
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem( this );
	return 1;
}

void CHudDoDCommon::InitHUDData()
{
	i_dodmusic = 1;
}

int CHudDoDCommon::VidInit()
{
	b_cutscene_return = false;
	g_iinjump = 0;
	g_ihidexhair = 0;
	i_ProneCounter = 0;
	return 1;
}

int CHudDoDCommon::Draw( float )
{
	if ( g_iinjump && g_ionground )
		g_iinjump = 0;
	if ( g_iVuser1z )
		b_cutscene_return = true;
	if ( g_iTeamNumber == 1 )
	{
		gHUD.m_bParatrooper = gHUD.m_bAllieParatrooper;
		gHUD.m_bInfiniteLives = gHUD.m_bAllieInfiniteLives;
	}
	else if ( g_iTeamNumber == 2 )
	{
		gHUD.m_bParatrooper = gHUD.m_bAxisParatrooper;
		gHUD.m_bInfiniteLives = gHUD.m_bAxisInfiniteLives;
	}
	if ( g_iUser1 && g_iUser2 )
	{
		if ( g_PlayerExtraInfo[g_iUser2].dead )
		{
			if ( fl_DeadTargetSwitch && gHUD.m_flTime > fl_DeadTargetSwitch )
			{
				gEngfuncs.pfnClientCmd( "+attack;wait;-attack" );
				fl_DeadTargetSwitch = 0;
			}
			else if ( !fl_DeadTargetSwitch )
				fl_DeadTargetSwitch = gHUD.m_flTime + 4;
		}
		else
			fl_DeadTargetSwitch = 0;
	}
	if ( b_cutscene_return && !g_iVuser1z )
	{
		gHUD.m_Scope.SetScope( 0 );
		b_cutscene_return = false;
	}
	if ( gDoDViewPortInterface->isWidescreenVisible() )
	{
		g_lastFOV = 115;
		gHUD.m_iFOV = 115;
	}
	else if ( !gDoDViewPortInterface->isWidescreenVisible() && gHUD.m_iFOV == 115 )
	{
		g_lastFOV = 0;
		gHUD.m_iFOV = 90;
	}
	if ( gDoDViewPortInterface->isWidescreenVisible() && !g_iUser1 && !g_iVuser1z )
		gDoDViewPortInterface->setWidescreenVisible( false );
	return 1;
}

int CHudDoDCommon::MsgFunc_GameRules( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	int allie = READ_BYTE();
	int axis = READ_BYTE();
	gHUD.m_bAllieParatrooper = (allie & 1) != 0;
	gHUD.m_bAllieInfiniteLives = (allie & 2) != 0;
	gHUD.m_bBritish = (allie & 4) != 0;
	gHUD.m_bAxisParatrooper = (axis & 1) != 0;
	gHUD.m_bAxisInfiniteLives = (axis & 2) != 0;
	gHUD.m_ObjectiveIcons.m_bWarmupMode = (axis & 4) != 0;
	if ( gHUD.m_bBritish )
		gDoDViewPortInterface->updateTeamMenu();
	return 1;
}

int CHudDoDCommon::MsgFunc_ResetSens( const char *, int, void * )
{
	ClientSetSensitivity( 0 );
	return 1;
}

int CHudDoDCommon::MsgFunc_CameraView( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	DoD_ReadCameraSettings( READ_STRING() );
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

float DoDGunGoOnOffScreen( float offset, struct ref_params_s * )
{
	int weapon = gHUD.GetCurrentWeaponId();
	if ( weapon == -1 || g_iUser3 == 3 || gEngfuncs.IsSpectateOnly() || g_iUser1 || g_iUser2 )
		return 0;
	int keys = gHUD.m_iKeyBits;
	bool hide = ((keys & IN_RUN) && (keys & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT))) ||
		i_ProneCounter > gHUD.m_flTime;
	if ( !hide )
	{
		if ( gHUD.GetWaterLevel() == 3 || (gHUD.GetWaterLevel() > 0 && g_iUser3 == 1) )
			hide = weapon != 1 && weapon != 2 && weapon != 19;
		if ( !hide )
			hide = (g_iUser3 && (keys & (IN_FORWARD | IN_BACK | IN_LEFT | IN_RIGHT | IN_MOVELEFT | IN_MOVERIGHT))) ||
				(!g_ionground && g_iinjump) || g_iMovetype == MOVETYPE_FLY;
	}
	if ( hide )
	{
		if ( !gHUD.IsInMGDeploy() )
		{
			if ( (int)offset <= 54 )
				offset += 1;
			g_ihidexhair = 1;
		}
	}
	else
	{
		if ( offset > 0 )
			offset = offset >= 3 ? offset - 3 : 0;
		g_ihidexhair = 0;
	}
	if ( gHUD.m_iFOV != 90 && gHUD.m_iFOV )
		return 10;
	return offset;
}

bool ShowHudElement( int element )
{
	bool spectator = g_iUser1 || g_iUser2;
	switch ( element )
	{
	case 1: return cl_hud_health->value > 0 && !spectator;
	case 2: return cl_hud_reinforcements->value > 0 && gHUD.m_bInfiniteLives && !spectator;
	case 3: return cl_hud_ammo->value > 0 && !spectator;
	case 4: return cl_hud_objtimer->value > 0;
	case 5: return cl_hud_objectives->value > 0;
	case 6: return !spectator && cl_hud_objectives != NULL;
	case 7: return true;
	default: return false;
	}
}
