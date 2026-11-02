#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "objective.h"
#include "../cl_dll/dod_shared.h"

extern int gmsgWeather;
extern int gmsgPShoot;
extern cvar_t *pcvar_mp_clan_scoring;
extern cvar_t *pcvar_mp_clan_scoring_bonus_allies;
extern cvar_t *pcvar_mp_clan_scoring_bonus_axis;
extern void FireDODCamera(const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

LINK_ENTITY_TO_CLASS(dod_object, CObject);
LINK_ENTITY_TO_CLASS(particle_shooter, CParticleShooter);

class CScoreEvent : public CBaseEntity
{
public:
	void Spawn();
	void KeyValue(KeyValueData *pkvd);
private:
	void EXPORT ScoreThink();
	void EXPORT ScoreEventUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
public:
	int m_iMain;
	int m_iResetItemsTime;
	int m_iResetPlayersTime;
	int m_iTeamPoints;
	float ResetItemsTime;
	BOOL ResetItems;
	char szNextMap[256];
	BOOL bChangeLevel;
	float ChangeLevelTime;
	int m_iChangeLevelDelay;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(dod_score_ent, CScoreEvent);

void CObject::Precache()
{
	if (pev->noise)
		PRECACHE_SOUND((char *)STRING(pev->noise));
	if (pev->noise1)
		PRECACHE_SOUND((char *)STRING(pev->noise1));
	if (pev->noise2)
		PRECACHE_SOUND((char *)STRING(pev->noise2));
	if (pev->noise3)
		PRECACHE_SOUND((char *)STRING(pev->noise3));
}

void CObject::Spawn( void )
{
	Precache();

	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_TOSS;

	UTIL_SetOrigin( this, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	m_vStartOrigin.x = 0;
	m_vStartOrigin.y = 0;
	m_vStartOrigin.z = 0;

	pev->sequence = 1;
	pev->framerate = 1;

	m_bDropped = FALSE;
	m_bCarried = FALSE;

	SetTouch( &CObject::ObjectTouch );
	SetThink( &CObject::ObjectThink );

	PRECACHE_MODEL( (char *)STRING( pev->model ) );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	pev->nextthink = gpGlobals->time + 0.1;

	m_iObjectHIcon = 0;

	if ( strlen( m_szObjectHIcon ) > 0 )
		m_iObjectHIcon = PRECACHE_MODEL( m_szObjectHIcon );

	RoundStore();
}

void CObject::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "object_name" ) )
	{
		strncpy( sz_Name, pkvd->szValue, sizeof( sz_Name ) - 1 );
		sz_Name[sizeof( sz_Name ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_owner" ) )
	{
		m_iTeam = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_main" ) )
	{
		m_iMain = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_group" ) )
	{
		strncpy( sz_Group, pkvd->szValue, sizeof( sz_Group ) - 1 );
		sz_Group[sizeof( sz_Group ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_takesound" ) )
	{
		if ( pkvd->szValue )
			pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_capsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise1 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_dropsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise2 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_returnsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise3 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_carryspeed" ) )
	{
		m_fCarrySpeed = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_resetontouch" ) )
	{
		m_iResetOnTouch = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_cappoints" ) )
	{
		m_iCapPoints = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_otherpoints" ) )
	{
		m_iOtherPoints = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_resettime" ) )
	{
		m_iResetTime = atoi( pkvd->szValue );
		if ( m_iResetTime > 600 )
			m_iResetTime = 600;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_donetarget" ) )
	{
		strncpy( sz_DoneTarget, pkvd->szValue, sizeof( sz_DoneTarget ) - 1 );
		sz_DoneTarget[sizeof( sz_DoneTarget ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_player_sprite" ) )
	{
		strncpy( m_szObjectPIcon, pkvd->szValue, sizeof( m_szObjectPIcon ) - 1 );
		m_szObjectPIcon[sizeof( m_szObjectPIcon ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "object_hud_sprite" ) )
	{
		strncpy( m_szObjectHIcon, pkvd->szValue, sizeof( m_szObjectHIcon ) - 1 );
		m_szObjectHIcon[sizeof( m_szObjectHIcon ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CObject::ResetObject( void )
{
	if ( m_vStartOrigin.Length() == 0 )
		m_vStartOrigin = pev->origin;

	Materialize();

	UTIL_SetOrigin( this, m_vStartOrigin );

	m_bCarried = FALSE;
	m_bDropped = FALSE;

	if ( pev->noise3 )
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noise3 ), 1, ATTN_NORM );

	DROP_TO_FLOOR( ENT( pev ) );
}

void CObject::RoundRespawn()
{
	ResetObject();
}

void CObject::AbsentPlayerThink( void )
{
	if ( m_bDropped )
	{
		SetThink( &CObject::ObjectThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		if ( m_bCarried )
		{
			if ( FNullEnt( (CBaseEntity *)m_hOwner ) )
			{
				m_bDropped = FALSE;
				ResetObject();
			}
		}

		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CObject::ObjectThink( void )
{
	if ( m_bDropped && m_iResetTime > 0 && gpGlobals->time >= m_flReset )
		ResetObject();

	pev->nextthink = gpGlobals->time + 0.1;
}

void CObject::ObjectTouch( CBaseEntity *pOther )
{
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;

	if ( pRules && pRules->RoundState() != RND_RUNNING )
		return;

	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;

	if ( m_flNextTouchTime > gpGlobals->time )
		return;

	if ( !pOther->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if ( !pPlayer )
		return;

	if ( pPlayer->HasObject )
		return;

	int realteam = 0;

	if ( m_iTeam == 2 )
		realteam = 1;
	else if ( m_iTeam == 1 )
		realteam = 2;

	if ( pPlayer->pev->team == realteam || realteam == 0 )
	{
		pPlayer->GetObject( this );

		if ( !FStringNull( pev->target ) )
			FireTargets( STRING( pev->target ), pPlayer, this, USE_TOGGLE, 0 );

		m_bCarried = TRUE;
		m_hOwner = pOther;

		Disappear();

		if ( !( pPlayer->m_flDisplayHistory & 0x100 ) )
		{
			pPlayer->m_flDisplayHistory |= 0x100;
			pPlayer->HintMessage( "#Hint_picked_up_object", FALSE );
		}

		if ( pev->noise )
			EMIT_SOUND( ENT( pPlayer->pev ), CHAN_STATIC, STRING( pev->noise ), 1, ATTN_NORM );
	}
	else
	{
		if ( m_bDropped && m_iResetOnTouch )
			ResetObject();
	}
}

void CObject::Drop( void )
{
	Materialize();

	m_flReset = m_iResetTime + gpGlobals->time;
	m_bDropped = TRUE;
}

void CObject::Materialize( void )
{
	if ( UTIL_IsMasterTriggered( m_sMaster, this ) )
	{
		pev->effects &= ~EF_NODRAW;

		SetTouch( &CObject::ObjectTouch );
		SetThink( &CObject::ObjectThink );

		pev->nextthink = gpGlobals->time + 0.1;
		m_flNextTouchTime = gpGlobals->time + 1.0;
	}
}

void CObject::Disappear( void )
{
	pev->effects |= EF_NODRAW;

	SetTouch( NULL );

	if ( m_bCarried )
		SetThink( &CObject::AbsentPlayerThink );
	else
		SetThink( NULL );

	pev->nextthink = gpGlobals->time + 0.1;
	m_bDropped = FALSE;
}

void CObject::SendDroppedMsg(CBasePlayer *pPlayer)
{
	if (pPlayer && (pev->spawnflags & 1))
		UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_lost_object", STRING(pPlayer->pev->netname), sz_Name);
}

void CObject::SendPickupMsg(CBasePlayer *pPlayer)
{
	if (pPlayer && (pev->spawnflags & 1))
		UTIL_ClientPrintAll(HUD_PRINTTALK, "#game_has_object", STRING(pPlayer->pev->netname), sz_Name);
}

void CParticleShooter::Precache()
{
	m_iSpriteIndex = PRECACHE_MODEL((char *)STRING(pev->model));
}

void CParticleShooter::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	RoundStore();
	m_iState = 0;
}

void CParticleShooter::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "group_id" ) )
	{
		m_iGroupId = (byte)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_speed" ) )
	{
		m_flSpeed = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_flVelocity" ) )
	{
		m_flSpeed = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "num_particles" ) )
	{
		m_iNumParticles = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
}

void CParticleShooter::FindTargetEntity( void )
{
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );

	if ( pTarget )
	{
		m_vShootDir = pTarget->pev->origin - pev->origin;
		m_vShootDir = m_vShootDir.Normalize() * m_flSpeed;
	}
	else
		m_vShootDir = pev->velocity;
}

void CParticleShooter::SendUpdate(CBasePlayer *pPlayer)
{
	if (pPlayer)
		MESSAGE_BEGIN(MSG_ONE, gmsgPShoot, NULL, pPlayer->edict());
	else
		MESSAGE_BEGIN(MSG_ALL, gmsgPShoot);
	WRITE_BYTE(m_iGroupId);
	WRITE_BYTE(m_iState);
	if (m_iState == 1)
	{
		WRITE_COORD(m_vShootDir.x);
		WRITE_COORD(m_vShootDir.y);
		WRITE_COORD(m_vShootDir.z);
	}
	MESSAGE_END();
}

void CParticleShooter::UpdateJoiningPlayer(CBasePlayer *pPlayer)
{
	SendUpdate(pPlayer);
}

void CParticleShooter::RoundRespawn()
{
	if (pev->spawnflags & 0x20)
	{
		m_iState = 1;
		FindTargetEntity();
		SendUpdate(NULL);
		if (m_iNumParticles >= 0)
			m_iState = 0;
	}
	else
	{
		m_iState = 0;
		SendUpdate(NULL);
	}
}

void CParticleShooter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	(void)pActivator;
	(void)pCaller;
	(void)value;

	int newState;

	if ( useType == USE_OFF )
		newState = 0;
	else if ( useType == USE_TOGGLE )
		newState = ( m_iState == 0 );
	else
		newState = 1;

	if ( m_iState != newState )
	{
		m_iState = newState;
		FindTargetEntity();
		SendUpdate( NULL );

		if ( m_iNumParticles >= 0 )
			m_iState = 0;
	}
}

void CWeatherSystem::Precache()
{
	PRECACHE_MODEL("sprites/white.spr");
}

void CWeatherSystem::Spawn()
{
	SET_MODEL(edict(), STRING(pev->model));
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;
	SetThink(&CWeatherSystem::InitThink);
	pev->nextthink = gpGlobals->time + 1;
}

void CWeatherSystem::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType |= atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "density" ) )
	{
		m_flDensity = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "lightning" ) )
	{
		int i = atoi( pkvd->szValue );

		if ( i > 0 )
			m_iType |= 4;

		pkvd->fHandled = TRUE;
	}
}

void CWeatherSystem::SendData( void )
{
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4458 )
#endif
	int entindex = ENTINDEX( edict() );
#ifdef _MSC_VER
#pragma warning( pop )
#endif

	WRITE_BYTE( entindex );
	WRITE_BYTE( m_iType );
}

void CWeatherSystem::InitThink()
{
	MESSAGE_BEGIN(MSG_ALL, gmsgWeather);
	SendData();
	MESSAGE_END();
}

void CWeatherSystem::SendForecast(CBasePlayer *pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgWeather, NULL, pPlayer->edict());
	SendData();
	MESSAGE_END();
}

void CScoreEvent::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	UTIL_SetOrigin(this, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	SetTouch(NULL);
	SetThink(&CScoreEvent::ScoreThink);
	SetUse(&CScoreEvent::ScoreEventUse);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CScoreEvent::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "score_resetitemtime" ) )
	{
		m_iResetItemsTime = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "score_points" ) )
	{
		m_iTeamPoints = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "score_resetplayerstime" ) )
	{
		m_iResetPlayersTime = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "score_nextmap" ) )
	{
		strncpy( szNextMap, pkvd->szValue, sizeof( szNextMap ) - 1 );
		szNextMap[sizeof( szNextMap ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "score_nextmapdelay" ) )
	{
		m_iChangeLevelDelay = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CScoreEvent::ScoreThink( void )
{
	if ( ResetItems && gpGlobals->time >= ResetItemsTime )
	{
		CBaseEntity *ent = NULL;
		ent = UTIL_FindEntityByClassname( NULL, "dod_object" );

		while ( ent )
		{
			CObject *obj = (CObject *)ent;

			if ( pev->noise && strcmp( STRING( pev->noise ), obj->sz_Group ) )
				continue;

			obj->Materialize();
			ent = UTIL_FindEntityByClassname( ent, "dod_object" );
		}

		ResetItems = FALSE;
	}

	if ( bChangeLevel && gpGlobals->time >= ChangeLevelTime )
	{
		char buf[269];
		sprintf( buf, "changelevel %s\n", szNextMap );
		SERVER_COMMAND( buf );
		ResetItems = FALSE;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CScoreEvent::ScoreEventUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	(void)pActivator;
	(void)useType;
	(void)value;

	if ( pCaller && pCaller->IsPlayer() )
		return;

	if ( strlen( szNextMap ) > 0 )
	{
		bChangeLevel = TRUE;
		ChangeLevelTime = m_iChangeLevelDelay + gpGlobals->time;
	}

	if ( m_iResetPlayersTime )
	{
		if ( pev->team == 0 )
			g_pGameRules->SetRoundState( RND_DRAW );
		else if ( pev->team == 1 )
			g_pGameRules->SetRoundState( RND_ALLIES_WIN );
		else if ( pev->team == 2 )
			g_pGameRules->SetRoundState( RND_AXIS_WIN );

		g_pGameRules->SetRestartTime( m_iResetPlayersTime + gpGlobals->time );
	}

	if ( m_iResetItemsTime )
	{
		ResetItemsTime = m_iResetItemsTime + gpGlobals->time;
		ResetItems = TRUE;
	}

	if ( pev->message )
		UTIL_ShowMessageAll( STRING( pev->message ), false );

	bool bClanMatch = g_pGameRules->IsClanMatch() && g_pGameRules->IsClanMatchActive();
	bool bGiveRegularPoints = true;

	if ( bClanMatch && pcvar_mp_clan_scoring && pcvar_mp_clan_scoring->value > 0 )
	{
		if ( pev->team == 1 && pcvar_mp_clan_scoring_bonus_allies && pcvar_mp_clan_scoring_bonus_allies->value >= 0 )
		{
			int points = (int)( pcvar_mp_clan_scoring_bonus_allies ? pcvar_mp_clan_scoring_bonus_allies->value : 0 );

			if ( points > 1000 )
				points = 1000;

			g_pGameRules->AddToTeamScores( 1, points );
			bGiveRegularPoints = false;

			UTIL_LogPrintf( "Team \"%s\" won the round. Scores: Allies: %d, Axis: %d\n", "Allies", g_pGameRules->TeamScores( 1 ), g_pGameRules->TeamScores( 2 ) );
		}
		else if ( pev->team == 2 && pcvar_mp_clan_scoring_bonus_axis && pcvar_mp_clan_scoring_bonus_axis->value >= 0 )
		{
			int points = (int)( pcvar_mp_clan_scoring_bonus_axis ? pcvar_mp_clan_scoring_bonus_axis->value : 0 );

			if ( points > 1000 )
				points = 1000;

			g_pGameRules->AddToTeamScores( 2, points );
			bGiveRegularPoints = false;

			UTIL_LogPrintf( "Team \"%s\" won the round. Scores: Allies: %d, Axis: %d\n", "Axis", g_pGameRules->TeamScores( 1 ), g_pGameRules->TeamScores( 2 ) );
		}
	}

	if ( m_iTeamPoints && bGiveRegularPoints )
	{
		if ( pev->team == 1 || pev->team == 0 )
			g_pGameRules->AddToTeamScores( 1, m_iTeamPoints );

		if ( pev->team == 2 || pev->team == 0 )
			g_pGameRules->AddToTeamScores( 2, m_iTeamPoints );
	}

	if ( pev->team == 1 || pev->team == 0 )
	{
		FireDODCamera( "allies_win_cut", NULL, NULL, USE_TOGGLE, 0 );
		FireDODCamera( "axis_loose_cut", NULL, NULL, USE_TOGGLE, 0 );
	}

	if ( pev->team == 2 || pev->team == 0 )
	{
		FireDODCamera( "axis_win_cut", NULL, NULL, USE_TOGGLE, 0 );
		FireDODCamera( "allies_loose_cut", NULL, NULL, USE_TOGGLE, 0 );
	}

	( (CDoDTeamPlay *)g_pGameRules )->ResetForDoDCamera();

	if ( pev->noise2 )
		g_pGameRules->EndMultiplayerGame();
}
