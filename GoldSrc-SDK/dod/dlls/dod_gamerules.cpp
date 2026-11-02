#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "items.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "objective.h"
#include "dod_stats.h"
#include "game.h"
#include "skill.h"
#include "voice_gamemgr.h"
#include "scripted.h"
#include "shake.h"
#include <ctype.h>
#include "../cl_dll/dod_shared.h"

extern int g_teamplay;
extern BOOL g_fGameOver;
extern int gmsgRoundState;
extern int gmsgTeamScore;
extern int gmsgWaveTime;
extern int gmsgScoreInfo;
extern int gmsgScoreInfoLong;
extern int gmsgTimerStatus;
extern int gmsgPTeam;
extern int gmsgSpectator;
extern int gmsgWaveStatus;
extern int gmsgGameRules;
extern int gmsgClientAreas;
extern int gmsgClanTimer;
extern int gmsgScoreShort;
extern int gmsgHLTV;
extern int gmsgPStatus;
extern int gmsgPClass;
extern int gmsgDeathMsg;
extern int gmsgServerName;
extern int gmsgMOTD;
extern int get_maptime_left();
extern void Host_Say(edict_t *pEntity, int teamonly);
extern void ResetEnts();
extern void FireDODCamera(const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
extern void ShowVGUI(CBasePlayer *pPlayer, int iMenu, int iParam);
extern float g_flTimeLimit;
extern cvar_t *pcvar_mp_alliesclasses;
extern cvar_t *pcvar_mp_allowspectators;
extern cvar_t *pcvar_mp_axisclasses;
extern cvar_t *pcvar_mp_chattime;
extern cvar_t *pcvar_mp_clan_match;
extern cvar_t *pcvar_mp_clan_match_warmup;
extern cvar_t *pcvar_mp_clan_readyrestart;
extern cvar_t *pcvar_mp_clan_respawntime;
extern cvar_t *pcvar_mp_clan_restartround;
extern cvar_t *pcvar_mp_clan_scoring;
extern cvar_t *pcvar_mp_clan_scoring_bonus_allies;
extern cvar_t *pcvar_mp_clan_scoring_bonus_axis;
extern cvar_t *pcvar_mp_clan_timer;
extern cvar_t *pcvar_mp_combinemglimits;
extern cvar_t *pcvar_mp_deathmsg;
extern cvar_t *pcvar_mp_friendlyfire;
extern cvar_t *pcvar_mp_log_scores;
extern cvar_t *pcvar_mp_log_scores_delay;
extern cvar_t *pcvar_mp_spawnbazookas;
extern cvar_t *pcvar_mp_teamlimit;
extern cvar_t *pcvar_mp_timelimit;
extern cvar_t *pcvar_mp_tkpenalty;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	bool CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker)
	{
		if ( g_teamplay )
		{
			if ( pTalker->pev->team == pListener->pev->team && pTalker->pev->deadflag == pListener->pev->deadflag )
				return true;
			if ( pTalker->pev->team == pListener->pev->team && pTalker->pev->deadflag == DEAD_NO )
				return true;
			return false;
		}
		return true;
	}
};

int m_SaveRoundState;
CVoiceGameMgr g_VoiceGameMgr;
static CMultiplayGameMgrHelper g_GameMgrHelper;
float g_flIntermissionStartTime;
char com_token[1500];
const char g_szTeamNames[4][128] = { "", "Allies", "Axis", "Spectators" };

class CDoDDetect : public CBaseEntity
{
public:
	CDoDDetect();
	void Spawn();
	void KeyValue(KeyValueData *pkvd);
	bool IsMasteredOn();
	gameplay_rules_t &GetGamePlay() { return m_GamePlayRules; }

	gameplay_rules_t m_GamePlayRules;
	int m_iAlliesInfiniteLives;
	int m_iAxisInfiniteLives;
	int m_iAlliesPara;
	int m_iAxisPara;
	int m_iAlliesElimPoints;
	int m_iAxisElimPoints;
	string_t m_sMaster;
};

class CDoDPreRound : public CPointEntity
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Launch(int iRoundState);

	int m_roundstate;
	string_t m_allieswin;
	string_t m_alliesloose;
	string_t m_axiswin;
	string_t m_axisloose;
	string_t m_draw;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CDodParaRoundTimer : public CBaseEntity
{
public:
	void Spawn();
	private:
	void EXPORT TimerThink();
	public:
	void StartTimer();
	void StopTimer();
	void ResetTimer();
	void SendStatus();
	void SendStatus(CBasePlayer *pPlayer);
	float TimeRemaining() { return m_fRoundTimer; }

	float m_fUpdateTimer;
	float m_fRoundTimer;
	float m_fLastThinkTime;
	bool m_bTimer;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(info_doddetect, CDoDDetect);
LINK_ENTITY_TO_CLASS(dod_preround, CDoDPreRound);
LINK_ENTITY_TO_CLASS(para_roundtimer, CDodParaRoundTimer);

void ResetGamePlayRules( gameplay_rules_t *pRules )
{
	pRules->m_bAlliesInfiniteLives = true;
	pRules->m_bAxisInfiniteLives = true;
	pRules->m_bAlliesArePara = false;
	pRules->m_bAlliesAreBrit = false;
	pRules->m_bAxisArePara = false;
	pRules->m_bUseTimer = false;
	pRules->m_iTimerTeam = 0;
	pRules->m_sAlliesElimTarget = 0;
	pRules->m_sAxisElimTarget = 0;
	pRules->m_sTimerExpireTarget = 0;
	pRules->m_iPointsAlliesElim = 5;
	pRules->m_iPointsAxisElim = 5;
	pRules->m_iPointsTimerExpires = 0;
	pRules->m_fAlliesRespawnFactor = 1;
	pRules->m_fAxisRespawnFactor = 1;
}

void CDoDTeamPlay::Init()
{
	ResetGamePlayRules(&m_GamePlayRules);
}

void CDoDTeamPlay::RefreshSkillData()
{
	CGameRules::RefreshSkillData();
	gSkillData.plrDmgCOLT = 40;
	gSkillData.plrDmgLUGER = 40;
	gSkillData.plrDmgGARAND = 120;
	gSkillData.plrDmgSCOPEDKAR = 160;
	gSkillData.plrDmgCARBINE = 40;
	gSkillData.plrDmgTHOMPSON = 40;
	gSkillData.plrDmgMP44 = 50;
	gSkillData.plrDmgSPRING = 160;
	gSkillData.plrDmgKAR = 160;
	gSkillData.plrDmgBAR = 85;
	gSkillData.plrDmg30CAL = 85;
	gSkillData.plrDmgMG42 = 85;
	gSkillData.plrDmgMG34 = 85;
	gSkillData.plrDmgMP40 = 40;
	gSkillData.plrDmgGREASEGUN = 40;
	gSkillData.plrDmgFG42 = 85;
	gSkillData.plrDmgK43 = 120;
	gSkillData.plrDmgEnfield = 160;
	gSkillData.plrDmgSten = 40;
	gSkillData.plrDmgBren = 85;
	gSkillData.plrDmgWebley = 40;
	gSkillData.plrDmgHandGrenade = 180;
	gSkillData.plrDmgStickGrenade = 180;
	gSkillData.plrDmgCrowbar = 60;
	gSkillData.plrDmgBayonet = 60;
	gSkillData.plrDmgGarandButt = 60;
	gSkillData.plrDmgEnfieldButt = 60;
	gSkillData.plrDmgK43Butt = 60;
	gSkillData.plrDmgBazookaShell = 100;
	gSkillData.plrDmgPiatShell = 100;
	gSkillData.plrDmgPschreckShell = 100;
	gSkillData.plrDmgMortarShell = 180;
}

CDoDTeamPlay::CDoDTeamPlay()
{
	g_VoiceGameMgr.Init( &g_GameMgrHelper, gpGlobals->maxClients );

	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	g_flIntermissionStartTime = 0;

	g_flTimeLimit = gpGlobals->time;
	setMapTime();

	if ( IS_DEDICATED_SERVER() )
	{
	}
	else
	{
		// listen server
		char *lservercfgfile = (char *)CVAR_GET_STRING( "lservercfgfile" );

		if ( lservercfgfile && lservercfgfile[0] )
		{
			char szCommand[256];

			ALERT( at_console, "Executing listen server config file\n" );
			_snprintf( szCommand, sizeof( szCommand ) - 1, "exec %s\n", lservercfgfile );
			szCommand[sizeof( szCommand ) - 1] = 0;
			SERVER_COMMAND( szCommand );
		}
	}

	char szTemp[74];
	sprintf( szTemp, "exec %s.cfg\n", STRING( gpGlobals->mapname ) );
	SERVER_COMMAND( szTemp );

	m_pParaTimer = NULL;
	m_flIntermissionEndTime = 0;
	g_flIntermissionStartTime = 0;
	m_iNumAlliesAlive = 0;
	m_iNumAxisAlive = 0;
	m_iNumAlliesOnTeam = 0;
	m_iNumAxisOnTeam = 0;
	m_flDoDMapTime = 0;
	m_DisableDeathMessages = FALSE;
	m_DisableDeathPenalty = FALSE;
	m_DoDCameraState = 0;
	m_flRoundTime = 0;
	m_vKickSessionState = 0;
	m_bClanMatch = false;
	m_bClanMatchActive = false;
	m_bRoundRestarting = FALSE;
	m_bAwaitingReadyRestart = FALSE;
	m_bHeardAlliesReady = FALSE;
	m_bHeardAxisReady = FALSE;
	m_flRestartRoundTime = 0;
	gr_gameRules = 0;
	SendTeamScores( 0 );
	m_rReadyToUnFreeze = 0;
	m_bIsRoundFrozen = false;
	m_RoundState = RND_GOODTOGO;
	m_SaveRoundState = m_RoundState;
	m_RoundDoesCount = 1;
	m_flGoodToGoTime = gpGlobals->time + 20;
	m_RoundParaCanJoin = 1;
	m_vKickTime = 0;
	m_flSpamResetTime = 0;
	m_vKickENT = NULL;

	if ( MapChangeFromTime != 1 )
		SetNextMap();
	MapChangeFromTime = 1;

	m_flAxisWaveTime = m_flAlliesWaveTime = m_flAlliesRespawn = m_flAxisRespawn = 0;

	float delay = pcvar_mp_log_scores_delay ? pcvar_mp_log_scores_delay->value : 10;
	if ( delay < 1 )
		delay = 1;
	if ( delay > 600 )
		delay = 600;
	m_fLogScoresTime = delay + gpGlobals->time;

	pcvar_classlimitcvars[0] = NULL;
	int i;
	for ( i = 1; i < 27; i++ )
		pcvar_classlimitcvars[i] = CVAR_GET_POINTER( classlimitcvarNames[i] );
}

BOOL CDoDTeamPlay::FAllowMonsters()
{
	return TRUE;
}

BOOL CDoDTeamPlay::IsMultiplayer()
{
	return TRUE;
}

BOOL CDoDTeamPlay::IsDeathmatch()
{
	return TRUE;
}

BOOL CDoDTeamPlay::IsTeamplay()
{
	return TRUE;
}

BOOL CDoDTeamPlay::IsCoOp()
{
	return (int)gpGlobals->coop;
}

const char *CDoDTeamPlay::GetGameDescription()
{
	return "Day of Defeat";
}

BOOL CDoDTeamPlay::AllowAutoTargetCrosshair()
{
	return FALSE;
}

BOOL CDoDTeamPlay::ShouldAutoAim(CBasePlayer *, edict_t *)
{
	return FALSE;
}

BOOL CDoDTeamPlay::FAllowFlashlight()
{
	return cvar_flashlight.value != 0;
}

float CDoDTeamPlay::FlPlayerFallDamage(CBasePlayer *pPlayer)
{
	pPlayer->m_flFallVelocity -= 500;
	return pPlayer->m_flFallVelocity * 100 / 400;
}

BOOL CDoDTeamPlay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if ( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		if ( pcvar_mp_friendlyfire && ( pcvar_mp_friendlyfire->value <= 0 ) && ( pAttacker != pPlayer ) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return FALSE;
		}
	}

	return TRUE;
}

int CDoDTeamPlay::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if ( !pKilled )
		return 0;

	if ( !pAttacker )
		return 1;

	if ( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return 0;

	return 1;
}

const char *CDoDTeamPlay::GetTeamID( CBaseEntity *pEntity )
{
	if ( pEntity == NULL || pEntity->pev == NULL )
		return "";

	// return their team name
	return pEntity->TeamID();
}

int CDoDTeamPlay::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() )
		return GR_NOTTEAMMATE;

	if ( (*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp( GetTeamID(pPlayer), GetTeamID(pTarget) ) )
	{
		return GR_TEAMMATE;
	}

	return GR_NOTTEAMMATE;
}

int CDoDTeamPlay::GetTeamIndex(const char *)
{
	return -1;
}

const char *CDoDTeamPlay::GetIndexedTeamName( int teamIndex )
{
	if ( teamIndex == 1 )
		return "Allies";
	else if ( teamIndex == 2 )
		return "Axis";
	else if ( teamIndex == 3 )
		return "Spectators";

	return "";
}

BOOL CDoDTeamPlay::IsValidTeam(const char *)
{
	return TRUE;
}

const char *CDoDTeamPlay::SetDefaultPlayerTeam(CBasePlayer *)
{
	return NULL;
}

BOOL CDoDTeamPlay::FPlayerCanRespawn(CBasePlayer *)
{
	return TRUE;
}

float CDoDTeamPlay::FlPlayerSpawnTime(CBasePlayer *)
{
	return gpGlobals->time;
}

Vector CDoDTeamPlay::VecWeaponRespawnSpot(CBasePlayerItem *pWeapon)
{
	return pWeapon->pev->origin;
}

int CDoDTeamPlay::WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	if ( pWeapon->pev->flags & FL_IMMUNE_LAVA )
		return GR_WEAPON_RESPAWN_NO;

	return GR_WEAPON_RESPAWN_YES;
}

float CDoDTeamPlay::FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	if ( pWeapon->iFlags() & 0x200 )
		return 0;

	if ( cvar_weaponstay.value > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return gpGlobals->time + 0;		// weapon respawns almost instantly
		}
	}

	return gpGlobals->time + 20;
}

float CDoDTeamPlay::FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon && pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - 100) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

int CDoDTeamPlay::DeadPlayerWeapons(CBasePlayer *)
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

int CDoDTeamPlay::DeadPlayerAmmo(CBasePlayer *)
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

BOOL CDoDTeamPlay::CanHaveItem(CBasePlayer *, CItem *)
{
	return TRUE;
}

void CDoDTeamPlay::PlayerGotItem(CBasePlayer *, CItem *)
{
}

int CDoDTeamPlay::ItemShouldRespawn( CItem *pItem )
{
	if ( pItem->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}

float CDoDTeamPlay::FlItemRespawnTime(CItem *)
{
	return gpGlobals->time + 30;
}

Vector CDoDTeamPlay::VecItemRespawnSpot(CItem *pItem)
{
	return pItem->pev->origin;
}

int CDoDTeamPlay::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	if ( pAmmo->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

float CDoDTeamPlay::FlAmmoRespawnTime(CBasePlayerAmmo *)
{
	return gpGlobals->time + 20;
}

Vector CDoDTeamPlay::VecAmmoRespawnSpot(CBasePlayerAmmo *pAmmo)
{
	return pAmmo->pev->origin;
}

void CDoDTeamPlay::PlayerGotAmmo(CBasePlayer *, char *, int)
{
}

void CDoDTeamPlay::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	ItemInfo ii;
	pWeapon->GetItemInfo( &ii );

	pWeapon->m_bTriggerSpawnOnDie = false;
	if ( !(pWeapon->pev->flags & FL_IMMUNE_LAVA) && (ii.iFlags & 0x200) )
		pWeapon->m_bTriggerSpawnOnDie = true;
}

float CDoDTeamPlay::FlHealthChargerRechargeTime()
{
	return 60;
}

float CDoDTeamPlay::FlHEVChargerRechargeTime()
{
	return 30;
}

void CDoDTeamPlay::PlayerThink(CBasePlayer *pPlayer)
{
	if (g_fGameOver)
	{
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

BOOL CDoDTeamPlay::PlayTextureSounds()
{
	return FALSE;
}

BOOL CDoDTeamPlay::PlayFootstepSounds( CBasePlayer *pl, float fvol )
{
	if ( g_footsteps && g_footsteps->value == 0 )
		return FALSE;

	if ( pl->IsOnLadder() || pl->pev->velocity.Length2D() > 200 )
		return TRUE;  // only make step sounds in multiplayer if the player is moving fast enough

	return FALSE;
}

int CDoDTeamPlay::RoundState()
{
	return m_RoundState;
}

void CDoDTeamPlay::SetRoundState(int iState)
{
	m_RoundState = iState;
	MESSAGE_BEGIN(MSG_ALL, gmsgRoundState);
	WRITE_BYTE(m_RoundState);
	MESSAGE_END();
}

void CDoDTeamPlay::SetRestartTime(float flTime)
{
	m_flRestartTime = flTime;
}

int CDoDTeamPlay::TeamScores(int iTeam)
{
	return m_iTeamScores[iTeam];
}

void CDoDTeamPlay::SetTeamScores(int iTeam, int iScore)
{
	if (m_RoundDoesCount == 1)
		m_iTeamScores[iTeam] = iScore;
}

void CDoDTeamPlay::AddToTeamScores(int iTeam, int iScore)
{
	if ((!IsClanMatch() || IsClanMatchActive()) && m_RoundDoesCount == 1)
	{
		m_iTeamScores[iTeam] += iScore;
		MESSAGE_BEGIN(MSG_ALL, gmsgTeamScore);
		WRITE_BYTE(iTeam);
		WRITE_SHORT(m_iTeamScores[iTeam]);
		MESSAGE_END();
	}
}

void CDoDTeamPlay::SendTeamScores( int m_iTeam )
{
	if ( m_iTeam == 0 )
		m_iTeamScores[1] = m_iTeamScores[2] = 0;

	if ( m_iTeam == 1 || m_iTeam == 3 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
			WRITE_BYTE( 1 );
			WRITE_SHORT( m_iTeamScores[1] );
		MESSAGE_END();
	}

	if ( m_iTeam == 2 || m_iTeam == 3 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
			WRITE_BYTE( 2 );
			WRITE_SHORT( m_iTeamScores[2] );
		MESSAGE_END();
	}
}

float CDoDTeamPlay::getWaveTime( int iTeam )
{
	float delayTime;

	switch ( iTeam )
	{
	case 1:
		delayTime = m_flAlliesRespawn - gpGlobals->time;
		break;
	case 2:
		delayTime = m_flAxisRespawn - gpGlobals->time;
		break;
	default:
		delayTime = 0;
		break;
	}

	if ( delayTime < 0 )
		delayTime = 0;

	return delayTime;
}

void CDoDTeamPlay::setMapTime()
{
	m_flDoDMapTime = gpGlobals->time;
}

float CDoDTeamPlay::getMapTime()
{
	return m_flDoDMapTime;
}

void CDoDTeamPlay::SetMapChangeFromTime(int iTime)
{
	MapChangeFromTime = iTime;
}

int CDoDTeamPlay::ReturnGameRules()
{
	return gr_gameRules;
}

bool CDoDTeamPlay::IsRoundFrozen()
{
	return m_bIsRoundFrozen;
}

bool CDoDTeamPlay::IsClanMatch()
{
	return m_bClanMatch;
}

bool CDoDTeamPlay::IsClanMatchActive()
{
	return m_bClanMatchActive;
}

bool CDoDTeamPlay::IsAxisInfinite()
{
	return m_GamePlayRules.m_bAxisInfiniteLives;
}

bool CDoDTeamPlay::IsAxisPara()
{
	return m_GamePlayRules.m_bAxisArePara;
}

bool CDoDTeamPlay::IsAlliesInfinite()
{
	return m_GamePlayRules.m_bAlliesInfiniteLives;
}

bool CDoDTeamPlay::IsAlliesPara()
{
	return m_GamePlayRules.m_bAlliesArePara;
}

bool CDoDTeamPlay::IsAlliesBrit()
{
	return m_GamePlayRules.m_bAlliesAreBrit;
}

bool CDoDTeamPlay::IsTeamPara( int team )
{
	return ( team == 1 && m_GamePlayRules.m_bAlliesArePara ) || ( team == 2 && m_GamePlayRules.m_bAxisArePara );
}

bool CDoDTeamPlay::IsTeamInfinite( int team )
{
	return ( team == 1 && m_GamePlayRules.m_bAlliesInfiniteLives ) || ( team == 2 && m_GamePlayRules.m_bAxisInfiniteLives );
}

void CDoDTeamPlay::CopyGamePlayLogic(const gameplay_rules_t &otherGamePlay)
{
	m_GamePlayRules = otherGamePlay;
}

void CDoDTeamPlay::VoteKick(CBasePlayer *, int, bool)
{
}

void CDoDTeamPlay::VoteKickTimeUp()
{
}

void CDoDTeamPlay::ReadInClassInfo()
{
}

void CDoDTeamPlay::CheckOneLifeRoundConditions()
{
}

void CDoDTeamPlay::EndMultiplayerGame()
{
	GoToIntermission();
}

void CDoDTeamPlay::VoiceCommSelect( CBasePlayer *pPlayer, int iVoiceSelect )
{
	if ( iVoiceSelect == 10 )
		return;

	if ( pPlayer->lastMenuSelected <= 0 )
		return;

	int select = iVoiceSelect + ( pPlayer->lastMenuSelected - 1 ) * 9;
	pPlayer->VoiceCom( select );
}

void CDoDTeamPlay::CheckClanRestart()
{
	if (m_bRoundRestarting && gpGlobals->time > m_flRestartRoundTime)
		StartClanMatch();
}

void CDoDTeamPlay::ReturnToWarmup()
{
	if (m_bClanMatch)
	{
		m_bClanMatchActive = false;
		setMapTime();
		SendClientGamerules(NULL);
	}
}

void CDoDTeamPlay::SetClanMatch( bool bMatchOn )
{
	m_bClanMatch = bMatchOn;
	m_bClanMatchActive = false;

	if ( !m_bClanMatch )
	{
		m_bRoundRestarting = FALSE;
		m_bAwaitingReadyRestart = FALSE;
		m_bHeardAlliesReady = FALSE;
		m_bHeardAxisReady = FALSE;
	}

	SendClientGamerules( NULL );
}

int CDoDTeamPlay::GetClassLimit( int cls )
{
#ifdef _MSC_VER
#pragma warning( suppress : 6385 )
#endif
	if ( cls > 28 || !pcvar_classlimitcvars[cls] )
		return -1;

	return pcvar_classlimitcvars[cls]->value;
}

char *WeaponIDToChar( int id )
{
	char *wpnname = NULL;

	if ( id >= 0 && id < MAX_WEAPONS )
		wpnname = weaponnames[id];

	return wpnname;
}

char *CDoDTeamPlay::GetPlayerClassName( int playerclass )
{
	char *result;
	int team = ( playerclass >= PLAYERCLASS_LEICHTINF_KAR && playerclass <= PLAYERCLASS_GERMORTAR ) ? 2 : 1;

	if ( IsTeamPara( team ) && playerclass < ARRAYSIZE( g_ParaClassInfo ) )
		result = g_ParaClassInfo[playerclass].classname;
	else
		result = g_ClassInfo[playerclass].classname;

	return result;
}

bool CDoDTeamPlay::AllowSpawnableBazookas()
{
	if (!pcvar_mp_spawnbazookas)
		pcvar_mp_spawnbazookas = CVAR_GET_POINTER("mp_spawnbazookas");
	return pcvar_mp_spawnbazookas->value > 0;
}

unsigned short CDoDTeamPlay::Camera()
{
	return m_Camera;
}

unsigned short CDoDTeamPlay::usVoice()
{
	return m_usVoice;
}

unsigned short CDoDTeamPlay::bodyDamage()
{
	return m_bodyDamage;
}

unsigned short CDoDTeamPlay::roundRestartSound()
{
	return m_roundRestartSound;
}

unsigned short CDoDTeamPlay::gerVoice()
{
	return m_gerVoice;
}

unsigned short CDoDTeamPlay::Pain()
{
	return m_Pain;
}

unsigned short CDoDTeamPlay::Smoke()
{
	return m_Smoke;
}

unsigned short CDoDTeamPlay::Prone()
{
	return m_Prone;
}

unsigned short CDoDTeamPlay::BloodSprite()
{
	return m_BloodSprite;
}

unsigned short CDoDTeamPlay::BloodStream()
{
	return m_BloodStream;
}

unsigned short CDoDTeamPlay::BulletTracers()
{
	return m_BulletTracers;
}

unsigned short CDoDTeamPlay::SparkShower()
{
	return m_SparkShower;
}

unsigned short CDoDTeamPlay::ScreenFades()
{
	return m_ScreenFades;
}

unsigned short CDoDTeamPlay::BubbleTrails()
{
	return m_BubbleTrails;
}

unsigned short CDoDTeamPlay::Bubbles()
{
	return m_Bubbles;
}

unsigned short CDoDTeamPlay::Shards()
{
	return m_Shards;
}

unsigned short CDoDTeamPlay::Explosion()
{
	return m_Explosion;
}

unsigned short CDoDTeamPlay::PopHelmet()
{
	return m_PopHelmet;
}

unsigned short CDoDTeamPlay::RoundReset()
{
	return m_RoundReset;
}

unsigned short CDoDTeamPlay::StartTrail()
{
	return m_RocketTrail;
}

void CDoDTeamPlay::SetCamera(short sPrecache)
{
	m_Camera = sPrecache;
}

void CDoDTeamPlay::SetusVoice(short sPrecache)
{
	m_usVoice = sPrecache;
}

void CDoDTeamPlay::SetbodyDamage(short sPrecache)
{
	m_bodyDamage = sPrecache;
}

void CDoDTeamPlay::SetroundRestartSound(short sPrecache)
{
	m_roundRestartSound = sPrecache;
}

void CDoDTeamPlay::SetgerVoice(short sPrecache)
{
	m_gerVoice = sPrecache;
}

void CDoDTeamPlay::SetPain(short sPrecache)
{
	m_Pain = sPrecache;
}

void CDoDTeamPlay::SetSmoke(short sPrecache)
{
	m_Smoke = sPrecache;
}

void CDoDTeamPlay::SetProne(short sPrecache)
{
	m_Prone = sPrecache;
}

void CDoDTeamPlay::SetBloodSprite(short sPrecache)
{
	m_BloodSprite = sPrecache;
}

void CDoDTeamPlay::SetBloodStream(short sPrecache)
{
	m_BloodStream = sPrecache;
}

void CDoDTeamPlay::SetBulletTracers(short sPrecache)
{
	m_BulletTracers = sPrecache;
}

void CDoDTeamPlay::SetSparkShower(short sPrecache)
{
	m_SparkShower = sPrecache;
}

void CDoDTeamPlay::SetScreenFades(short sPrecache)
{
	m_ScreenFades = sPrecache;
}

void CDoDTeamPlay::SetBubbleTrails(short sPrecache)
{
	m_BubbleTrails = sPrecache;
}

void CDoDTeamPlay::SetBubbles(short sPrecache)
{
	m_Bubbles = sPrecache;
}

void CDoDTeamPlay::SetShards(short sPrecache)
{
	m_Shards = sPrecache;
}

void CDoDTeamPlay::SetExplosion(short sPrecache)
{
	m_Explosion = sPrecache;
}

void CDoDTeamPlay::SetPopHelmet(short sPrecache)
{
	m_PopHelmet = sPrecache;
}

void CDoDTeamPlay::SetRoundReset(short sPrecache)
{
	m_RoundReset = sPrecache;
}

void CDoDTeamPlay::SetRocketTrail(short sPrecache)
{
	m_RocketTrail = sPrecache;
}

float CDoDTeamPlay::GetMaxWaveTime( int nTeam )
{
	int nNumPlayers = 0;
	float fTime = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *player = UTIL_PlayerByIndex( i );

		if ( player && player->pev->team == nTeam )
			nNumPlayers++;
	}

	if ( nNumPlayers < 3 )
		fTime = 6;
	else if ( nNumPlayers < 6 )
		fTime = 8;
	else if ( nNumPlayers < 8 )
		fTime = 10;
	else if ( nNumPlayers < 10 )
		fTime = 11;
	else if ( nNumPlayers < 12 )
		fTime = 12;
	else if ( nNumPlayers < 14 )
		fTime = 13;
	else
		fTime = 14;

	if ( nTeam == 1 )
		fTime *= m_GamePlayRules.m_fAlliesRespawnFactor;
	else if ( nTeam == 2 )
		fTime *= m_GamePlayRules.m_fAxisRespawnFactor;

	return fTime;
}

void CDoDTeamPlay::setWaveTime( int iTeam, float fTime )
{
	if ( !IsTeamInfinite( iTeam ) )
		return;

	switch ( iTeam )
	{
	case 1:
		m_flAlliesRespawn = fTime + gpGlobals->time;
		m_flAlliesWaveTime = fTime;
		break;
	case 2:
		m_flAxisRespawn = fTime + gpGlobals->time;
		m_flAxisWaveTime = fTime;
		break;
	}

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && pPlayer->pev->team == iTeam )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgWaveTime, NULL, pPlayer->edict() );
				WRITE_CHAR( fTime );
			MESSAGE_END();
		}
	}
}

int CDoDTeamPlay::CountClasses( int targetClass )
{
	int numCounted = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if ( pEnt && pEnt->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pEnt;

			if ( pPlayer->m_iNextClass == targetClass || pPlayer->pev->playerclass == targetClass )
				numCounted++;
		}
	}

	return numCounted;
}

BOOL CDoDTeamPlay::ReachedClassLimit( int targetClass )
{
	int classLimit = -1;
	int classExisting;
	BOOL result = FALSE;

	if ( targetClass == PLAYERCLASS_RANDOM )
	{
		if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0 )
			return TRUE;
		return FALSE;
	}

	if ( targetClass < 1 || targetClass >= NUM_PLAYERCLASSES )
		return FALSE;

	classLimit = GetClassLimit( targetClass );
	classExisting = CountClasses( targetClass );

	if ( pcvar_mp_combinemglimits && pcvar_mp_combinemglimits->value > 0 )
	{
		if ( targetClass == PLAYERCLASS_MG34 )
		{
			classLimit += GetClassLimit( PLAYERCLASS_MG42 );
			classExisting += CountClasses( PLAYERCLASS_MG42 );
		}
		else if ( targetClass == PLAYERCLASS_MG42 )
		{
			classLimit += GetClassLimit( PLAYERCLASS_MG34 );
			classExisting += CountClasses( PLAYERCLASS_MG34 );
		}
	}

	if ( classLimit > -1 && classExisting >= classLimit )
		result = TRUE;

	return result;
}

bool CDoDTeamPlay::CanPlayerJoinClass( int playerclass )
{
	if ( ReachedClassLimit( playerclass ) )
		return false;

	int mask = 0;
	int cls = 0;
	int team = ( playerclass >= PLAYERCLASS_LEICHTINF_KAR && playerclass <= PLAYERCLASS_GERMORTAR ) ? 2 : 1;

	if ( playerclass == PLAYERCLASS_MORTAR || playerclass == PLAYERCLASS_BRIT_MORTAR || playerclass == PLAYERCLASS_GERMORTAR )
		return false;

	if ( team == 1 )
	{
		mask = pcvar_mp_alliesclasses->value;

		if ( IsAlliesBrit() )
			cls = playerclass - PLAYERCLASS_BRIT_LIGHTINF;
		else
			cls = playerclass - PLAYERCLASS_LIGHTINF_GARAND;
	}
	else
	{
		mask = pcvar_mp_axisclasses->value;
		cls = playerclass - PLAYERCLASS_LEICHTINF_KAR;
	}

	if ( mask < 0 )
		mask = -1;

	if ( !( mask & ( 1 << cls ) ) )
		return false;

	if ( team == 2 && !IsAxisPara() && ( playerclass == PLAYERCLASS_FG42_BIPOD || playerclass == PLAYERCLASS_FG42_SCOPE ) )
		return false;

	return true;
}

void CDoDTeamPlay::ChooseRandomClass( CBasePlayer *pPlayer )
{
	int i;
	int numChoices = 0;
	int choices[10];
	int firstclass = 0;
	int lastclass = 0;

	if ( pPlayer->pev->team == 1 )
	{
		if ( IsAlliesBrit() )
		{
			firstclass = PLAYERCLASS_BRIT_LIGHTINF;
			lastclass = PLAYERCLASS_BRIT_MORTAR;
		}
		else
		{
			firstclass = PLAYERCLASS_LIGHTINF_GARAND;
			lastclass = PLAYERCLASS_MORTAR;
		}
	}
	else if ( pPlayer->pev->team == 2 )
	{
		firstclass = PLAYERCLASS_LEICHTINF_KAR;
		lastclass = PLAYERCLASS_GERMORTAR;
	}

	for ( i = firstclass; i <= lastclass; i++ )
	{
		if ( CanPlayerJoinClass( i ) )
			choices[numChoices++] = i;
	}

	if ( numChoices == 0 )
	{
		ALERT( at_console, "Random class found that all classes were full - ignoring class limits for this spawn\n" );
		pPlayer->m_iNextClass = RANDOM_LONG( firstclass, lastclass );
	}
	else
	{
		i = RANDOM_LONG( 0, numChoices - 1 );
		pPlayer->m_iNextClass = choices[i];
	}
}

void CDoDTeamPlay::PreparePlayerForSpawn( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_bIsObserver && pPlayer->m_imissedwave )
		pPlayer->m_irdytospawn = 1;

	char *classname = GetPlayerClassName( pPlayer->m_iNextClass );

	ClientPrint( pPlayer->pev, HUD_PRINTTALK, pPlayer->IsAlive() ? "#game_respawn_as" : "#game_spawn_as", classname );

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed role to \"%s\"\n",
		STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ),
		GETPLAYERAUTHID( pPlayer->edict() ),
		"Allies",
		classname );

	if ( pPlayer->m_irdytospawn )
	{
		if ( !IsTeamInfinite( pPlayer->pev->team ) )
		{
			if ( m_RoundState != RND_RUNNING )
				pPlayer->DelayedSpawn();
		}
		else
			pPlayer->DelayedSpawn();
	}
}

void CDoDTeamPlay::UpdateData( CBasePlayer *pPlayer )
{
	int IsThisFunctionEverCalled;
	IsThisFunctionEverCalled = 1;

	if ( pPlayer->m_iObjScore > 255 || pPlayer->pev->frags > 255 || pPlayer->m_iDeaths > 255 )
	{
		ALERT( at_aiconsole, "Init HUD data requiring short values - sending with WRITE_SHORT()\n" );

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfoLong );
			WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
			WRITE_SHORT( pPlayer->m_iObjScore );
			WRITE_SHORT( pPlayer->pev->frags );
			WRITE_SHORT( pPlayer->m_iDeaths );
			WRITE_BYTE( pPlayer->pev->playerclass );
			WRITE_BYTE( pPlayer->pev->team );
			WRITE_BYTE( pPlayer->PlayerStatus() );
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
			WRITE_BYTE( pPlayer->m_iObjScore );
			WRITE_BYTE( pPlayer->pev->frags );
			WRITE_BYTE( pPlayer->m_iDeaths );
			WRITE_BYTE( pPlayer->pev->playerclass );
			WRITE_BYTE( pPlayer->pev->team );
			WRITE_BYTE( pPlayer->PlayerStatus() );
		MESSAGE_END();
	}
}

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CDoDDetect::CDoDDetect()
{
	ResetGamePlayRules(&m_GamePlayRules);
}

void CDoDDetect::Spawn()
{
	AddEntityHashValue( pev, "info_doddetect", CLASSNAME );
	pev->solid = SOLID_NOT;

	CDoDTeamPlay *Dodgamerules = (CDoDTeamPlay *)g_pGameRules;
	Dodgamerules->DetectGameRules();
}

bool CDoDDetect::IsMasteredOn()
{
	if ( m_sMaster )
		return UTIL_IsMasterTriggered( m_sMaster, this ) > 0;

	return true;
}

void CDoDDetect::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "detect_axis_infinite" ) )
	{
		switch ( atoi( pkvd->szValue ) )
		{
		case 1:
			m_GamePlayRules.m_bAxisInfiniteLives = true;
			break;
		default:
			m_GamePlayRules.m_bAxisInfiniteLives = false;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_allies_infinite" ) )
	{
		m_GamePlayRules.m_bAlliesInfiniteLives = ( atoi( pkvd->szValue ) == 1 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "axis_infinite_lifes" ) )
	{
		m_GamePlayRules.m_bAxisInfiniteLives = ( atoi( pkvd->szValue ) == 1 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "allies_infinite_lifes" ) )
	{
		m_GamePlayRules.m_bAlliesInfiniteLives = ( atoi( pkvd->szValue ) == 1 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_axis_paras" ) )
	{
		m_GamePlayRules.m_bAxisArePara = ( atoi( pkvd->szValue ) > 0 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_allies_country" ) )
	{
		m_GamePlayRules.m_bAlliesAreBrit = ( atoi( pkvd->szValue ) == 1 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_allies_paras" ) )
	{
		m_GamePlayRules.m_bAlliesArePara = ( atoi( pkvd->szValue ) > 0 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_points_allieseliminated" ) )
	{
		m_GamePlayRules.m_iPointsAlliesElim = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_points_axiseliminated" ) )
	{
		m_GamePlayRules.m_iPointsAxisElim = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_target_allieseliminated" ) )
	{
		m_GamePlayRules.m_sAlliesElimTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_target_axiseliminated" ) )
	{
		m_GamePlayRules.m_sAxisElimTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_use_timer" ) )
	{
		m_GamePlayRules.m_bUseTimer = ( atoi( pkvd->szValue ) > 0 );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_target_timerexpired" ) )
	{
		m_GamePlayRules.m_sTimerExpireTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_timer_team" ) )
	{
		m_GamePlayRules.m_iTimerTeam = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_points_timerexpired" ) )
	{
		m_GamePlayRules.m_iPointsTimerExpires = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_allies_respawnfactor" ) )
	{
		m_GamePlayRules.m_fAlliesRespawnFactor = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "detect_axis_respawnfactor" ) )
	{
		m_GamePlayRules.m_fAxisRespawnFactor = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CDoDTeamPlay::DetectGameRules( void )
{
	int lastGameRules;
	lastGameRules = gr_gameRules;
	bool bFound = false;

	CBaseEntity *pEnt = UTIL_FindEntityByClassname( NULL, "info_doddetect" );
	while ( pEnt )
	{
		CDoDDetect *pDetect = (CDoDDetect *)pEnt;

		if ( pDetect && pDetect->IsMasteredOn() )
		{
			gr_gameRules = 1;
			bFound = true;
			CopyGamePlayLogic( pDetect->GetGamePlay() );
		}

		pEnt = UTIL_FindEntityByClassname( pEnt, "info_doddetect" );
	}

	if ( !bFound )
	{
		gr_gameRules = 0;
		ResetGamePlayRules( &m_GamePlayRules );
	}

	SendClientGamerules( NULL );
}

void CDoDPreRound::KeyValue( KeyValueData *pkvd )
{
	pkvd->fHandled = TRUE;

	if ( FStrEq( pkvd->szKeyName, "allieswin" ) )
		m_allieswin = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "alliesloose" ) )
		m_alliesloose = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "axiswin" ) )
		m_axiswin = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "axisloose" ) )
		m_axisloose = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "draw" ) )
		m_draw = ALLOC_STRING( pkvd->szValue );
	else
		CBaseEntity::KeyValue( pkvd );
}

void CDoDPreRound::Launch( int iRoundState )
{
	if ( iRoundState == RND_ALLIES_WIN )
	{
		if ( m_allieswin )
			FireTargets( STRING( m_allieswin ), NULL, this, USE_TOGGLE, 0 );

		if ( m_axisloose )
			FireTargets( STRING( m_axisloose ), NULL, this, USE_TOGGLE, 0 );
	}
	else if ( iRoundState == RND_AXIS_WIN )
	{
		if ( m_axiswin )
			FireTargets( STRING( m_axiswin ), NULL, this, USE_TOGGLE, 0 );

		if ( m_alliesloose )
			FireTargets( STRING( m_alliesloose ), NULL, this, USE_TOGGLE, 0 );
	}
	else if ( iRoundState == RND_DRAW && m_draw )
		FireTargets( STRING( m_draw ), NULL, this, USE_TOGGLE, 0 );
}

void CDodParaRoundTimer::Spawn( void )
{
	m_bTimer = false;
	pev->solid = SOLID_NOT;
	SetThink( &CDodParaRoundTimer::TimerThink );
	m_fLastThinkTime = gpGlobals->time;
	m_fUpdateTimer = 30;
	m_fRoundTimer = 0;
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CDodParaRoundTimer::SendStatus( void )
{
	float t = TimeRemaining();

	MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( t > 0 ? t : 0 );
	MESSAGE_END();
}

void CDodParaRoundTimer::SendStatus( CBasePlayer *pPlayer )
{
	float t = TimeRemaining();

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerStatus, NULL, pPlayer->edict() );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( t > 0 ? t : 0 );
	MESSAGE_END();
}

void CDodParaRoundTimer::StartTimer()
{
	m_bTimer = true;
	SendStatus();
	SetThink(&CDodParaRoundTimer::TimerThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CDodParaRoundTimer::StopTimer()
{
	m_bTimer = false;
	SendStatus();
	SetThink(NULL);
}

void CDodParaRoundTimer::ResetTimer( void )
{
	m_bTimer = true;

	float mppararoundtime = 4;
	if ( mppararoundtime < 3 )
		mppararoundtime = 3;

	m_fRoundTimer = mppararoundtime * 60;
	m_fUpdateTimer = 30;

	SendStatus();

	SetThink( &CDodParaRoundTimer::TimerThink );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CDodParaRoundTimer::TimerThink( void )
{
	float frameTime = gpGlobals->time - m_fLastThinkTime;

	m_fLastThinkTime = gpGlobals->time;
	pev->nextthink = gpGlobals->time + 0.1f;

	if ( g_pGameRules->RoundState() != RND_RUNNING && m_bTimer == true )
		StopTimer();

	if ( m_bTimer == true )
	{
		m_fRoundTimer -= frameTime;

		if ( TimeRemaining() <= 0 )
		{
			CDoDTeamPlay *pDoDTeamPlay = (CDoDTeamPlay *)g_pGameRules;

			if ( pDoDTeamPlay )
				pDoDTeamPlay->TimerExpired();

			StopTimer();
		}
		else
		{
			m_fUpdateTimer -= 0.1f;

			if ( m_fUpdateTimer <= 0 )
			{
				SendStatus();
				m_fUpdateTimer = 30;
			}
		}
	}
}

BOOL CDoDTeamPlay::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pWeapon->CanDeploy() )
	{
		// that weapon can't deploy anyway.
		return FALSE;
	}

	if ( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if ( !pPlayer->m_pActiveItem->CanHolster() )
	{
		// can't put away the active item.
		return FALSE;
	}

	if ( pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight() )
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CDoDTeamPlay::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{

	CBasePlayerItem *pCheck;
	CBasePlayerItem *pBest;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight;
	int i;

	iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	if ( !pCurrentWeapon->CanHolster() )
	{
		// can't put this gun away right now, so can't switch.
		return FALSE;
	}

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pCheck = pPlayer->m_rgpPlayerItems[ i ];

		while ( pCheck )
		{
			if ( pCheck->iWeight() > -1 && pCheck->iWeight() == pCurrentWeapon->iWeight() && pCheck != pCurrentWeapon )
			{
				// this weapon is from the same category.
				if ( pCheck->CanDeploy() )
				{
					if ( pPlayer->SwitchWeapon( pCheck, 0 ) )
					{
						return TRUE;
					}
				}
			}
			else if ( pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
			{
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted
				// weapon.
				if ( pCheck->CanDeploy() )
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}

			pCheck = pCheck->m_pNext;
		}
	}

	// if we make it here, we've checked all the weapons and found no useable

	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always
	// at least get the crowbar, but ya never know.
	if ( !pBest )
	{
		return FALSE;
	}

	pPlayer->SwitchWeapon( pBest, 0 );

	return TRUE;
}

BOOL CDoDTeamPlay::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
	if ( cvar_weaponstay.value > 0 && (pItem->iFlags() & ITEM_FLAG_LIMITINWORLD) )
		return CGameRules::CanHavePlayerItem( pPlayer, pItem );

	int itemClass = pItem->Classify();
	int superClass = 0;

	if ( itemClass & 8 )
		superClass = 8;
	if ( itemClass & 0x3F0 )
		superClass = 0x3F0;
	if ( itemClass & 1 )
		superClass = 1;
	if ( itemClass & 6 )
		superClass = 6;

	if ( superClass == 6 )
		return CGameRules::CanHavePlayerItem( pPlayer, pItem );

	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		CBasePlayerItem *pItem2 = pPlayer->m_rgpPlayerItems[i];
		while ( pItem2 )
		{
			int thisItemClass = pItem2->Classify();
			if ( thisItemClass & superClass )
				return FALSE;
			pItem2 = pItem2->m_pNext;
		}
	}

	return CGameRules::CanHavePlayerItem( pPlayer, pItem );
}

edict_t *CDoDTeamPlay::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = CGameRules::GetPlayerSpawnSpot( pPlayer );

	if ( !pentSpawnSpot )
		return NULL;

	if ( IsMultiplayer() && pentSpawnSpot->v.target )
	{
		FireTargets( STRING(pentSpawnSpot->v.target), pPlayer, pPlayer, USE_TOGGLE, 0 );
	}

	return pentSpawnSpot;
}

BOOL CDoDTeamPlay::ClientConnected(edict_t *pEntity, const char *, const char *, char *)
{
	g_VoiceGameMgr.ClientConnected(pEntity);
	char *pInfo = g_engfuncs.pfnGetInfoKeyBuffer(pEntity);
	const char *pszName = g_engfuncs.pfnInfoKeyValue(pInfo, "name");
	if (*pszName == '#')
	{
		char sName[268];
		strncpy(sName, pszName, 255);
		sName[255] = 0;
		sName[0] = '*';
		g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pEntity), pInfo, "name", sName);
	}
	return TRUE;
}

void CDoDTeamPlay::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	char *mdls;
	mdls = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );

	if ( pPlayer->m_sPlayerModel )
		g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", pPlayer->m_sPlayerModel );

	char *cl_lw_value = g_engfuncs.pfnInfoKeyValue( infobuffer, "cl_lw" );
	int value = cl_lw_value[0] - '0';

	if ( value < 1 )
		g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "cl_lw", "1" );

	pPlayer->SetPrefsFromUserinfo( infobuffer );
}

void CDoDTeamPlay::ChangePlayerTeam( CBasePlayer *pPlayer, int iTeam, BOOL bKill, BOOL bGib )
{
	int damageFlags = DMG_NEVERGIB;
	int clientIndex = pPlayer->entindex();
	const char *pTeamName = g_szTeamNames[iTeam];

	if ( bKill )
	{
		entvars_t *pevWorld = VARS( INDEXENT(0) );
		pPlayer->TakeDamage( pevWorld, pevWorld, 900, damageFlags );
	}

	strncpy( pPlayer->m_szTeamName, pTeamName, TEAM_NAME_LENGTH );

	g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team", pPlayer->m_szTeamName );

	MESSAGE_BEGIN( MSG_ALL, gmsgPTeam );
		WRITE_BYTE( clientIndex );
		WRITE_BYTE( iTeam );
	MESSAGE_END();
}

BOOL CDoDTeamPlay::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	if ( !AllowSpawnableBazookas() )
	{
		const char *clsName = STRING( pEntity->pev->classname );

		if ( FStrEq( clsName, "weapon_piat" ) || FStrEq( clsName, "weapon_pschreck" ) || FStrEq( clsName, "weapon_bazooka" ) )
			return FALSE;
	}

	return TRUE;
}

void CDoDTeamPlay::EnumerateTeams(int *aliveAllies, int *numAllies, int *aliveAxis, int *numAxis)
{
	*aliveAllies = *numAllies = *aliveAxis = *numAxis = 0;
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex(i);
		if (!pPlayer || pPlayer->has_disconnected)
			continue;
		if (pPlayer->pev->team == 1)
		{
			++*numAllies;
			if (pPlayer->IsAlive())
				++*aliveAllies;
		}
		else if (pPlayer->pev->team == 2)
		{
			++*numAxis;
			if (pPlayer->IsAlive())
				++*aliveAxis;
		}
	}
}

void CDoDTeamPlay::ResetForDoDCamera( void )
{
	CBaseEntity *pEnt = NULL;

	pEnt = UTIL_FindEntityByClassname( NULL, "scripted_sequence" );
	while ( pEnt )
	{
		CCineMonster *pBarney = (CCineMonster *)pEnt;

		if ( pBarney )
			pBarney->DoDReset();

		pEnt = UTIL_FindEntityByClassname( pEnt, "scripted_sequence" );
	}

	pEnt = UTIL_FindEntityByClassname( NULL, "func_train" );
	while ( pEnt )
	{
		pEnt->RoundRespawn();
		pEnt = UTIL_FindEntityByClassname( pEnt, "func_train" );
	}
}

int CDoDTeamPlay::GetClassMask( int team )
{
	int i;
	int mask = 0;
	int allowed;
	int firstclass;
	int lastclass;

	if ( team == 1 )
	{
		if ( IsAlliesBrit() )
		{
			firstclass = PLAYERCLASS_BRIT_LIGHTINF;
			lastclass = PLAYERCLASS_BRIT_MORTAR;
		}
		else
		{
			firstclass = PLAYERCLASS_LIGHTINF_GARAND;
			lastclass = PLAYERCLASS_MORTAR;
		}
	}
	else
	{
		firstclass = PLAYERCLASS_LEICHTINF_KAR;
		lastclass = PLAYERCLASS_GERMORTAR;
	}

	for ( i = firstclass; i <= lastclass; i++ )
	{
		if ( ReachedClassLimit( i ) )
			mask |= ( 1 << ( i - firstclass ) );
	}

	mask = mask << 16;

	if ( team == 1 )
	{
		allowed = pcvar_mp_alliesclasses->value;

		if ( allowed < 0 )
			allowed = -1;

		mask |= allowed & 0xFFFF;

		if ( IsAlliesBrit() )
			mask &= ~( 1 << ( PLAYERCLASS_BRIT_MORTAR - PLAYERCLASS_BRIT_LIGHTINF ) );
		else
			mask &= ~( 1 << ( PLAYERCLASS_MORTAR - PLAYERCLASS_LIGHTINF_GARAND ) );
	}
	else
	{
		allowed = pcvar_mp_axisclasses->value;

		if ( allowed < 0 )
			allowed = -1;

		mask |= allowed & 0xFFFF;
		mask &= ~( 1 << ( PLAYERCLASS_GERMORTAR - PLAYERCLASS_LEICHTINF_KAR ) );
	}

	return mask;
}

void CDoDTeamPlay::ShowClassSelectMenu( CBasePlayer *pPlayer )
{
	if ( pPlayer->pev->team == 1 )
	{
		if ( IsAlliesPara() )
			ShowVGUI( pPlayer, 11, GetClassMask( 1 ) );
		else if ( IsAlliesBrit() )
			ShowVGUI( pPlayer, 12, GetClassMask( 1 ) );
		else
			ShowVGUI( pPlayer, 10, GetClassMask( 1 ) );
	}
	else if ( pPlayer->pev->team == 2 )
	{
		if ( IsAxisPara() )
			ShowVGUI( pPlayer, 14, GetClassMask( 2 ) );
		else
			ShowVGUI( pPlayer, 13, GetClassMask( 2 ) );
	}
}

void CDoDTeamPlay::ClientDisconnected( edict_t *pClient )
{
	if ( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if ( pPlayer )
		{
			// Tell all clients this player isn't a spectator anymore
			MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
				WRITE_BYTE( ENTINDEX( pClient ) );
				WRITE_BYTE( 0 );
			MESSAGE_END();

			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

			if ( pPlayer->pev->team == 1 )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" disconnected\n",
					STRING( pPlayer->pev->netname ),
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					"Allies" );
			}
			else
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" disconnected\n",
					STRING( pPlayer->pev->netname ),
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					"Axis" );
			}

			pPlayer->DropPlayerItem( "weapon_bazooka", true );
			pPlayer->DropPlayerItem( "weapon_pschreck", true );
			pPlayer->DropPlayerItem( "weapon_piat", true );

			pPlayer->RemoveAllItems( TRUE );// destroy all of the players weapons and items

			CBasePlayer *client = NULL;
			while ( ( ( client = (CBasePlayer *)UTIL_FindEntityByClassname( client, "player" ) ) != NULL ) && ( !FNullEnt( client->edict() ) ) )
			{
				if ( !client->pev )
					continue;
				if ( client == pPlayer )
					continue;

				// If a spectator was chasing this player, move him/her onto the next player
				if ( client->m_hObserverTarget == pPlayer )
				{
					int iMode = client->pev->iuser1;
					client->pev->iuser1 = 0;
					client->m_hObserverTarget = NULL;
					client->Observer_SetMode( iMode );
				}
			}

			if ( pPlayer->HasObject )
				pPlayer->DropObject();
		}

		pPlayer->ClientDisconnect();
	}

	CheckOneLifeRoundConditions();
}

void CDoDTeamPlay::SendObjectiveStatus( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgRoundState, NULL, pPlayer->pev );
		WRITE_BYTE( RoundState() );
	MESSAGE_END();

	CBaseEntity *pEnt = UTIL_FindEntityByClassname( NULL, "dod_control_point_master" );
	while ( pEnt )
	{
		CControlPointMaster *pMaster = (CControlPointMaster *)pEnt;
		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_control_point_master" );

		if ( pMaster && pMaster->IsMasteredOn() )
		{
			pMaster->SendObjectiveStatus( pPlayer );
			pEnt = NULL;
		}
	}

	pEnt = UTIL_FindEntityByClassname( NULL, "dod_capture_area" );
	while ( pEnt )
	{
		pEnt->area_SendStatus( pPlayer );
		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_capture_area" );
	}

	pEnt = UTIL_FindEntityByClassname( NULL, "dod_object_goal" );
	while ( pEnt )
	{
		pEnt->area_SendStatus( pPlayer );
		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_object_goal" );
	}

	pEnt = UTIL_FindEntityByClassname( NULL, "func_breakable" );
	while ( pEnt )
	{
		pEnt->area_SendStatus( pPlayer );
		pEnt = UTIL_FindEntityByClassname( pEnt, "func_breakable" );
	}

	pEnt = UTIL_FindEntityByClassname( NULL, "dod_round_timer" );
	while ( pEnt )
	{
		CDodRoundTimer *pTimer = (CDodRoundTimer *)pEnt;

		if ( pTimer )
			pTimer->SendStatus( pPlayer );

		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_round_timer" );
	}

	pEnt = UTIL_FindEntityByClassname( NULL, "particle_shooter" );
	while ( pEnt )
	{
		CParticleShooter *pShooter = (CParticleShooter *)pEnt;

		if ( pShooter )
			pShooter->SendUpdate( pPlayer );

		pEnt = UTIL_FindEntityByClassname( pEnt, "particle_shooter" );
	}
}

void CDoDTeamPlay::SendWaveStatus(CBasePlayer *pPlayer, int status)
{
	if (pPlayer && IsTeamInfinite(pPlayer->pev->team))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWaveStatus, NULL, pPlayer->edict());
		WRITE_BYTE(status);
		MESSAGE_END();
	}
}

void CDoDTeamPlay::SendClientGamerules( CBasePlayer *pPlayer )
{
	bool b_everyone = ( pPlayer == NULL );
	byte allies = 0;
	byte axis = 0;

	if ( m_GamePlayRules.m_bAlliesArePara )
		allies |= 1;
	else if ( m_GamePlayRules.m_bAlliesAreBrit )
		allies |= 4;

	if ( m_GamePlayRules.m_bAlliesInfiniteLives )
		allies |= 2;

	if ( m_GamePlayRules.m_bAxisArePara )
		axis |= 1;

	if ( m_GamePlayRules.m_bAxisInfiniteLives )
		axis |= 2;

	if ( IsClanMatch() && !IsClanMatchActive() )
		axis |= 4;

	if ( b_everyone )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *plr = UTIL_PlayerByIndex( i );

			if ( plr && plr->pev->team > 0 && plr->pev->team < 4 )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgGameRules, NULL, plr->edict() );
					WRITE_BYTE( allies );
					WRITE_BYTE( axis );
				MESSAGE_END();
			}
		}
	}
	else if ( pPlayer )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgGameRules, NULL, pPlayer->edict() );
			WRITE_BYTE( allies );
			WRITE_BYTE( axis );
		MESSAGE_END();
	}
}

int CDoDTeamPlay::GetWeaponId( const char *weaponname )
{
	int id = -1;

	if ( !strcmp( weaponname, "WEAPON_AMERKNIFE" ) )
		id = WEAPON_AMERKNIFE;
	else if ( !strcmp( weaponname, "WEAPON_GERKNIFE" ) )
		id = WEAPON_GERKNIFE;
	else if ( !strcmp( weaponname, "WEAPON_COLT" ) )
		id = WEAPON_COLT;
	else if ( !strcmp( weaponname, "WEAPON_LUGER" ) )
		id = WEAPON_LUGER;
	else if ( !strcmp( weaponname, "WEAPON_GARAND" ) )
		id = WEAPON_GARAND;
	else if ( !strcmp( weaponname, "WEAPON_SCOPEDKAR" ) )
		id = WEAPON_SCOPEDKAR;
	else if ( !strcmp( weaponname, "WEAPON_THOMPSON" ) )
		id = WEAPON_THOMPSON;
	else if ( !strcmp( weaponname, "WEAPON_MP44" ) )
		id = WEAPON_MP44;
	else if ( !strcmp( weaponname, "WEAPON_SPRING" ) )
		id = WEAPON_SPRING;
	else if ( !strcmp( weaponname, "WEAPON_KAR" ) )
		id = WEAPON_KAR;
	else if ( !strcmp( weaponname, "WEAPON_BAR" ) )
		id = WEAPON_BAR;
	else if ( !strcmp( weaponname, "WEAPON_MP40" ) )
		id = WEAPON_MP40;
	else if ( !strcmp( weaponname, "WEAPON_MG42" ) )
		id = WEAPON_MG42;
	else if ( !strcmp( weaponname, "WEAPON_30CAL" ) )
		id = WEAPON_30CAL;
	else if ( !strcmp( weaponname, "WEAPON_SPADE" ) )
		id = WEAPON_SPADE;
	else if ( !strcmp( weaponname, "WEAPON_M1CARBINE" ) )
		id = WEAPON_M1CARBINE;
	else if ( !strcmp( weaponname, "WEAPON_MG34" ) )
		id = WEAPON_MG34;
	else if ( !strcmp( weaponname, "WEAPON_GREASEGUN" ) )
		id = WEAPON_GREASEGUN;
	else if ( !strcmp( weaponname, "WEAPON_FG42" ) )
		id = WEAPON_FG42;
	else if ( !strcmp( weaponname, "WEAPON_K43" ) )
		id = WEAPON_K43;
	else if ( !strcmp( weaponname, "WEAPON_ENFIELD" ) )
		id = WEAPON_ENFIELD;
	else if ( !strcmp( weaponname, "WEAPON_STEN" ) )
		id = WEAPON_STEN;
	else if ( !strcmp( weaponname, "WEAPON_BREN" ) )
		id = WEAPON_BREN;
	else if ( !strcmp( weaponname, "WEAPON_WEBLEY" ) )
		id = WEAPON_WEBLEY;
	else if ( !strcmp( weaponname, "WEAPON_FOLDINGCARBINE" ) )
		id = WEAPON_M1CARBINE;
	else if ( !strcmp( weaponname, "WEAPON_MG34" ) )
		id = WEAPON_MG34;
	else if ( !strcmp( weaponname, "WEAPON_GERPARAKNIFE" ) )
		id = WEAPON_GERKNIFE;
	else if ( !strcmp( weaponname, "WEAPON_SCOPED_FG42" ) )
		id = 65;
	else if ( !strcmp( weaponname, "WEAPON_SCOPEDENFIELD" ) )
		id = WEAPON_ENFIELD;

	return id;
}

void CDoDTeamPlay::LevelChangeResets( CBasePlayer *pPlayer )
{
	pPlayer->m_iFOV = 0;
	pPlayer->pev->fov = 0;
	pPlayer->CheckPlayerSpeed();

	MESSAGE_BEGIN( MSG_ONE, gmsgClientAreas, NULL, pPlayer->pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 2 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgWaveTime, NULL, pPlayer->pev );
		WRITE_CHAR( 0 );
	MESSAGE_END();
}

void CDoDTeamPlay::StartClanRestart( void )
{
	float fRestartDelay = pcvar_mp_clan_timer ? pcvar_mp_clan_timer->value : 0;

	m_flRestartRoundTime = fRestartDelay + gpGlobals->time;
	m_bRoundRestarting = TRUE;

	char secs[8];
	char mins[8];
	int time_mins, time_secs;
	int iRestartDelay = fRestartDelay;

	time_mins = iRestartDelay / 60;
	time_secs = iRestartDelay - ( time_mins * 60 );

	strncpy( mins, UTIL_dtos1( time_mins ), sizeof( mins ) - 1 );
	mins[sizeof( mins ) - 1] = 0;
	strcpy( secs, UTIL_dtos1( time_secs ) );

	MESSAGE_BEGIN( MSG_ALL, gmsgClanTimer );
		WRITE_BYTE( iRestartDelay );
	MESSAGE_END();

	UTIL_ClientPrintAll( HUD_PRINTCENTER, "#Clan_time_remaining", mins, secs );
}

void CDoDTeamPlay::StartClanMatch( void )
{
	m_bClanMatchActive = true;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr )
		{
			plr->m_iObjScore = 0;
			plr->pev->frags = 0;
			plr->m_iDeaths = 0;

			MESSAGE_BEGIN( MSG_ALL, gmsgScoreShort );
				WRITE_BYTE( ENTINDEX( plr->edict() ) );
				WRITE_SHORT( 0 );
				WRITE_SHORT( 0 );
				WRITE_SHORT( 0 );
				WRITE_BYTE( plr->PlayerStatus() );
			MESSAGE_END();
		}
	}

	SendClientGamerules( NULL );
	SendTeamScores( 0 );
	SetRoundState( 0 );
	RestartRound();
	setMapTime();
	m_bRoundRestarting = FALSE;
}

void CDoDTeamPlay::GoodToGo( void )
{
	DetectGameRules();

	int iPlayersAvalable = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr && ( plr->pev->team == 1 || plr->pev->team == 2 ) && plr->m_iNextClass != -1 )
			iPlayersAvalable++;
	}

	if ( !iPlayersAvalable )
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_not_enough" );
		SetRoundState( 0 );
		m_flGoodToGoTime = gpGlobals->time + 20;
	}
	else
		RestartRound();
}

void CDoDTeamPlay::GoToIntermission( void )
{
	UTIL_LogPrintf( "Final Scores: Allies: %d   Axis: %d\n", m_iTeamScores[1], m_iTeamScores[2] );

	if ( g_fGameOver )
		return;		// intermission has already been triggered, so ignore.

	if ( IsClanMatch() )
		ReturnToWarmup();

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	int time = pcvar_mp_chattime ? (int)pcvar_mp_chattime->value : 10;
	if ( time < 10 )
		CVAR_SET_FLOAT( "mp_chattime", 10 );
	else if ( time > 120 )
		CVAR_SET_FLOAT( "mp_chattime", 120 );

#ifdef _MSC_VER
#pragma warning( suppress : 6011 )
#endif
	m_flIntermissionEndTime = gpGlobals->time + ( (int)pcvar_mp_chattime->value );
	g_flIntermissionStartTime = gpGlobals->time;

	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = TRUE;
}

int CountTeamMembers( int teamNum )
{
	int iAllies = 0;
	int iAxis = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *p = UTIL_PlayerByIndex( i );

		if ( p )
		{
			if ( p->pev->team == 1 )
				iAllies++;
			else if ( p->pev->team == 2 )
				iAxis++;
		}
	}

	if ( teamNum == 1 )
		return iAllies - iAxis;
	else
		return iAxis - iAllies;
}

int DeadTeammates( entvars_t *pevPlr )
{
	int iDead = 0;

	if ( !pevPlr )
		return 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *p = UTIL_PlayerByIndex( i );

		if ( !p )
			continue;

		if ( p->has_disconnected )
			continue;

		if ( p->pev->team != pevPlr->team )
			continue;

		if ( p->pev == pevPlr )
			continue;

		if ( p->IsAlive() )
			continue;

		iDead++;
	}

	return iDead;
}

int AliveTeammates( entvars_t *pevPlr )
{
	int iAlive = 0;

	if ( !pevPlr )
		return 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *p = UTIL_PlayerByIndex( i );

		if ( !p )
			continue;

		if ( p->has_disconnected )
			continue;

		if ( p->pev->team != pevPlr->team )
			continue;

		if ( p->pev == pevPlr )
			continue;

		if ( !p->IsAlive() )
			continue;

		iAlive++;
	}

	return iAlive;
}

void RespawnAllies()
{
	ALERT(at_console, "Respawning Allies\n");
	FireTargets("dod_respawn_allies", NULL, NULL, USE_TOGGLE, 0);
	CBasePlayer *pFirst = NULL;
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (pPlayer && !pPlayer->has_disconnected && pPlayer->pev->team == 1 && !pPlayer->IsAlive())
		{
			if (pPlayer->m_iNextClass == -1)
			{
				ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#game_will_spawn");
				pPlayer->m_imissedwave = 1;
			}
			else
			{
				pPlayer->DelayedSpawn();
				if (!pFirst)
					pFirst = pPlayer;
			}
		}
	}
	UTIL_SendDirectorMessage(pFirst, NULL, 130);
}

void RespawnAxis()
{
	ALERT(at_console, "Respawning Axis\n");
	CBasePlayer *pFirst = NULL;
	FireTargets("dod_respawn_axis", NULL, NULL, USE_TOGGLE, 0);
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (pPlayer && !pPlayer->has_disconnected && pPlayer->pev->team == 2 && !pPlayer->IsAlive())
		{
			if (pPlayer->m_iNextClass == -1)
			{
				ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#game_will_spawn");
				pPlayer->m_imissedwave = 1;
			}
			else
			{
				pPlayer->DelayedSpawn();
				if (!pFirst)
					pFirst = pPlayer;
			}
		}
	}
	UTIL_SendDirectorMessage(pFirst, NULL, 2);
}

void CDoDTeamPlay::PlayerDeathThink( CBasePlayer *pPlayer )
{
	if ( pPlayer->m_fDeadTime + 1 < gpGlobals->time && !g_fGameOver )
	{
		if ( RoundState() == RND_RUNNING )
		{
			pPlayer->m_bIsObserver = TRUE;

			if ( IsTeamInfinite( pPlayer->pev->team ) )
			{
				if ( getWaveTime( pPlayer->pev->team ) <= 0 )
				{
					float waveTime = GetMaxWaveTime( pPlayer->pev->team );

					if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0 )
					{
						float tempTime = pcvar_mp_clan_respawntime ? pcvar_mp_clan_respawntime->value : -1;

						if ( tempTime > 0 )
							waveTime = tempTime;
					}

					if ( waveTime <= 3 )
						waveTime = 3;

					setWaveTime( pPlayer->pev->team, waveTime );
				}

				SendWaveStatus( pPlayer, 1 );
			}

			pPlayer->StartObserver( pPlayer->pev->origin, pPlayer->pev->angles );
		}
		else
		{
			pPlayer->StartObserver( pPlayer->pev->origin, pPlayer->pev->angles );
		}
	}
}

void CDoDTeamPlay::RoundFreeze( void )
{
	m_rRoundStartCount = gpGlobals->time + 5.0;
	m_rReadyToUnFreeze = 1;
	m_bIsRoundFrozen = true;

	CBaseEntity *pEnt = NULL;

	pEnt = UTIL_FindEntityByClassname( NULL, "dod_round_timer" );
	while ( pEnt )
	{
		CDodRoundTimer *pObject = (CDodRoundTimer *)pEnt;

		if ( pObject && TRUE )
			pObject->ResetTimer();

		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_round_timer" );
	}

	bool iFireAxisCam = false, iFireAllieCam = false;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pl = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( pl && pl->Classify() == CLASS_PLAYER )
		{
			if ( pl->pev->team == 3 )
				continue;

			if ( pl->pev->team == 1 && !pl->i_seenAllieSpawnScene )
				iFireAllieCam = true;
			else if ( pl->pev->team == 2 && !pl->i_seenAxisSpawnScene )
				iFireAxisCam = true;

			g_engfuncs.pfnSetClientMaxspeed( ENT( pl->pev ), 1 );
		}
	}

	ResetForDoDCamera();

	if ( iFireAllieCam )
		FireDODCamera( "allie_spawn_cut", NULL, NULL, USE_TOGGLE, 0 );

	if ( iFireAxisCam )
		FireDODCamera( "axis_spawn_cut", NULL, NULL, USE_TOGGLE, 0 );
}

void CDoDTeamPlay::RoundUnfreeze( void )
{
	float f_Random;

	SetRoundState( RND_RUNNING );
	m_rReadyToUnFreeze = 0;
	m_bIsRoundFrozen = false;

	CBaseEntity *pEnt = NULL;

	pEnt = UTIL_FindEntityByClassname( NULL, "dod_round_timer" );
	while ( pEnt )
	{
		CDodRoundTimer *pObject = (CDodRoundTimer *)pEnt;

		if ( pObject && TRUE )
		{
			if ( !( pObject->pev->spawnflags & 2 ) )
				pObject->StartTimer();
		}

		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_round_timer" );
	}

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pl = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( pl && pl->Classify() == CLASS_PLAYER )
		{
			if ( pl->pev->team == 3 )
				continue;

			pl->CheckPlayerSpeed();

			f_Random = RANDOM_LONG( 0, 3 );

			if ( pl->pev->team == 1 && IsAlliesBrit() )
			{
				switch ( (int)f_Random )
				{
				case 0:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_brit1" );
					break;
				case 1:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_brit2" );
					break;
				case 2:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_brit1" );
					break;
				case 3:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_brit2" );
					break;
				}

				PLAYBACK_EVENT_FULL( 0, pl->edict(), m_roundRestartSound, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, pl->pev->team, (int)f_Random, 1, 0 );
			}
			else if ( pl->pev->team == 1 )
			{
				switch ( (int)f_Random )
				{
				case 0:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_allie1" );
					break;
				case 1:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_allie2" );
					break;
				case 2:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_allie1" );
					break;
				case 3:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_allie2" );
					break;
				}

				PLAYBACK_EVENT_FULL( 0, pl->edict(), m_roundRestartSound, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, pl->pev->team, (int)f_Random, 0, 0 );
			}
			else if ( pl->pev->team == 2 )
			{
				switch ( (int)f_Random )
				{
				case 0:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_axis1" );
					break;
				case 1:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_axis2" );
					break;
				case 2:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_axis1" );
					break;
				case 3:
					ClientPrint( pl->pev, HUD_PRINTTALK, "#game_roundstart_axis2" );
					break;
				}

				PLAYBACK_EVENT_FULL( 0, pl->edict(), m_roundRestartSound, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, pl->pev->team, (int)f_Random, 0, 0 );
			}
		}
	}
}

void CDoDTeamPlay::RestartRound( void )
{
	m_RoundDoesCount = 1;
	SendTeamScores( 3 );

	MESSAGE_BEGIN( MSG_SPEC, gmsgHLTV );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 100 | 128 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_SPEC, gmsgHLTV );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	m_rRoundStartCount = gpGlobals->time;
	m_rReadyToUnFreeze = 2;
	m_RoundParaCanJoin = 1;

	ResetEnts();

	CBaseEntity *pEnt = NULL;

	pEnt = UTIL_FindEntityByClassname( NULL, "dod_preround" );
	while ( pEnt )
	{
		GetClassPtr( (CDoDPreRound *)pEnt->pev )->Launch( m_SaveRoundState );
		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_preround" );
	}

	CBaseEntity *ent1 = NULL;

	if ( cvar_timelimit.value < 0 )
		CVAR_SET_FLOAT( "timelimit", 0 );

	int i_AllieAmmount = 0, i_AxisAmmount = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr && plr->Classify() == CLASS_PLAYER )
		{
			if ( plr->has_disconnected )
				continue;

			if ( plr->pev->team == 3 )
				continue;

			if ( plr->pev->team != 0 && plr->m_iNextClass != -1 )
			{
				SendWaveStatus( plr, 0 );
				plr->HasObject = FALSE;
				plr->DelayedSpawn();

				if ( plr->pev->team == 1 )
					i_AllieAmmount++;
				else if ( plr->pev->team == 2 )
					i_AxisAmmount++;

				if ( !ent1 )
					ent1 = plr;
			}
		}
	}

	if ( ( !IsAlliesInfinite() || !IsAxisInfinite() ) && ( !i_AllieAmmount || !i_AxisAmmount ) )
	{
		m_RoundDoesCount = 0;
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#game_bogus_round" );
	}

	UTIL_SendDirectorMessage( ent1, NULL, 2 );

	m_flAxisWaveTime = m_flAlliesWaveTime = 0;
	m_flRoundTime = 0;
	m_flCanJoinTime = gpGlobals->time + 15.0;

	int index = 0;

	pEnt = UTIL_FindEntityByClassname( NULL, "dod_capture_area" );
	while ( pEnt )
	{
		pEnt->area_SetIndex( index );
		index++;
		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_capture_area" );
	}

	pEnt = UTIL_FindEntityByClassname( NULL, "dod_object_goal" );
	while ( pEnt )
	{
		pEnt->area_SetIndex( index );
		index++;
		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_object_goal" );
	}

	pEnt = UTIL_FindEntityByClassname( NULL, "func_breakable" );
	while ( pEnt )
	{
		pEnt->area_SetIndex( index );
		index++;
		pEnt = UTIL_FindEntityByClassname( pEnt, "func_breakable" );
	}
}

void CDoDTeamPlay::TeamWon( int iTeamWon )
{
	int iAllies = 0;
	int iAxis = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( p )
		{
			if ( p->pev->team == 1 )
				iAllies++;
			else if ( p->pev->team == 2 )
				iAxis++;
		}
	}

	edict_t *pGrenade = FIND_ENTITY_BY_CLASSNAME( NULL, "grenade" );

	while ( !FNullEnt( pGrenade ) )
	{
		CGrenade *pGren = GetClassPtr( (CGrenade *)VARS( pGrenade ) );
		pGren->pev->iuser1 = 1;
		pGrenade = FIND_ENTITY_BY_CLASSNAME( pGrenade, "grenade" );
	}

	pGrenade = FIND_ENTITY_BY_CLASSNAME( NULL, "grenade2" );

	while ( !FNullEnt( pGrenade ) )
	{
		CGrenade *pGren = GetClassPtr( (CGrenade *)VARS( pGrenade ) );
		pGren->pev->iuser1 = 1;
		pGrenade = FIND_ENTITY_BY_CLASSNAME( pGrenade, "grenade2" );
	}

	UTIL_LogPrintf( "\"%s\" scored \"%i\" with \"%i\" players\n", "Allies", m_iTeamScores[1], iAllies );
	UTIL_LogPrintf( "\"%s\" scored \"%i\" with \"%i\" players\n", "Axis", m_iTeamScores[2], iAxis );

	SetRoundState( 0 );
	GoodToGo();
}

void CDoDTeamPlay::TimerExpired( void )
{
	if ( m_GamePlayRules.m_bUseTimer )
	{
		switch ( m_GamePlayRules.m_iTimerTeam )
		{
		default:
			SetRoundState( RND_DRAW );
			ResetForDoDCamera();
			FireTargets( "para_draw", NULL, NULL, USE_TOGGLE, 0 );
			FireDODCamera( "axis_draw_cut", NULL, NULL, USE_TOGGLE, 0 );
			FireDODCamera( "allies_draw_cut", NULL, NULL, USE_TOGGLE, 0 );
			FireTargets( STRING( m_GamePlayRules.m_sTimerExpireTarget ), NULL, NULL, USE_TOGGLE, 0 );
			break;
		case 1:
			SetRoundState( RND_ALLIES_WIN );
			AddToTeamScores( 1, m_GamePlayRules.m_iPointsTimerExpires );
			FireDODCamera( "allies_win_cut", NULL, NULL, USE_TOGGLE, 0 );
			FireDODCamera( "axis_loose_cut", NULL, NULL, USE_TOGGLE, 0 );
			FireTargets( STRING( m_GamePlayRules.m_sTimerExpireTarget ), NULL, NULL, USE_TOGGLE, 0 );
			break;
		case 2:
			SetRoundState( RND_AXIS_WIN );
			AddToTeamScores( 2, m_GamePlayRules.m_iPointsTimerExpires );
			FireDODCamera( "axis_win_cut", NULL, NULL, USE_TOGGLE, 0 );
			FireDODCamera( "allies_loose_cut", NULL, NULL, USE_TOGGLE, 0 );
			FireTargets( STRING( m_GamePlayRules.m_sTimerExpireTarget ), NULL, NULL, USE_TOGGLE, 0 );
			break;
		}

		m_SaveRoundState = m_RoundState;
		SetRestartTime( gpGlobals->time + 7.0 );
	}
}

void CDoDTeamPlay::PlayerSpawn( CBasePlayer *pPlayer )
{
	if ( pPlayer->has_disconnected )
		return;

	if ( pPlayer->pev->iuser1 )
		return;

	if ( pPlayer->pev->team == 0 || pPlayer->pev->team == 3 || pPlayer->m_iNextClass == 0 )
		return;

	if ( pPlayer->m_bIsRandomClass )
	{
		ChooseRandomClass( pPlayer );
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_now_as", GetPlayerClassName( pPlayer->m_iNextClass ) );
	}

	pPlayer->pev->playerclass = pPlayer->m_iNextClass;
	pPlayer->m_ilastteam = 0;
	pPlayer->SetBodygroup( 1, 2 );

	if ( pPlayer->pev->playerclass )
	{
		int i;
		DodClassInfo_t *pClass;

		if ( pPlayer->pev->team == 1 )
			pPlayer->m_sPlayerModel = "us-inf";
		else if ( pPlayer->pev->team == 2 )
			pPlayer->m_sPlayerModel = "axis-inf";

		if ( pPlayer->pev->playerclass < NUM_PLAYERCLASSES && pPlayer->pev->playerclass > 0 )
		{
			if ( IsTeamPara( pPlayer->pev->team ) && pPlayer->pev->playerclass < ARRAYSIZE( g_ParaClassInfo ) )
				pClass = &g_ParaClassInfo[pPlayer->pev->playerclass];
			else
				pClass = &g_ClassInfo[pPlayer->pev->playerclass];

			pPlayer->m_sPlayerModel = pClass->modelname;

			for ( i = 0; i < 32; i++ )
			{
				if ( pClass->weapons & ( 1 << i ) )
				{
					if ( i == WEAPON_FG42 )
					{
						edict_t *pEnt = pPlayer->GiveNamedItem( "weapon_fg42" );

						if ( pPlayer->pev->playerclass == PLAYERCLASS_FG42_SCOPE )
						{
							CBasePlayerWeapon *pWeapon = GetClassPtr( (CBasePlayerWeapon *)VARS( pEnt ) );

							if ( pWeapon )
							{
								pWeapon->m_iWeaponState |= 1;
								pWeapon->Deploy();
							}
						}
					}
					else if ( i == WEAPON_ENFIELD )
					{
						edict_t *pEnt = pPlayer->GiveNamedItem( "weapon_enfield" );

						if ( pPlayer->pev->playerclass == PLAYERCLASS_BRIT_SNIPER )
						{
							CBasePlayerWeapon *pWeapon = GetClassPtr( (CBasePlayerWeapon *)VARS( pEnt ) );

							if ( pWeapon )
							{
								pWeapon->m_iWeaponState |= 1;
								pWeapon->Deploy();
							}
						}
					}
					else if ( i < 32 )
						pPlayer->GiveNamedItem( WeaponIDToChar( i ) );
				}
			}

			for ( i = 0; i < 32; i++ )
			{
				if ( pClass->weapons2 & ( 1 << i ) )
					pPlayer->GiveNamedItem( WeaponIDToChar( i + 32 ) );
			}

			if ( pPlayer->pev->team == 1 )
			{
				for ( i = 0; i < pClass->numgrenades; i++ )
					pPlayer->GiveNamedItem( "weapon_handgrenade" );
			}
			else if ( pPlayer->pev->team == 2 )
			{
				for ( i = 0; i < pClass->numgrenades; i++ )
					pPlayer->GiveNamedItem( "weapon_stickgrenade" );
			}

			pPlayer->SetBodygroup( 2, pClass->headgroup );
			pPlayer->SetBodygroup( 1, pClass->helmetgroup );
			pPlayer->SetBodygroup( 3, pClass->geargroup );
			pPlayer->SetBodygroup( 0, pClass->bodygroup );
			pPlayer->SetBodygroup( 4, 0 );
		}

		pPlayer->m_iClassSpeed = 600;
		pPlayer->CheckPlayerSpeed();
		g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", pPlayer->m_sPlayerModel );
	}
	else
		pPlayer->m_sPlayerModel = NULL;

	SendTeamScores( 3 );

	int entindex = ENTINDEX( pPlayer->edict() );

	MESSAGE_BEGIN( MSG_ALL, gmsgPStatus );
		WRITE_BYTE( entindex );
		WRITE_BYTE( pPlayer->PlayerStatus() );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgPClass );
		WRITE_BYTE( entindex );
		WRITE_BYTE( pPlayer->pev->playerclass );
	MESSAGE_END();
}

void CDoDTeamPlay::InitHUD( CBasePlayer *pPlayer )
{
	int i;

	UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_joined_game",
		( pPlayer->pev->netname && STRING(pPlayer->pev->netname)[0] != 0 ) ? STRING(pPlayer->pev->netname) : "unconnected" );

	SendMOTDToClient( pPlayer->edict() );

	// loop through all active players and send their score info to the new client
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr && plr != pPlayer )
		{
			if ( plr->m_iObjScore > 255 || plr->pev->frags > 255 || plr->m_iDeaths > 255 )
			{
				ALERT( at_aiconsole, "Init HUD data requiring short values - sending with WRITE_SHORT()\n" );

				MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfoLong, NULL, pPlayer->edict() );
					WRITE_BYTE( i );	// client number
					WRITE_SHORT( plr->m_iObjScore );
					WRITE_SHORT( plr->pev->frags );
					WRITE_SHORT( plr->m_iDeaths );
					WRITE_BYTE( plr->pev->playerclass );
					WRITE_BYTE( plr->pev->team );
					WRITE_BYTE( plr->PlayerStatus() );
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pPlayer->edict() );
					WRITE_BYTE( i );
					WRITE_BYTE( plr->m_iObjScore );
					WRITE_BYTE( plr->pev->frags );
					WRITE_BYTE( plr->m_iDeaths );
					WRITE_BYTE( plr->pev->playerclass );
					WRITE_BYTE( plr->pev->team );
					WRITE_BYTE( plr->PlayerStatus() );
				MESSAGE_END();
			}
		}
	}

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
		WRITE_BYTE( pPlayer->m_iObjScore );
		WRITE_BYTE( pPlayer->pev->frags );
		WRITE_BYTE( pPlayer->m_iDeaths );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 3 );
		WRITE_BYTE( pPlayer->PlayerStatus() );
	MESSAGE_END();

	if ( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pPlayer->edict() );
		MESSAGE_END();
	}

	UTIL_LogPrintf( "\"%s<%i><%s><>\" entered the game\n",
		STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ),
		GETPLAYERAUTHID( pPlayer->edict() ) );

	LevelChangeResets( pPlayer );

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if ( plr && plr->pev->team > 0 && plr->pev->team < 4 )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgPTeam, NULL, pPlayer->edict() );
				WRITE_BYTE( plr->entindex() );
				WRITE_BYTE( plr->pev->team );
			MESSAGE_END();
		}
	}

	SendClientGamerules( pPlayer );
	ShowVGUI( pPlayer, 2, 0 );

	pPlayer->SetThink( &CBasePlayer::ServerSpawnCheck );
	pPlayer->pev->nextthink = 2;

	pPlayer->SetMapMarker( 0 );
}

void CDoDTeamPlay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int flags )
{
	if ( flags != 3 && flags != 4 && flags != 5 && flags != 6 )
		flags = 0;

	if ( IsTeamInfinite( pVictim->pev->team ) )
	{
		if ( !( pVictim->m_flDisplayHistory & 0x10000 ) )
		{
			pVictim->m_flDisplayHistory |= 0x10000;
			pVictim->HintMessage( "#Hint_player_killed_wavetime", 0 );
		}
	}
	else if ( !( pVictim->m_flDisplayHistory & 0x20000 ) )
	{
		pVictim->m_flDisplayHistory |= 0x20000;
		pVictim->HintMessage( "#Hint_player_killed_onelife", 0 );
	}

	if ( !m_DisableDeathPenalty )
	{
		DeathNotice( pVictim, pKiller, pInflictor, flags );

		if ( !m_RoundDoesCount )
			return;

		pVictim->m_iDeaths += 1;

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreShort );
			WRITE_BYTE( ENTINDEX( pVictim->edict() ) );
			WRITE_SHORT( pVictim->m_iObjScore );
			WRITE_SHORT( pVictim->pev->frags );
			WRITE_SHORT( pVictim->m_iDeaths );
			WRITE_BYTE( 1 );
		MESSAGE_END();

		float tkPenalty = pcvar_mp_tkpenalty ? pcvar_mp_tkpenalty->value : -1;

		FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

		CBasePlayer *peKiller = NULL;
		CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );

		if ( ktmp && ( ktmp->Classify() == CLASS_PLAYER ) )
			peKiller = (CBasePlayer *)ktmp;

		if ( ktmp && ktmp->IsPlayer() )
		{
			CBasePlayer *p = (CBasePlayer *)ktmp;
			CBasePlayer *killer = GetClassPtr( (CBasePlayer *)pKiller );

			if ( p && pVictim->pev != pKiller )
			{
				int points = IPointsForKill( killer, pVictim );

				p->AddFrags( points, TRUE );

				FireTargets( "game_playerkill", ktmp, ktmp, USE_TOGGLE, 0 );

#ifdef _MSC_VER
#pragma warning( suppress : 6011 )
#endif
				if ( pVictim->pev->team == peKiller->pev->team && tkPenalty > 0 )
					peKiller->m_iNumTKs++;
				else
					peKiller->m_iNumTKs = 0;

				if ( pVictim->pev->team == peKiller->pev->team )
				{
					if ( tkPenalty > 0 )
						peKiller->m_iNumTKs++;

					if ( !( killer->m_flDisplayHistory & 0x10 ) )
					{
						killer->m_flDisplayHistory |= 0x10;
						killer->HintMessage( "#Hint_careful_around_teammates", 1 );
					}
				}
				else
				{
					peKiller->m_iNumTKs = 0;

					if ( !( killer->m_flDisplayHistory & 0x20 ) )
					{
						killer->m_flDisplayHistory |= 0x20;
						killer->HintMessage( "#Hint_killing_enemies_is_good", 0 );
					}
				}

				p->m_flNextDecalTime = gpGlobals->time;

				if ( tkPenalty > 0 && p->m_iNumTKs >= tkPenalty && ( p->entindex() != 1 || IS_DEDICATED_SERVER() ) )
				{
					UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_kick_tk",
						( p->pev->netname && STRING( p->pev->netname )[0] != 0 ) ? STRING( p->pev->netname ) : "unconnected" );

					UTIL_LogPrintf( "\"%s<%i>\" has been auto kicked from the game for TKing\n",
						STRING( p->pev->netname ),
						GETPLAYERUSERID( peKiller->edict() ) );

					ClientPrint( p->pev, HUD_PRINTCONSOLE, "#dod_kicked_reason_tk" );
					CLIENT_COMMAND( p->edict(), "disconnect\n" );
				}
			}
		}
	}
}

void CDoDTeamPlay::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor, int flags )
{
	bool bDeathMsg = ( pcvar_mp_deathmsg && pcvar_mp_deathmsg->value > 0 );

	// Work out what killed the player, and send a message to all clients about it
	CBaseEntity *Killer;
	Killer = CBaseEntity::Instance( pKiller );

	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_weapon = 0;
	int killer_index = 0;

	CBasePlayerItem *pItem = NULL;

	if ( pKiller->flags & FL_CLIENT )
	{
		killer_index = ENTINDEX( ENT( pKiller ) );

		if ( pevInflictor )
		{
			if ( pevInflictor == pKiller )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pKiller );

				if ( pPlayer->m_pActiveItem )
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
					killer_weapon = pPlayer->m_pActiveItem->m_iId;
					pItem = pPlayer->m_pActiveItem;
				}
			}
			else
			{
				killer_weapon_name = STRING( pevInflictor->classname );  // it's just that easy

				if ( !strcmp( killer_weapon_name, "grenade" ) )
					killer_weapon = WEAPON_HANDGRENADE;
				else if ( !strcmp( killer_weapon_name, "grenade2" ) )
					killer_weapon = WEAPON_STICKGRENADE;
			}
		}
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if ( !memcmp( killer_weapon_name, "weapon_", 7 ) )
		killer_weapon_name += 7;
	else if ( !memcmp( killer_weapon_name, "monster_", 8 ) )
		killer_weapon_name += 8;
	else if ( !memcmp( killer_weapon_name, "func_", 5 ) )
		killer_weapon_name += 5;
	else if ( !memcmp( killer_weapon_name, "shell_", 6 ) )
		killer_weapon_name += 6;

	if ( killer_weapon == WEAPON_STICKGRENADE_EX )
		killer_weapon_name = "stick";
	else if ( killer_weapon == WEAPON_SCOPEDKAR )
		killer_weapon_name = "scopedkar";

	if ( FStrEq( killer_weapon_name, "kar" ) && flags == 3 )
	{
		killer_weapon_name = "bayonet";
		killer_weapon = 37;
	}
	else if ( FStrEq( killer_weapon_name, "garand" ) && flags == 4 )
	{
		killer_weapon_name = "garandbutt";
		killer_weapon = 42;
	}
	else if ( FStrEq( killer_weapon_name, "k43" ) && flags == 6 )
	{
		killer_weapon_name = "k43butt";
		killer_weapon = 42;
	}
	else if ( FStrEq( killer_weapon_name, "enfield" ) && flags == 5 )
	{
		killer_weapon_name = "enf_bayonet";
		killer_weapon = 43;
	}
	else if ( FStrEq( killer_weapon_name, "m1carbine" ) && IsTeamPara( pKiller->team ) )
	{
		killer_weapon_name = "fcarbine";
		killer_weapon = 36;
	}
	else if ( pItem && FStrEq( killer_weapon_name, "fg42" ) )
	{
		CBasePlayerWeapon *pWpn = (CBasePlayerWeapon *)pItem;

		if ( pWpn && ( pWpn->m_iWeaponState & 1 ) )
		{
			killer_weapon_name = "scoped_fg42";
			killer_weapon = 35;
		}
	}
	else if ( pItem && FStrEq( killer_weapon_name, "enfield" ) )
	{
		CBasePlayerWeapon *pWpn = (CBasePlayerWeapon *)pItem;

		if ( pWpn && ( pWpn->m_iWeaponState & 1 ) )
		{
			killer_weapon_name = "scoped_enfield";
			killer_weapon = 38;
		}
	}
	else if ( FStrEq( killer_weapon_name, "grenade" ) && IsAlliesBrit() )
	{
		killer_weapon_name = "mills_bomb";
		killer_weapon = 39;
	}
	else if ( FStrEq( killer_weapon_name, "amerknife" ) && IsAlliesBrit() )
	{
		killer_weapon_name = "brit_knife";
		killer_weapon = 40;
	}
	else if ( FStrEq( killer_weapon_name, "bazooka" ) )
	{
		killer_weapon = WEAPON_BAZOOKA;
	}
	else if ( FStrEq( killer_weapon_name, "pschreck" ) )
	{
		killer_weapon = WEAPON_PSCHRECK;
	}
	else if ( FStrEq( killer_weapon_name, "piat" ) )
	{
		killer_weapon = WEAPON_PIAT;
	}
	else if ( FStrEq( killer_weapon_name, "mortar" ) )
	{
		killer_weapon = WEAPON_MORTAR;
	}

	CBasePlayer *client;
	client = NULL;

	if ( bDeathMsg )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
			WRITE_BYTE( killer_index );						// the killer
			WRITE_BYTE( ENTINDEX( pVictim->edict() ) );		// the victim
			WRITE_BYTE( killer_weapon );		// what they were killed by (should this be a string?)
		MESSAGE_END();
	}

	int hltv_priority = 0;

	if ( pVictim->pev == pKiller )
	{
		// killed self
		if ( pVictim->pev->team == 1 )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				"Allies",
				killer_weapon_name );
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				"Axis",
				killer_weapon_name );
		}

		hltv_priority = 0;
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance( pKiller );

		if ( pKiller->team == 1 )
		{
			if ( pVictim->pev->team == 1 )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
					STRING( pInflictor->pev->netname ),
					GETPLAYERUSERID( pInflictor->edict() ),
					GETPLAYERAUTHID( pInflictor->edict() ),
					"Allies",
					STRING( pVictim->pev->netname ),
					GETPLAYERUSERID( pVictim->edict() ),
					GETPLAYERAUTHID( pVictim->edict() ),
					"Allies",
					killer_weapon_name );
			}
			else
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
					STRING( pInflictor->pev->netname ),
					GETPLAYERUSERID( pInflictor->edict() ),
					GETPLAYERAUTHID( pInflictor->edict() ),
					"Allies",
					STRING( pVictim->pev->netname ),
					GETPLAYERUSERID( pVictim->edict() ),
					GETPLAYERAUTHID( pVictim->edict() ),
					"Axis",
					killer_weapon_name );
			}
		}
		else
		{
			if ( pVictim->pev->team == 1 )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
					STRING( pInflictor->pev->netname ),
					GETPLAYERUSERID( pInflictor->edict() ),
					GETPLAYERAUTHID( pInflictor->edict() ),
					"Axis",
					STRING( pVictim->pev->netname ),
					GETPLAYERUSERID( pVictim->edict() ),
					GETPLAYERAUTHID( pVictim->edict() ),
					"Allies",
					killer_weapon_name );
			}
			else
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
					STRING( pInflictor->pev->netname ),
					GETPLAYERUSERID( pInflictor->edict() ),
					GETPLAYERAUTHID( pInflictor->edict() ),
					"Axis",
					STRING( pVictim->pev->netname ),
					GETPLAYERUSERID( pVictim->edict() ),
					GETPLAYERAUTHID( pVictim->edict() ),
					"Axis",
					killer_weapon_name );
			}
		}

		if ( pevInflictor )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pKiller );

			if ( pPlayer->m_pActiveItem )
			{
				switch ( pPlayer->m_pActiveItem->m_iId )
				{
				case WEAPON_HANDGRENADE:
				case WEAPON_STICKGRENADE:
				case WEAPON_STICKGRENADE_EX:
				case WEAPON_HANDGRENADE_EX:
					hltv_priority = 5;
					break;
				case WEAPON_AMERKNIFE:
				case WEAPON_GERKNIFE:
				case WEAPON_SPADE:
					hltv_priority = 7;
					break;
				case WEAPON_SCOPEDKAR:
				case WEAPON_SPRING:
					hltv_priority = 5;
					break;
				default:
					hltv_priority = 4;
					break;
				}
			}
		}
	}
	else
	{
		// killed by the world
		if ( pVictim->pev->team == 1 )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n",
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				"Allies",
				killer_weapon_name );
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n",
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				"Axis",
				killer_weapon_name );
		}

		hltv_priority = 4;
	}

	CheckOneLifeRoundConditions();

	CBaseEntity *Inflictor = NULL;

	if ( !FNullEnt( pevInflictor ) )
		Inflictor = CBaseEntity::Instance( pevInflictor );
	else if ( !FNullEnt( pKiller ) )
		Inflictor = CBaseEntity::Instance( pKiller );

	UTIL_SendDirectorMessage( pVictim, Inflictor, hltv_priority );
}

void CDoDTeamPlay::Think( void )
{
	int time_remaining;
	time_remaining = 0;

	g_VoiceGameMgr.Update( gpGlobals->frametime );

	if ( g_fGameOver )   // someone else quit the game already
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *player = UTIL_PlayerByIndex( i );

			if ( !player )
				continue;

			if ( player->IsAlive() )
				continue;

			((CBasePlayer *)player)->StopObserver();
			SendWaveStatus( (CBasePlayer *)player, 0 );
		}

		int time = pcvar_mp_chattime ? (int)pcvar_mp_chattime->value : 10;
		if ( time < 10 )
			CVAR_SET_FLOAT( "mp_chattime", 10 );
		else if ( time > 120 )
			CVAR_SET_FLOAT( "mp_chattime", 120 );

		m_flIntermissionEndTime = g_flIntermissionStartTime + pcvar_mp_chattime->value;

		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->time )
		{
			if ( m_iEndIntermissionButtonHit  // check that someone has pressed a key, or the max intermission time is over
				|| ( ( g_flIntermissionStartTime + 120 ) < gpGlobals->time) )
			{
				int iAllies = 0;
				int iAxis = 0;

				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBaseEntity *p = UTIL_PlayerByIndex( i );

					if ( p )
					{
						if ( p->pev->team == 1 )
							iAllies++;
						else if ( p->pev->team == 2 )
							iAxis++;
					}
				}

				UTIL_LogPrintf( "\"%s\" scored \"%i\" with \"%i\" players\n", "Allies", g_pGameRules->TeamScores( 1 ), iAllies );
				UTIL_LogPrintf( "\"%s\" scored \"%i\" with \"%i\" players\n", "Axis", g_pGameRules->TeamScores( 2 ), iAxis );

				g_pGameRules->SetMapChangeFromTime( 0 );
				ChangeLevel(); // intermission is over
			}
		}

		return;
	}

	int iTimeLimit = get_maptime_left();
	float timelimit = pcvar_mp_timelimit ? pcvar_mp_timelimit->value : 60;
	timelimit = timelimit * 60;

	if ( iTimeLimit <= 0 && (int)timelimit != 0 )
	{
		GoToIntermission();
		return;
	}

	if ( m_fLogScoresTime < gpGlobals->time && pcvar_mp_log_scores && pcvar_mp_log_scores->value > 0 )
	{
		float delay = pcvar_mp_log_scores ? pcvar_mp_log_scores->value : 10;
		if ( delay < 1 )
			delay = 1;
		if ( delay > 600 )
			delay = 600;

		UTIL_LogPrintf( "Team Scores: Allies: %d, Axis: %d\n", g_pGameRules->TeamScores( 1 ), g_pGameRules->TeamScores( 2 ) );
		m_fLogScoresTime = delay + gpGlobals->time;
	}

	if ( m_RoundState == RND_RUNNING && ( !IsAlliesInfinite() || !IsAxisInfinite() ) )
	{
		m_flRoundTime += gpGlobals->frametime;
		if ( m_flRoundTime <= 30 )
			m_RoundParaCanJoin = TRUE;
		else
			m_RoundParaCanJoin = FALSE;
	}

	if ( IsClanMatch() )
	{
		if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value <= 0 )
		{
			SetClanMatch( false );
			setMapTime();
		}
		else if ( pcvar_mp_clan_restartround && pcvar_mp_clan_restartround->value > 0 )
		{
			StartClanRestart();
			CVAR_SET_FLOAT( "mp_clan_restartround", 0 );
		}
		else if ( pcvar_mp_clan_readyrestart && pcvar_mp_clan_readyrestart->value > 0 )
		{
			m_bAwaitingReadyRestart = TRUE;
			m_bHeardAlliesReady = FALSE;
			m_bHeardAxisReady = FALSE;

			char *readyString = (char *)CVAR_GET_STRING( "mp_clan_ready_signal" );
			UTIL_ClientPrintAll( HUD_PRINTCENTER, "#clan_ready_rules", readyString );
			CVAR_SET_FLOAT( "mp_clan_readyrestart", 0 );
		}
		else if ( pcvar_mp_clan_match_warmup && pcvar_mp_clan_match_warmup->value > 0 )
		{
			UTIL_ClientPrintAll( HUD_PRINTCENTER, "#clan_warmup_rules" );
			ReturnToWarmup();
			CVAR_SET_FLOAT( "mp_clan_match_warmup", 0 );
		}
		else if ( m_bAwaitingReadyRestart && m_bHeardAlliesReady && m_bHeardAxisReady )
		{
			StartClanRestart();
			m_bAwaitingReadyRestart = FALSE;
			m_bHeardAlliesReady = FALSE;
			m_bHeardAxisReady = FALSE;
		}
		else
		{
			CheckClanRestart();
		}
	}
	else if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0 )
	{
		SetClanMatch( true );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "#clan_warmup_rules" );
	}

	if ( m_RoundState == RND_GOODTOGO )
	{
		if ( gpGlobals->time >= m_flGoodToGoTime && !m_rReadyToUnFreeze )
			GoodToGo();
		else if ( !g_fGameOver && m_rRoundStartCount <= gpGlobals->time && m_rReadyToUnFreeze == 2 )
			RoundFreeze();
		else if ( !g_fGameOver && m_rRoundStartCount <= gpGlobals->time && m_rReadyToUnFreeze == 1 && m_DoDCameraState <= 0 )
			RoundUnfreeze();
	}

	if ( !g_fGameOver && m_flAlliesWaveTime != 0 && m_RoundState == RND_RUNNING &&
		(int)m_flAlliesRespawn - (int)gpGlobals->time <= 0 )
	{
		RespawnAllies();
		m_flAlliesWaveTime = 0;
	}

	if ( !g_fGameOver && m_flAxisWaveTime != 0 && m_RoundState == RND_RUNNING &&
		(int)m_flAxisRespawn - (int)gpGlobals->time <= 0 )
	{
		RespawnAxis();
		m_flAxisWaveTime = 0;
	}

	if ( !g_fGameOver && m_vKickSessionState == 1 && m_vKickTime <= gpGlobals->time )
		VoteKickTimeUp();

	if ( gpGlobals->time >= m_flRestartTime &&
		( m_RoundState == RND_ALLIES_WIN || m_RoundState == RND_AXIS_WIN || m_RoundState == RND_DRAW ) &&
		!g_fGameOver && m_DoDCameraState < 1 )
	{
		m_SaveRoundState = m_RoundState;
		TeamWon( m_RoundState );
		m_flRestartTime = gpGlobals->time + 3.0;
	}
}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	mapcycle_item_s* next;
	char mapname[32];
	int minplayers;
	int maxplayers;
	char rulebuffer[MAX_RULE_BUFFER];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	mapcycle_item_t* items;
	mapcycle_item_t* next_item;
} mapcycle_t;

void DestroyMapCycle( mapcycle_t *cycle )
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if ( p )
	{
		start = p;
		p = p->next;
		while ( p != start )
		{
			n = p->next;
			delete p;
			p = n;
		}

		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

char *COM_Parse (char *data)
{
	int             c;
	int             len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}

// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}


// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			if ( len < (int)sizeof( com_token ) - 1 )
			{
				com_token[len] = c;
				len++;
			}
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		if ( len < (int)sizeof( com_token ) - 1 )
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while (c>32);

	com_token[len] = 0;
	return data;
}

int COM_TokenWaiting( char *buffer )
{
	char *p;

	p = buffer;
	while ( *p && *p!='\n')
	{
		if ( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}

int ReloadMapCycleFile( char *filename, mapcycle_t *cycle )
{
	char szBuffer[ MAX_RULE_BUFFER ];
	char szMap[ 32 ];
	int length;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( filename, &length );
	int hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if ( pFileList && length )
	{
		// the first map name in the file becomes the default
		while ( 1 )
		{
			hasbuffer = 0;
			memset( szBuffer, 0, MAX_RULE_BUFFER );

			pFileList = COM_Parse( pFileList );
			if ( strlen( com_token ) <= 0 )
				break;

			strncpy( szMap, com_token, sizeof( szMap ) );
			szMap[ sizeof( szMap ) - 1 ] = '\0';

			// Any more tokens on this line?
			if ( COM_TokenWaiting( pFileList ) )
			{
				pFileList = COM_Parse( pFileList );
				if ( strlen( com_token ) > 0 )
				{
					hasbuffer = 1;
					strncpy( szBuffer, com_token, sizeof( szBuffer ) );
					szBuffer[ sizeof( szBuffer ) - 1 ] = '\0';
				}
			}

			// Check map
			if ( IS_MAP_VALID( szMap ) )
			{
				// Create entry
				char *s;

				item = new mapcycle_item_s;

				strcpy( item->mapname, szMap );

				item->minplayers = 0;
				item->maxplayers = 0;

				memset( item->rulebuffer, 0, MAX_RULE_BUFFER );

				if ( hasbuffer )
				{
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "minplayers" );
					if ( s && s[0] )
					{
						item->minplayers = atoi( s );
						item->minplayers = max( item->minplayers, 0 );
						item->minplayers = min( item->minplayers, gpGlobals->maxClients );
					}
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "maxplayers" );
					if ( s && s[0] )
					{
						item->maxplayers = atoi( s );
						item->maxplayers = max( item->maxplayers, 0 );
						item->maxplayers = min( item->maxplayers, gpGlobals->maxClients );
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "minplayers" );
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "maxplayers" );

					strcpy( item->rulebuffer, szBuffer );
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT( at_console, "Skipping %s from mapcycle, not a valid map\n", szMap );
			}

		}

		FREE_FILE( aFileList );
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while ( item )
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if ( !item )
	{
		return 0;
	}

	while ( item->next )
	{
		item = item->next;
	}
	item->next = cycle->items;

	cycle->next_item = item->next;

	return 1;
}

int CountPlayers( void )
{
	int	num = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if ( pEnt )
		{
			num = num + 1;
		}
	}

	return num;
}

void ExtractCommandString( char *s, char *szCommand )
{
	// Now make rules happen
	char	pkey[512];
	char	value[512];	// use two buffers so compares
								// work without stomping on each other
	char	*o;

	if ( *s == '\\' )
		s++;

	while (1)
	{
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s )
				return;
			if ( o < pkey + sizeof( pkey ) - 1 )
				*o++ = *s;
			s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			if ( o < value + sizeof( value ) - 1 )
				*o++ = *s;
			s++;
		}
		*o = 0;

		strcat( szCommand, pkey );
		if ( strlen( value ) > 0 )
		{
			strcat( szCommand, " " );
			strcat( szCommand, value );
		}
		strcat( szCommand, "\n" );

		if (!*s)
			return;
		s++;
	}
}

char *CDoDTeamPlay::COM_Parse (char *data)
{
	int             c;
	int             len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;
		data++;
	}

	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}


	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			if ( len < (int)sizeof( com_token ) - 1 )
			{
				com_token[len] = c;
				len++;
			}
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

	do
	{
		if ( len < (int)sizeof( com_token ) - 1 )
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while (c>32);

	com_token[len] = 0;
	return data;
}

int CDoDTeamPlay::COM_TokenWaiting( char *buffer )
{
	char *p;

	p = buffer;
	while ( *p && *p!='\n')
	{
		if ( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}

void CDoDTeamPlay::SetNextMap( void )
{
	static char szPreviousMapCycleFile[ 256 ];
	static mapcycle_t mapcycle;

	char szFirstMapInList[32];
	char szCommands[ 1500 ];
	char szRules[ 1500 ];
	int minplayers, maxplayers;
	minplayers = 0;
	maxplayers = 0;
	strcpy( szFirstMapInList, "dod_caen" );

	BOOL do_cycle = TRUE;

	// find the map to change to
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[ 0 ] = '\0';
	szRules[ 0 ] = '\0';

#ifdef _MSC_VER
#pragma warning( suppress : 6054 )
#endif
	if ( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		strncpy( szPreviousMapCycleFile, mapcfile, sizeof(szPreviousMapCycleFile) - 1 );
		szPreviousMapCycleFile[sizeof(szPreviousMapCycleFile) - 1] = 0;

		DestroyMapCycle( &mapcycle );

		if ( !ReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = FALSE;
		}
	}

	if ( do_cycle && mapcycle.items )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		strncpy( sNextMap, STRING(gpGlobals->mapname), sizeof( sNextMap ) - 1 );
		sNextMap[sizeof( sNextMap ) - 1] = 0;
		strncpy( szFirstMapInList, STRING(gpGlobals->mapname), sizeof( szFirstMapInList ) - 1 );
		szFirstMapInList[sizeof( szFirstMapInList ) - 1] = 0;

		// Traverse list
		for ( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			ASSERT( item != NULL );

			if ( keeplooking )
				continue;

			found = TRUE;
			break;
		}

		if ( !found )
		{
			item = mapcycle.next_item;
		}

		// Increment next item pointer
		mapcycle.next_item = item->next;

		strcpy( sNextMap, item->mapname );
	}

	if ( !IS_MAP_VALID(sNextMap) )
	{
		strcpy( sNextMap, szFirstMapInList );
	}
}

void CDoDTeamPlay::ChangeLevel( void )
{
	static char szPreviousMapCycleFile[ 256 ];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[ 1500 ];
	char szRules[ 1500 ];
	int minplayers = 0, maxplayers = 0;
	strcpy( szFirstMapInList, "dod_caen" );

	int	curplayers;
	BOOL do_cycle = TRUE;

	// find the map to change to
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[ 0 ] = '\0';
	szRules[ 0 ] = '\0';

	curplayers = CountPlayers();

	// Has the map cycle filename changed?
#ifdef _MSC_VER
#pragma warning( suppress : 6054 )
#endif
	if ( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		strncpy( szPreviousMapCycleFile, mapcfile, sizeof(szPreviousMapCycleFile) - 1 );
		szPreviousMapCycleFile[sizeof(szPreviousMapCycleFile) - 1] = 0;

		DestroyMapCycle( &mapcycle );

		if ( !ReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = FALSE;
		}
	}

	if ( do_cycle && mapcycle.items )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		// Assume current map
		strncpy( szNextMap, STRING(gpGlobals->mapname), sizeof( szNextMap ) - 1 );
		szNextMap[sizeof( szNextMap ) - 1] = 0;
		strncpy( szFirstMapInList, STRING(gpGlobals->mapname), sizeof( szFirstMapInList ) - 1 );
		szFirstMapInList[sizeof( szFirstMapInList ) - 1] = 0;

		// Traverse list
		for ( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			ASSERT( item != NULL );

			if ( item->minplayers != 0 )
			{
				if ( curplayers >= item->minplayers )
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( item->maxplayers != 0 )
			{
				if ( curplayers <= item->maxplayers )
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( keeplooking )
				continue;

			found = TRUE;
			break;
		}

		if ( !found )
		{
			item = mapcycle.next_item;
		}

		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy( szNextMap, item->mapname );

		ExtractCommandString( item->rulebuffer, szCommands );
		strcpy( szRules, item->rulebuffer );
	}

	if ( !IS_MAP_VALID(szNextMap) )
	{
		strcpy( szNextMap, szFirstMapInList );
	}

	g_fGameOver = TRUE;

	if ( IsClanMatch() )
		ReturnToWarmup();

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	if ( minplayers || maxplayers )
	{
		ALERT( at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers );
	}
	if ( strlen( szRules ) > 0 )
	{
		ALERT( at_console, "RULES:  %s\n", szRules );
	}

	CHANGE_LEVEL( szNextMap, NULL );
	if ( strlen( szCommands ) > 0 )
	{
		SERVER_COMMAND( szCommands );
	}
}

void CDoDTeamPlay::SendMOTDToClient( edict_t* client )
{
	enum { MAX_MOTD_CHUNK = 60, MAX_MOTD_LENGTH = 1536 };
	int length;
	int characters = 0;
	char* file = (char*)LOAD_FILE_FOR_ME( (char*)CVAR_GET_STRING( "motdfile" ), &length );
	char* data = file;
	MESSAGE_BEGIN( MSG_ONE, gmsgServerName, NULL, client );
		WRITE_STRING( CVAR_GET_STRING( "hostname" ) );
	MESSAGE_END();
	while ( data && *data && characters < MAX_MOTD_LENGTH )
	{
		char chunk[MAX_MOTD_CHUNK + 1];
		if ( strlen( data ) < MAX_MOTD_CHUNK )
			strcpy( chunk, data );
		else
		{
			strncpy( chunk, data, MAX_MOTD_CHUNK );
			chunk[MAX_MOTD_CHUNK] = 0;
		}
		characters += strlen( chunk );
		if ( characters < MAX_MOTD_LENGTH )
			data = file + characters;
		else
			*data = 0;
		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( *data ? FALSE : TRUE );
			WRITE_STRING( chunk );
		MESSAGE_END();
	}
	FREE_FILE( file );
}

enum
{
	DOD_TEAM_ALLIES = 1,
	DOD_TEAM_AXIS = 2,
	DOD_TEAM_SPECTATORS = 3,
	DOD_TEAM_AUTOASSIGN = 5
};

void CDoDTeamPlay::TeamSelect( CBasePlayer *pPlayer, int iTeamSelect )
{
	if ( iTeamSelect < 1 || iTeamSelect > 5 || iTeamSelect == 4 )
		return;

	if ( iTeamSelect == 3 && pcvar_mp_allowspectators && pcvar_mp_allowspectators->value <= 0 )
	{
		ShowVGUI( pPlayer, 2, 0 );
		return;
	}

	if ( iTeamSelect == 5 )
	{
		int diff = CountTeamMembers( 1 );

		if ( diff < 0 )
			iTeamSelect = 1;
		else if ( diff > 0 )
			iTeamSelect = 2;
		else
			iTeamSelect = ( m_iTeamScores[1] >= m_iTeamScores[2] ) + 1;
	}

	int firstTime = 0;
	bool b_playableteam;
	b_playableteam = ( iTeamSelect == 1 || iTeamSelect == 2 );

	if ( pPlayer->pev->team == iTeamSelect )
		return;

	float teamOver = pcvar_mp_teamlimit ? pcvar_mp_teamlimit->value : 0;

	if ( teamOver > 0 && iTeamSelect != 5 && iTeamSelect != 3 && CountTeamMembers( iTeamSelect ) >= teamOver )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#game_uneven_teams" );
		ShowVGUI( pPlayer, 2, 0 );
		return;
	}

	if ( pPlayer->pev->team < 1 || pPlayer->pev->team > 3 )
		firstTime = 1;

	if ( pPlayer->m_ilastteam == 1 && pPlayer->pev->team != 3 )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#game_switch_teams_once" );
		return;
	}

	if ( pPlayer->pev->team > 0 && pPlayer->pev->team != iTeamSelect )
		pPlayer->m_ilastteam = 1;

	pPlayer->m_iNextClass = -1;

	if ( pPlayer->pev->team != 3 )
	{
		pPlayer->SetThink( &CBasePlayer::ServerSpawnCheck );
		pPlayer->pev->nextthink = gpGlobals->time;
	}

	pPlayer->pev->playerclass = 0;
	pPlayer->m_bIsRandomClass = false;

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" joined team \"%s\"\n",
		STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ),
		GETPLAYERAUTHID( pPlayer->edict() ),
		g_szTeamNames[pPlayer->pev->team],
		g_szTeamNames[iTeamSelect] );

	ChangePlayerTeam( pPlayer, iTeamSelect, TRUE, FALSE );

	pPlayer->pev->team = iTeamSelect;

	if ( iTeamSelect == 3 )
	{
		pPlayer->m_iJoiningState = JOINED;
		pPlayer->UTIL_EdictScreenFade( pPlayer->edict(), Vector( 0, 0, 0 ), 1, 15, 0, FFADE_OUT | FFADE_STAYOUT );
		pPlayer->pev->deadflag = DEAD_DEAD;
		pPlayer->pev->effects |= EF_NOINTERP;
		pPlayer->m_bIsObserver = TRUE;
		pPlayer->StartObserver( pPlayer->pev->origin, pPlayer->pev->angles );
	}
	else
	{
		if ( getWaveTime( pPlayer->pev->team ) <= 0 &&
			( ( iTeamSelect == 1 && IsAlliesInfinite() ) || ( iTeamSelect == 2 && IsAxisInfinite() ) ) &&
			firstTime != 1 )
		{
			float waveTime = GetMaxWaveTime( pPlayer->pev->team );

			if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0 )
			{
				float tempTime = pcvar_mp_clan_respawntime ? pcvar_mp_clan_respawntime->value : -1;

				if ( tempTime > 0 )
					waveTime = tempTime;
			}

			if ( waveTime <= 3 )
				waveTime = 3;

			if ( m_RoundState != RND_RUNNING )
				waveTime = 0.5;

			SendWaveStatus( pPlayer, 1 );
			setWaveTime( pPlayer->pev->team, waveTime );
		}
		else if ( firstTime != 1 && m_RoundState == RND_RUNNING )
		{
			SendWaveStatus( pPlayer, 1 );
		}

		ShowClassSelectMenu( pPlayer );

		MESSAGE_BEGIN( MSG_ALL, gmsgPClass );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_BYTE( pPlayer->pev->playerclass );
		MESSAGE_END();
	}

#ifdef _MSC_VER
#pragma warning( suppress : 6385 )
#endif
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_joined_team", STRING( pPlayer->pev->netname ), g_szTeamNames[iTeamSelect] );

	CheckOneLifeRoundConditions();
}

void CDoDTeamPlay::ClassSelect( CBasePlayer *pPlayer, int iClassSelect )
{
	if ( pPlayer->pev->team == 3 )
		return;

	pPlayer->m_bIsRandomClass = false;

	int classid = iClassSelect;
	int team = pPlayer->pev->team;
	BOOL b_isBrit = IsAlliesBrit();

	if ( classid == PLAYERCLASS_RANDOM )
	{
		if ( pPlayer->m_iNextClass == -1 && pPlayer->m_imissedwave && m_RoundState == RND_RUNNING )
			pPlayer->m_irdytospawn = 1;

		pPlayer->m_bIsRandomClass = true;
		pPlayer->m_iNextClass = classid;

		if ( pPlayer->IsAlive() )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_respawn_asrandom" );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_spawn_asrandom" );

			if ( pPlayer->m_iJoiningState != JOINED )
				pPlayer->m_iJoiningState = READYTOGO;
		}

		if ( team == 1 )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed role to \"Random\"\n",
				STRING( pPlayer->pev->netname ),
				GETPLAYERUSERID( pPlayer->edict() ),
				GETPLAYERAUTHID( pPlayer->edict() ),
				"Allies" );
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed role to \"Random\"\n",
				STRING( pPlayer->pev->netname ),
				GETPLAYERUSERID( pPlayer->edict() ),
				GETPLAYERAUTHID( pPlayer->edict() ),
				"Axis" );
		}

		if ( pPlayer->m_irdytospawn )
		{
			if ( !IsTeamInfinite( pPlayer->pev->team ) )
			{
				if ( m_RoundState != RND_RUNNING )
					pPlayer->DelayedSpawn();
			}
			else
				pPlayer->DelayedSpawn();
		}
	}
	else
	{
		if ( pPlayer->pev->team == 1 && b_isBrit && ( classid < PLAYERCLASS_BRIT_LIGHTINF || classid > PLAYERCLASS_BRIT_MORTAR ) )
			return;

		if ( pPlayer->pev->team == 1 && !b_isBrit && ( classid < PLAYERCLASS_LIGHTINF_GARAND || classid > PLAYERCLASS_MORTAR ) )
			return;

		if ( pPlayer->pev->team == 2 && ( classid < PLAYERCLASS_LEICHTINF_KAR || classid > PLAYERCLASS_GERMORTAR ) )
			return;

		if ( classid == PLAYERCLASS_UNDEFINED )
			return;

		char *classname = GetPlayerClassName( classid );

		if ( classid != pPlayer->pev->playerclass && !CanPlayerJoinClass( classid ) )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_class_limit", classname );
			ShowClassSelectMenu( pPlayer );
			return;
		}

		pPlayer->m_iNextClass = classid;

		if ( pPlayer->m_imissedwave && m_RoundState == RND_RUNNING )
			pPlayer->m_irdytospawn = 1;

		if ( pPlayer->IsAlive() )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_respawn_as", classname );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_spawn_as", classname );

			if ( pPlayer->m_iJoiningState != JOINED )
				pPlayer->m_iJoiningState = READYTOGO;
		}

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed role to \"%s\"\n",
			STRING( pPlayer->pev->netname ),
			GETPLAYERUSERID( pPlayer->edict() ),
			GETPLAYERAUTHID( pPlayer->edict() ),
			( team == 1 ) ? "Allies" : "Axis",
			classname );

		if ( pPlayer->m_irdytospawn )
		{
			if ( !IsTeamInfinite( pPlayer->pev->team ) )
			{
				if ( m_RoundState != RND_RUNNING )
					pPlayer->DelayedSpawn();
			}
			else
				pPlayer->DelayedSpawn();
		}
	}
}

BOOL CDoDTeamPlay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	const char *pstr = NULL;

	if ( g_VoiceGameMgr.ClientCommand( pPlayer, pcmd ) )
		return TRUE;

	if ( FStrEq( pcmd, "spectate" ) && ( pPlayer->pev->flags & FL_PROXY ) )
	{
		pPlayer->pev->flags |= FL_PROXY;
		pPlayer->m_iNextClass = -1;
		pPlayer->pev->playerclass = 0;
		pPlayer->m_bIsRandomClass = false;
		pPlayer->pev->team = 3;

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" joined team \"%s\"\n",
			STRING( pPlayer->pev->netname ),
			GETPLAYERUSERID( pPlayer->edict() ),
			GETPLAYERAUTHID( pPlayer->edict() ),
			g_szTeamNames[0],
			g_szTeamNames[pPlayer->pev->team] );

		pPlayer->m_iJoiningState = JOINED;
		ChangePlayerTeam( pPlayer, pPlayer->pev->team, FALSE, FALSE );
		pPlayer->UTIL_EdictScreenFade( pPlayer->edict(), Vector( 0, 0, 0 ), 1, 15, 0, FFADE_OUT | FFADE_STAYOUT );
		pPlayer->pev->deadflag = DEAD_DEAD;
		pPlayer->pev->effects |= EF_NOINTERP;
		pPlayer->m_bIsObserver = TRUE;
		pPlayer->StartObserver( pPlayer->pev->origin, pPlayer->pev->angles );
	}
	else if ( FStrEq( pcmd, "specmode" ) )	// new spectator mode
	{
		if ( pPlayer->IsObserver() )
			pPlayer->Observer_SetMode( atoi( CMD_ARGV(1) ) );
	}
	else if ( FStrEq( pcmd, "follownext" ) )	// follow next player
	{
		if ( pPlayer->IsObserver() )
			pPlayer->Observer_FindNextPlayer( atoi( CMD_ARGV(1) ) != 0 );
	}
	else if ( FStrEq( pcmd, "spectateperson" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;

		int ent = atoi( CMD_ARGV(1) );
		pPlayer->Observer_SpectatePlayer( ent );
	}
	else if ( FStrEq( pcmd, "fullupdate" ) )
	{
		pPlayer->ForceClientDllUpdate();
	}
	else if ( FStrEq( pcmd, "timeleft" ) )
	{
		if ( cvar_timelimit.value != 0 )
		{
			int time_mins, time_secs;
			int flTimeLimit = get_maptime_left();

			time_mins = flTimeLimit / 60;
			time_secs = flTimeLimit % 60;

			char secs[5];
			char *temp = UTIL_dtos2( time_secs );

			if ( time_secs < 10 )
			{
				secs[0] = '0';
				secs[1] = temp[0];
				secs[2] = '\0';
			}
			else
			{
				secs[0] = temp[0];
				secs[1] = temp[1];
				secs[2] = '\0';
			}

			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_time_left1", UTIL_dtos1( time_mins ), secs );
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_time_left2" );
	}
	else if ( FStrEq( pcmd, "dod_version" ) )
	{
		char *ver = "Version 1.2f";
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, ver );
	}
	else if ( FStrEq( pcmd, "chatmsg" ) )
	{
		int temp = pPlayer->m_chatMsg;

		if ( temp )
		{
			temp = 0;
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_chat_off" );
		}
		else
		{
			temp = 1;
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_chat_on" );
		}

		pPlayer->m_chatMsg = temp;
	}
	else if ( FStrEq( pcmd, "deathmsg" ) )
	{
		int temp = pPlayer->m_deathMsg;

		if ( temp )
		{
			temp = 0;
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_deathmsg_off" );
		}
		else
		{
			temp = 1;
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_deathmsg_on" );
		}

		pPlayer->m_deathMsg = temp;
	}
	else if ( FStrEq( pcmd, "clientcutscenes" ) )
	{
		if ( CMD_ARGC() < 2 )
			return FALSE;

		int cmd = atoi( CMD_ARGV(1) );

		if ( !cmd )
		{
			pPlayer->i_seenAllieSpawnScene = 1;
			pPlayer->i_seenAxisSpawnScene = 1;
			pPlayer->i_seenAllieWinScene = 1;
			pPlayer->i_seenAxisWinScene = 1;
			pPlayer->i_seenAllieLooseScene = 1;
			pPlayer->i_seenAxisLooseScene = 1;
			pPlayer->i_seenAllieDrawScene = 1;
			pPlayer->i_seenAxisDrawScene = 1;
		}
	}
	else if ( FStrEq( pcmd, "vote" ) )
	{
		if ( pPlayer->lastMenuSelected == 6 || pPlayer->lastMenuSelected == 7 || pPlayer->lastMenuSelected == 8 ||
			pPlayer->lastMenuSelected == 9 || pPlayer->lastMenuSelected == 5 )
			return TRUE;

		if ( m_vKickSessionState == 1 )
			pPlayer->VoteMenu( m_vKick, 5 );
		else if ( m_vKickSessionState == 0 )
			pPlayer->VoteMenu( -1, 6 );
	}
	else if ( FStrEq( pcmd, "fov" ) )
	{
		pPlayer->pev->fov = pPlayer->m_iFOV;
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_dont_cheat" );
	}
	else if ( FStrEq( pcmd, "say" ) )
	{
		int temp = pPlayer->m_chatMsg;

		const char *readyCvar = CVAR_GET_STRING( "mp_clan_ready_signal" );
		char readyString[64];
		_snprintf( readyString, sizeof( readyString ), "\"%s\"", readyCvar );
		readyString[sizeof( readyString ) - 1] = 0;

		if ( m_bAwaitingReadyRestart && CMD_ARGC() >= 2 )
		{
			char *text = (char *)CMD_ARGS();

			if ( pPlayer->pev->team == 1 && !m_bHeardAlliesReady )
			{
				if ( !strcmp( text, readyString ) )
				{
					m_bHeardAlliesReady = TRUE;
					UTIL_ShowMessageAll( "#Clan_allies_ready", false );
				}
			}
			else if ( pPlayer->pev->team == 2 && !m_bHeardAxisReady && !strcmp( text, readyString ) )
			{
				m_bHeardAxisReady = TRUE;
				UTIL_ShowMessageAll( "#Clan_axis_ready", false );
			}
		}

		if ( !temp )
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_chat_off" );
		else
			Host_Say( pPlayer->edict(), 0 );
	}
	else if ( FStrEq( pcmd, "say_team" ) )
	{
		int temp = pPlayer->m_chatMsg;

		if ( !temp )
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_chat_off" );
		else
			Host_Say( pPlayer->edict(), 1 );
	}
	else if ( FStrEq( pcmd, "menuselect" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;

		if ( pPlayer->lastMenuSelected == 5 )
		{
			if ( atoi( CMD_ARGV(1) ) == 1 )
				VoteKick( pPlayer, m_vKick, true );
		}
		else if ( pPlayer->lastMenuSelected == 6 )
		{
			if ( atoi( CMD_ARGV(1) ) == 9 )
				pPlayer->VoteMenu( -1, 7 );
			else if ( atoi( CMD_ARGV(1) ) != 10 )
				VoteKick( pPlayer, atoi( CMD_ARGV(1) ), true );
		}
		else if ( pPlayer->lastMenuSelected == 7 )
		{
			if ( atoi( CMD_ARGV(1) ) == 9 )
				pPlayer->VoteMenu( -1, 8 );
			else if ( atoi( CMD_ARGV(1) ) != 10 )
				VoteKick( pPlayer, atoi( CMD_ARGV(1) ) + 8, true );
		}
		else if ( pPlayer->lastMenuSelected == 8 )
		{
			if ( atoi( CMD_ARGV(1) ) == 9 )
				pPlayer->VoteMenu( -1, 9 );
			else if ( atoi( CMD_ARGV(1) ) != 10 )
				VoteKick( pPlayer, atoi( CMD_ARGV(1) ) + 16, true );
		}
		else if ( pPlayer->lastMenuSelected == 9 )
		{
			if ( atoi( CMD_ARGV(1) ) != 10 && atoi( CMD_ARGV(1) ) != 9 )
				VoteKick( pPlayer, atoi( CMD_ARGV(1) ) + 24, true );
		}
		else if ( pPlayer->lastMenuSelected > 0 && pPlayer->lastMenuSelected < 5 )
		{
			VoiceCommSelect( pPlayer, atoi( CMD_ARGV(1) ) );
		}

		pPlayer->lastMenuSelected = -1;
		return TRUE;
	}
	else if ( FStrEq( pcmd, "jointeam" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;

		TeamSelect( pPlayer, atoi( CMD_ARGV(1) ) );
		return TRUE;
	}
	else if ( FStrEq( pcmd, "changeclass" ) )
	{
		ShowClassSelectMenu( pPlayer );
		return TRUE;
	}
	else if ( FStrEq( pcmd, "changeteam" ) )
	{
		ShowVGUI( pPlayer, 2, 0 );
		return TRUE;
	}
	else if ( !memcmp( pcmd, "cls_", 4 ) )
	{
		int classid = 0;

		for ( int i = 0; i < NUM_PLAYERCLASSES; i++ )
		{
			if ( FStrEq( g_ClassInfo[i].selectcmd, pcmd ) )
			{
				classid = i;
				break;
			}
		}

		ClassSelect( pPlayer, classid );
	}
	else if ( FStrEq( pcmd, "nextmap" ) )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#game_nextmap", sNextMap );
		return TRUE;
	}
	else if ( FStrEq( pcmd, "debug" ) )
	{
		return TRUE;
	}
	else if ( FStrEq( pcmd, "debug2" ) )
	{
		return TRUE;
	}
	else if ( pPlayer->pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}
	else if ( FStrEq( pcmd, "sprone" ) )
	{
		if ( pPlayer->pev->deadflag != DEAD_NO )
			return TRUE;

		if ( pPlayer->IsSandbagDeployed() || pPlayer->IsInMortarDeploy() )
			return TRUE;

		if ( pPlayer->pev->button & IN_ATTACK )
			return TRUE;

		if ( pPlayer->IsOnLadder() )
			return TRUE;

		if ( !pPlayer->IsProne() && pPlayer->pev->waterlevel > 1 && pPlayer->pev->watertype != CONTENT_FOG )
			return TRUE;

		bool bWentProne = false;

		if ( pPlayer->m_inextprone < gpGlobals->time )
		{
			if ( pPlayer->GetProneState() == 1 )
			{
				pPlayer->UnProne();
			}
			else if ( pPlayer->GetProneState() == 0 )
			{
				if ( ( pPlayer->pev->button & IN_DUCK ) || pPlayer->IsDucking() || ( pPlayer->m_afPhysicsFlags & PFLAG_DUCKING ) )
					return TRUE;

				pPlayer->GoProne();
				bWentProne = true;
			}
		}

		pPlayer->CheckPlayerSpeed();

		if ( bWentProne && !( pPlayer->m_flDisplayHistory & ( 1 << 13 ) ) )
		{
			pPlayer->m_flDisplayHistory |= ( 1 << 13 );
			pPlayer->HintMessage( "#Hint_prone", 0 );
		}
	}
	else if ( FStrEq( pcmd, "give" ) )
	{
		if ( CVAR_GET_FLOAT( "sv_cheats" ) != 0.0 && !pPlayer->pev->iuser1 )
		{
			int times = atoi( CMD_ARGV(2) );
			int iszItem = ALLOC_STRING( CMD_ARGV(1) );	// Make a copy of the classname

			if ( times < 1 )
				times = 1;

			for ( int i = 0; i < times; i++ )
				pPlayer->GiveNamedItem( STRING(iszItem) );
		}
	}
	else if ( FStrEq( pcmd, "use" ) )
	{
		pPlayer->SelectItem( (char *)CMD_ARGV(1) );
	}
	else if ( ( ( pstr = strstr( pcmd, "weapon_" ) ) != NULL ) && ( pstr == pcmd ) )
	{
		pPlayer->SelectItem( pcmd );
	}
	else if ( FStrEq( pcmd, "lastinv" ) )
	{
		pPlayer->SelectLastItem();
	}
	else if ( FStrEq( pcmd, "dropammo" ) || FStrEq( pcmd, "dropmgammo" ) )
	{
		pPlayer->DropGenericAmmo();
	}
	else if ( FStrEq( pcmd, "maim" ) )
	{
		char *q;
		q = (char *)CMD_ARGS();

		if ( CMD_ARGC() < 2 )
			return TRUE;

		char *p = (char *)CMD_ARGV(1);
		byte b;
		b = atoi( p );
	}
	else if ( FStrEq( pcmd, "drop" ) )
	{
		pPlayer->DropPlayerItem( (char *)CMD_ARGV(1), false );
	}
	else if ( FStrEq( pcmd, "signal_sticktogether" ) )
	{
		pPlayer->HandSignal( HS_STICK_TOGETHER );
	}
	else if ( FStrEq( pcmd, "signal_fallback" ) )
	{
		pPlayer->HandSignal( HS_FALL_BACK );
	}
	else if ( FStrEq( pcmd, "signal_no" ) )
	{
		pPlayer->HandSignal( HS_NO_SIR );
	}
	else if ( FStrEq( pcmd, "signal_yes" ) )
	{
		pPlayer->HandSignal( HS_YES_SIR );
	}
	else if ( FStrEq( pcmd, "signal_sniper" ) )
	{
		pPlayer->HandSignal( HS_SNIPER );
	}
	else if ( FStrEq( pcmd, "signal_backup" ) )
	{
		pPlayer->HandSignal( HS_BACKUP );
	}
	else if ( FStrEq( pcmd, "signal_enemyright" ) )
	{
		pPlayer->HandSignal( HS_ENEMY_RIGHT );
	}
	else if ( FStrEq( pcmd, "signal_enemyleft" ) )
	{
		pPlayer->HandSignal( HS_ENEMY_LEFT );
	}
	else if ( FStrEq( pcmd, "signal_grenade" ) )
	{
		pPlayer->HandSignal( HS_GRENADE );
	}
	else if ( FStrEq( pcmd, "signal_flankleft" ) )
	{
		pPlayer->HandSignal( HS_FLANK_LEFT );
	}
	else if ( FStrEq( pcmd, "signal_moveout" ) )
	{
		pPlayer->HandSignal( HS_MOVE_OUT );
	}
	else if ( FStrEq( pcmd, "signal_flankright" ) )
	{
		pPlayer->HandSignal( HS_FLANK_RIGHT );
	}
	else if ( FStrEq( pcmd, "signal_areaclear" ) )
	{
		pPlayer->HandSignal( HS_AREA_CLEAR );
	}
	else if ( FStrEq( pcmd, "signal_coveringfire" ) )
	{
		pPlayer->HandSignal( HS_COVERING_FIRE );
	}
	else if ( FStrEq( pcmd, "signal_holdposition" ) )
	{
		pPlayer->HandSignal( HS_HOLD_POSITION );
	}
	else if ( FStrEq( pcmd, "signal_enemyspotted" ) )
	{
		pPlayer->HandSignal( HS_ENEMY_SPOTTED );
	}
	else if ( pcmd[0] == 'd' && pcmd[1] == 'o' && pcmd[2] == 'd' && pcmd[3] == '_' && pcmd[4] == 'i' && pcmd[5] == 's' &&
		pcmd[6] == '_' && pcmd[7] == 't' && pcmd[8] == 'o' && pcmd[9] == 'n' && pcmd[10] == 'e' && pcmd[11] == 'd' )
	{
		int num = atoi( pcmd + 12 );

		if ( num >= 2 && num <= 5 )
			pPlayer->HandSignal( num + 24 );
	}
	else if ( FStrEq( pcmd, "voice_menu1" ) )
	{
		pPlayer->VoiceMenu( 1 );
	}
	else if ( FStrEq( pcmd, "voice_menu2" ) )
	{
		pPlayer->VoiceMenu( 2 );
	}
	else if ( FStrEq( pcmd, "voice_menu3" ) )
	{
		pPlayer->VoiceMenu( 3 );
	}
	else if ( FStrEq( pcmd, "dropobject" ) )
	{
		if ( pPlayer->HasObject )
			pPlayer->DropObject();

		return TRUE;
	}
	else if ( FStrEq( pcmd, "chattoplayer" ) )
	{
		char text[201];
		int client_Index = atoi( CMD_ARGV(1) );
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( client_Index );

		if ( plr )
		{
			sprintf( text, "%c%s: ", 2, STRING( plr->pev->netname ) );

			for ( int i = 2; i != CMD_ARGC() + 1; i++ )
			{
				char text2[50];
				_snprintf( text2, sizeof( text2 ) - 1, "%s ", CMD_ARGV(i) );
				text2[sizeof( text2 ) - 1] = 0;
				strncat( text, text2, sizeof( text ) - 1 - strlen( text ) );
			}

			ClientPrint( plr->pev, HUD_PRINTTALK, text );
		}

		return TRUE;
	}
	else if ( FStrEq( pcmd, "firemarker" ) )
	{
		pPlayer->FireMapMarker();
	}
	else if ( FStrEq( pcmd, "nextmarker" ) )
	{
		pPlayer->NextMapMarker();
	}
	else if ( FStrEq( pcmd, "prevmarker" ) )
	{
		pPlayer->PrevMapMarker();
	}
	else
	{
		int i = 1;

		while ( i < 28 )
		{
			if ( FStrEq( pcmd, s_VoiceCommands[i][SLOT_VOICECOMMAND] ) )
			{
				pPlayer->VoiceCom( i );
				return TRUE;
			}

			i++;
		}

		// tell the user they entered an unknown command
		char command[128];

		// check the length of the command (prevents crash)
		// max total length is 192 ...and we're adding a string below ("Unknown command: %s\n")
		strncpy( command, pcmd, 127 );
		command[127] = '\0';

		// First parse the name and remove any %'s
		for ( char *pApersand = command; pApersand != NULL && *pApersand != 0; pApersand++ )
		{
			if ( *pApersand == '%' )
				*pApersand = ' ';
		}

		ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "Unknown command: %s\n", command );
	}

	return FALSE;
}
