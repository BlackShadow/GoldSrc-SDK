#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"

extern int gmsgResetFade;
extern int gmsgSpecFade;

static const Vector g_vRed( 255, 0, 0 );
static const Vector g_vBlue( 0, 0, 255 );
static const Vector g_vYellow( 255, 255, 0 );
static const Vector g_vGreen( 0, 255, 0 );
static const Vector g_vBlack( 0, 0, 0 );

enum
{
	TELEPORTER_ENTRANCE = 4,
	TELEPORTER_EXIT = 5,
	TELEPORTER_IDLE = 1,
	TELEPORTER_READY = 2,
	TELEPORTER_SENDING = 3,
	TELEPORTER_RECEIVING = 4,
	TELEPORTER_RECEIVED = 5,
	TELEPORTER_RECHARGING = 6
};

void TeleporterResetEffects( CBasePlayer* pPlayer );
void TeleporterResetFade( CBasePlayer* pPlayer );
void TeleporterResetGlowshell( CBasePlayer* pPlayer );
void UpdateEntityEvents( CBaseEntity* pEntity );
void DoDamageEffects( entvars_t* pev, float damageLevelOne, float damageLevelTwo );
void GetWeapon( entvars_t* pevInflictor, entvars_t* pKiller, char* weapon );

LINK_ENTITY_TO_CLASS( building_teleporter, CTFTeleporter );

void CTFTeleporter::Precache()
{
	PRECACHE_MODEL( "models/teleporter.mdl" );
	PRECACHE_SOUND( "misc/teleport_out.wav" );
	PRECACHE_SOUND( "misc/teleport_in.wav" );
	PRECACHE_SOUND( "misc/teleport_ready.wav" );
	PRECACHE_SOUND( "misc/teleport_ready.wav" );
	PRECACHE_MODEL( "sprites/shellchrome.spr" );
	PRECACHE_MODEL( "sprites/particle.spr" );
	m_iShardIndex = PRECACHE_MODEL( "models/computergibs.mdl" );
	m_usBuildingEvent = PRECACHE_EVENT( 1, "events/misc/tf_buildingevent.sc" );
}

void CTFTeleporter::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING( "building_teleporter" );
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_TOSS;
	pev->flags &= ~FL_ONGROUND;
	pev->velocity = Vector( 0, 0, 8 );
	pev->max_health = 125;
	pev->health = pev->max_health;
	pev->takedamage = DAMAGE_NO;
	int topColor = teamcolors[team_no][10].topColor;
	pev->colormap = topColor + (unsigned short)((topColor + 10) << 8);
	pev->flags |= FL_MONSTER;
	SET_MODEL( edict(), "models/teleporter.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 12 ) );
}

CTFTeleporter* CTFTeleporter::CreateTeleporter( Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner, int type )
{
	(void)vecAngles;
	CTFTeleporter* pTeleporter = GetClassPtr( (CTFTeleporter*)NULL );
	UTIL_SetOrigin( pTeleporter->pev, vecOrigin );
	pTeleporter->pev->angles = g_vecZero;
	pTeleporter->pev->angles.y = UTIL_AngleMod( pOwner->pev->angles.y + 180 );
	pTeleporter->real_owner = pOwner;
	pTeleporter->team_no = pOwner->team_no;
	pTeleporter->m_iType = type;
	pTeleporter->Spawn();
	pTeleporter->m_iDestroyed = 0;
	return pTeleporter;
}

void CTFTeleporter::Finished()
{
	const char* pszMessage;
	const char* pszTeamMessage;
	if ( m_iType == TELEPORTER_ENTRANCE )
	{
		real_owner->has_entry_teleporter = 1;
		pszMessage = "#Teleporter_Entrance_Finished";
		pszTeamMessage = "#Teleporter_Entrance_Built";
	}
	else
	{
		real_owner->has_exit_teleporter = 1;
		pszMessage = "#Teleporter_Exit_Finished";
		pszTeamMessage = "#Teleporter_Exit_Built";
	}
	ClientPrint( real_owner->pev, HUD_PRINTNOTIFY, pszMessage );
	teamsprint( team_no, real_owner, HUD_PRINTCENTER, pszTeamMessage, STRING( real_owner->pev->netname ), NULL, NULL );
	const char* pszTeam = real_owner->team_no ? GetTeamName( real_owner->team_no ) : "SPECTATOR";
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"%s\"\n", STRING( real_owner->pev->netname ),
		GETPLAYERUSERID( real_owner->edict() ), GETPLAYERAUTHID( real_owner->edict() ), pszTeam, pszMessage + 1 );
	real_owner->ammo_cells -= 125;
	pev->takedamage = DAMAGE_AIM;
	m_iState = TELEPORTER_IDLE;
	SetTouch( &CTFTeleporter::TeleporterTouch );
	SetThink( &CTFTeleporter::TeleporterThink );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->movetype = MOVETYPE_TOSS;
	pev->velocity = Vector( 0, 0, 8 );
	pev->flags &= ~FL_ONGROUND;
	m_vOldOrigin = pev->origin;
	EMIT_SOUND_DYN( edict(), CHAN_ITEM, "weapons/turrset.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}

void CTFTeleporter::TeleporterTouch( CBaseEntity* pOther )
{
	if ( !(pOther->pev->flags & FL_CLIENT) )
		return;
	if ( pOther->team_no != 0 && pOther->team_no != team_no )
	{
		ClientPrint( real_owner->pev, HUD_PRINTCENTER, m_iType == TELEPORTER_ENTRANCE ?
			"#Teleporter_entrance_enemy_use" : "#Teleporter_exit_enemy_use" );
		return;
	}
	if ( m_iType != TELEPORTER_ENTRANCE )
	{
		if ( m_iState == TELEPORTER_IDLE )
			ClientPrint( pOther->pev, HUD_PRINTCENTER, "#Teleporter_exit_idle" );
		return;
	}
	if ( m_iState == TELEPORTER_IDLE )
	{
		ClientPrint( pOther->pev, HUD_PRINTCENTER, "#Teleporter_entrance_idle" );
		return;
	}
	if ( m_iState != TELEPORTER_READY )
		return;
	if ( pOther->is_unableto_spy_or_teleport )
	{
		CBaseEntity* pGoal = NULL;
		while ( (pGoal = UTIL_FindEntityByClassname( pGoal, "item_tfgoal" )) != NULL )
		{
			if ( pGoal->pev->owner == pOther->edict() && (pGoal->goal_result & 0x10) )
			{
				ClientPrint( pOther->pev, HUD_PRINTCENTER, "#Teleporter_carrying_goal", STRING( pGoal->pev->netname ) );
				break;
			}
		}
		return;
	}
	if ( pOther->pev->velocity.Length() == 0 )
	{
		CTFTeleporter* pMatch = FindMatch();
		if ( pMatch && pMatch->m_iState == TELEPORTER_READY )
			TeleporterSend( (CBasePlayer*)pOther );
	}
}

void CTFTeleporter::TeamFortress_TakeEMPBlast( entvars_t* pevGren )
{
	CBaseEntity* pOther = CBaseEntity::Instance( pevGren->owner );
	ENT( pevGren );
	if ( !((int)gpGlobals->teamplay & 0x1E) || !IsAlly( pOther ) || real_owner == pOther )
		TakeDamage( pevGren, pOther->pev, 200, DMG_BLAST );
}

BOOL CTFTeleporter::EngineerUse( CBasePlayer* pPlayer )
{
	int cells = min( pPlayer->ammo_cells, (int)ceil( (pev->max_health - pev->health) / 5 ) );
	if ( cells <= 0 )
		return FALSE;
	const char* pszMessage = m_iType == TELEPORTER_ENTRANCE ? "#Teleporter_Entrance_Repaired" : "#Teleporter_Exit_Repaired";
	ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, pszMessage );
	pPlayer->ammo_cells -= cells;
	pev->health = min( pev->health + 5 * cells, pev->max_health );
	const char* pszTeam = pPlayer->team_no ? GetTeamName( pPlayer->team_no ) : "SPECTATOR";
	if ( pPlayer->pev == real_owner->pev )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"%s\"\n", STRING( pPlayer->pev->netname ),
			GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ), pszTeam, pszMessage + 1 );
	}
	else
	{
		const char* pszOwnerTeam = real_owner->team_no ? GetTeamName( real_owner->team_no ) : "SPECTATOR";
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"%s\" against \"%s<%i><%s><%s>\"\n",
			STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ),
			pszTeam, pszMessage + 1, STRING( real_owner->pev->netname ), GETPLAYERUSERID( real_owner->edict() ),
			GETPLAYERAUTHID( real_owner->edict() ), pszOwnerTeam );
	}
	CTFTeleporter* pMatch = FindMatch();
	if ( pMatch && m_iState == TELEPORTER_RECHARGING && pMatch->m_iState == TELEPORTER_RECHARGING &&
		pev->health == pev->max_health && pMatch->pev->health == pMatch->pev->max_health &&
		m_flMyNextThink - gpGlobals->time > 10 )
	{
		m_flMyNextThink = gpGlobals->time + 10;
		pMatch->m_flMyNextThink = gpGlobals->time + 10;
	}
	return TRUE;
}

void CTFTeleporter::Teleporter_Explode()
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLODEMODEL );
	WRITE_COORD( pev->origin.x ); WRITE_COORD( pev->origin.y ); WRITE_COORD( pev->origin.z );
	WRITE_COORD( 400 ); WRITE_SHORT( m_iShardIndex ); WRITE_SHORT( 10 ); WRITE_BYTE( 25 );
	MESSAGE_END();
	if ( !real_owner->has_disconnected )
		RadiusDamage( pev, real_owner->pev, 25, CLASS_NONE, DMG_BLAST | 0x04000000 );
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLOSION );
	WRITE_COORD( pev->origin.x ); WRITE_COORD( pev->origin.y ); WRITE_COORD( pev->origin.z );
	WRITE_SHORT( g_sModelIndexFireball ); WRITE_BYTE( 30 ); WRITE_BYTE( 15 ); WRITE_BYTE( 3 );
	MESSAGE_END();
	dremove( this );
}

void CTFTeleporter::Remove()
{
	CBasePlayer* pOwner = (CBasePlayer*)(CBaseEntity*)real_owner;
	const char* pszMessage;
	if ( m_iType == TELEPORTER_ENTRANCE )
	{
		pOwner->has_entry_teleporter = 0;
		pszMessage = "#Teleporter_Entrance_Destroyed";
	}
	else
	{
		pOwner->has_exit_teleporter = 0;
		pszMessage = "#Teleporter_Exit_Destroyed";
	}
	ClientPrint( pOwner->pev, HUD_PRINTCENTER, pszMessage );
	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
		0, 0, 1024, 0, 0, 0 );
	if ( m_iType == TELEPORTER_ENTRANCE )
		pOwner->no_entry_teleporter_message = 0;
	else
		pOwner->no_exit_teleporter_message = 0;
	const char* pszTeam = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
	UTIL_LogPrintf( "\"<-1><><>\" triggered \"%s\" against \"%s<%i><%s><%s>\"\n", pszMessage + 1,
		STRING( pOwner->pev->netname ), GETPLAYERUSERID( pOwner->edict() ), GETPLAYERAUTHID( pOwner->edict() ), pszTeam );
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLOSION );
	WRITE_COORD( pev->origin.x ); WRITE_COORD( pev->origin.y ); WRITE_COORD( pev->origin.z );
	WRITE_SHORT( g_sModelIndexFireball ); WRITE_BYTE( 30 ); WRITE_BYTE( 15 ); WRITE_BYTE( 3 );
	MESSAGE_END();
	dremove( this );
}

void CTFTeleporter::Killed( entvars_t* pevInflictor, entvars_t* pevAttacker, int iGib )
{
	(void)iGib;
	if ( m_iDestroyed )
		return;
	CBasePlayer* pOwner = (CBasePlayer*)(CBaseEntity*)real_owner;
	const char* pszMessage;
	BOOL bPrint;
	if ( m_iType == TELEPORTER_ENTRANCE )
	{
		pOwner->has_entry_teleporter = 0;
		bPrint = !pOwner->no_entry_teleporter_message;
		pOwner->no_entry_teleporter_message = 0;
		pszMessage = "#Teleporter_Entrance_Destroyed";
	}
	else
	{
		pOwner->has_exit_teleporter = 0;
		bPrint = !pOwner->no_exit_teleporter_message;
		pOwner->no_exit_teleporter_message = 0;
		pszMessage = "#Teleporter_Exit_Destroyed";
	}
	if ( bPrint )
		ClientPrint( pOwner->pev, HUD_PRINTCENTER, pszMessage );
	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
		0, 0, 1024, 0, 0, 0 );
	TeleporterKilled();
	SetThink( &CTFTeleporter::Teleporter_Explode );
	pev->nextthink = gpGlobals->time + 0.1;
	if ( pevAttacker && pevInflictor )
	{
		CBasePlayer* pAttacker = (CBasePlayer*)CBaseEntity::Instance( pevAttacker );
		char weaponName[30];
		GetWeapon( pevInflictor, pevAttacker, weaponName );
		const char* pszOwnerTeam = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
		if ( pevAttacker == pevInflictor )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"%s\"\n", STRING( pOwner->pev->netname ),
				GETPLAYERUSERID( pOwner->edict() ), GETPLAYERAUTHID( pOwner->edict() ), pszOwnerTeam, pszMessage + 1 );
		}
		else if ( pevAttacker != pOwner->pev )
		{
			const char* pszTeam = pAttacker->team_no ? GetTeamName( pAttacker->team_no ) : "SPECTATOR";
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"%s\" against \"%s<%i><%s><%s>\" with \"%s\"\n",
				STRING( pevAttacker->netname ), GETPLAYERUSERID( pAttacker->edict() ), GETPLAYERAUTHID( pAttacker->edict() ),
				pszTeam, pszMessage + 1, STRING( pOwner->pev->netname ), GETPLAYERUSERID( pOwner->edict() ),
				GETPLAYERAUTHID( pOwner->edict() ), pszOwnerTeam, weaponName );
		}
	}
	m_iDestroyed = 1;
}

void CTFTeleporter::TeleporterKilled()
{
	CTFTeleporter* pMatch = FindMatch();
	CTFTeleporter* pSource = m_pPlayer ? this : pMatch;
	if ( pSource && pSource->m_pPlayer )
	{
		CBasePlayer* pPlayer = pSource->m_pPlayer;
		pPlayer->tfstate &= ~0x10000;
		pPlayer->TeamFortress_SetSpeed();
		pPlayer->m_iBeingTeleported = 0;
		MESSAGE_BEGIN( MSG_ONE, gmsgResetFade, NULL, pPlayer->edict() );
		MESSAGE_END();
		MESSAGE_BEGIN( MSG_ALL, gmsgSpecFade );
		WRITE_BYTE( pPlayer->entindex() ); WRITE_BYTE( 1 ); WRITE_BYTE( 0 ); WRITE_SHORT( 1 ); WRITE_BYTE( 0 );
		MESSAGE_END();
		TeleporterResetFade( pPlayer );
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, m_iType == TELEPORTER_ENTRANCE ?
			"#Teleporter_Entrance_Gone_Player_Killed" : "#Teleporter_Exit_Gone_Player_Killed" );
		pPlayer->TakeDamage( pSource->pev, pSource->real_owner->pev, 500, DMG_ALWAYSGIB );
	}
	if ( pMatch )
	{
		pMatch->m_pPlayer = NULL;
		pMatch->m_iState = TELEPORTER_IDLE;
		pMatch->m_flMyNextThink = gpGlobals->time + 0.1;
		pMatch->SetTeleporterRings( TELEPORTER_IDLE );
		pMatch->SetTeleporterParticles( pMatch->m_iState );
	}
}

bool CTFTeleporter::PlayerIsStandingOnTeleporter( CBaseEntity* pOther )
{
	(void)pOther;
	return true;
}

float CTFTeleporter::GetDamageMultiplier()
{
	return 1;
}

const Vector& CTFTeleporter::GetTeamColor()
{
	switch ( team_no )
	{
	case 1: return g_vBlue;
	case 2: return g_vRed;
	case 3: return g_vYellow;
	case 4: return g_vGreen;
	default: return g_vBlack;
	}
}

CTFTeleporter* CTFTeleporter::FindMatch()
{
	int iMatchType = m_iType == TELEPORTER_ENTRANCE ? TELEPORTER_EXIT : TELEPORTER_ENTRANCE;
	CTFTeleporter* pMatch = NULL;
	while ( (pMatch = (CTFTeleporter*)UTIL_FindEntityByClassname( pMatch, "building_teleporter" )) != NULL )
	{
		if ( pMatch->real_owner->pev == real_owner->pev && pMatch->m_iType == iMatchType )
			return pMatch;
	}
	return NULL;
}

BOOL CTFTeleporter::TeleportersReady()
{
	CTFTeleporter* pMatch = FindMatch();
	return pMatch && pMatch->m_iState != 0;
}

void CTFTeleporter::TeleporterSend( CBasePlayer* pPlayer )
{
	if ( pPlayer )
	{
		m_pPlayer = pPlayer;
		m_iState = TELEPORTER_SENDING;
		m_flMyNextThink = gpGlobals->time + 0.1;
	}
}

void CTFTeleporter::TeleporterReceive( CBasePlayer* pPlayer, float flDelay )
{
	if ( pPlayer )
	{
		m_pPlayer = pPlayer;
		m_iState = TELEPORTER_RECEIVING;
		m_flDamageDelay = flDelay;
		m_flMyNextThink = gpGlobals->time + flDelay * 0.25;
	}
}

void CTFTeleporter::SetTeleporterRings( int state )
{
	int iEvent = m_iType == TELEPORTER_ENTRANCE ? 4 : 8;
	BOOL bActive = state >= TELEPORTER_READY && state <= TELEPORTER_RECHARGING;
	if ( bActive == ((m_iBuildingEventState & iEvent) != 0) )
		return;
	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
		0, 0, iEvent, team_no, bActive, 0 );
	if ( bActive )
		m_iBuildingEventState |= iEvent;
	else
		m_iBuildingEventState &= ~iEvent;
}

void CTFTeleporter::SetTeleporterParticles( int state )
{
	if ( state == TELEPORTER_READY )
	{
		if ( m_iType == TELEPORTER_ENTRANCE && !(m_iBuildingEventState & 512) )
		{
			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
				0, 0, 512, team_no, 1, 0 );
			m_iBuildingEventState |= 512;
			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
				0, 0, 32, team_no, 1, 0 );
		}
	}
	else if ( m_iBuildingEventState & 512 )
	{
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
			0, 0, 512, team_no, 0, 0 );
		m_iBuildingEventState &= ~512;
	}
}

void CTFTeleporter::TeleporterFadePlayer( int direction )
{
	if ( m_pPlayer )
	{
		m_pPlayer->pev->rendermode = kRenderTransAlpha;
		m_pPlayer->pev->renderamt = direction ? 0 : 255;
		m_pPlayer->m_iFadeDirection = direction ? 1 : 0;
		m_pPlayer->m_flFadeAmount = 255 / (0.25 * m_flDamageDelay / 0.05);
	}
}

void CTFTeleporter::TeleporterProcessFade()
{
	if ( !m_pPlayer || m_pPlayer->pev->rendermode == kRenderNormal )
		return;
	if ( m_pPlayer->m_iFadeDirection == 1 && m_iType == TELEPORTER_ENTRANCE )
		m_pPlayer->pev->renderamt = min( m_pPlayer->pev->renderamt + m_pPlayer->m_flFadeAmount, 255.0f );
	else if ( m_pPlayer->m_iFadeDirection == 0 && m_iType == TELEPORTER_EXIT )
		m_pPlayer->pev->renderamt = max( m_pPlayer->pev->renderamt - m_pPlayer->m_flFadeAmount, 0.0f );
}

void CTFTeleporter::TeleporterThink()
{
	CheckBelowBuilding( 8 );
	UpdateEntityEvents( this );
	DoDamageEffects( pev, pev->max_health * 0.66, pev->max_health * 0.33 );
	TeleporterProcessFade();
	pev->nextthink = gpGlobals->time + 0.05;
	if ( m_flMyNextThink != 0 && m_flMyNextThink > gpGlobals->time )
		return;

	CTFTeleporter* pMatch;
	switch ( m_iState )
	{
	case TELEPORTER_READY:
		if ( !TeleportersReady() )
			m_iState = TELEPORTER_IDLE;
		m_flMyNextThink = gpGlobals->time + 0.1;
		break;
	case TELEPORTER_SENDING:
		m_pPlayer->tfstate |= 0x10000;
		m_pPlayer->TeamFortress_SetSpeed();
		m_pPlayer->m_iBeingTeleported = 1;
		pMatch = FindMatch();
		m_flDamageDelay = 1;
		if ( pMatch && pMatch->m_iType == TELEPORTER_EXIT && pMatch->m_iState == TELEPORTER_READY )
		{
			TeleporterResetEffects( m_pPlayer );
			UTIL_ScreenFade( m_pPlayer->edict(), GetTeamColor(), 0.25 * m_flDamageDelay,
				0.25 * m_flDamageDelay, 255, 1 );
			TeleporterFadePlayer( 0 );
			MESSAGE_BEGIN( MSG_ALL, gmsgSpecFade );
			WRITE_BYTE( m_pPlayer->entindex() ); WRITE_BYTE( 0 ); WRITE_BYTE( team_no );
			WRITE_SHORT( (int)(0.25 * m_flDamageDelay * 100) ); WRITE_BYTE( 255 );
			MESSAGE_END();
			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
				0, 0, 128, team_no, 1, 0 );
			pMatch->TeleporterReceive( m_pPlayer, m_flDamageDelay );
			m_iState = TELEPORTER_RECHARGING;
			m_flMyNextThink = gpGlobals->time + m_flDamageDelay * 10.5;
		}
		else
		{
			m_pPlayer->tfstate &= ~0x10000;
			m_pPlayer->TeamFortress_SetSpeed();
			m_pPlayer->m_iBeingTeleported = 0;
			TeleporterResetFade( m_pPlayer );
			m_iState = TELEPORTER_RECHARGING;
			m_flMyNextThink = gpGlobals->time + m_flDamageDelay * 10;
			SetTeleporterRings( TELEPORTER_RECHARGING );
			SetTeleporterParticles( m_iState );
		}
		break;
	case TELEPORTER_RECEIVING:
		{
			Vector newPosition = pev->origin + Vector( 0, 0, 49 );
			CTelefragDeath* pDeath = GetClassPtr( (CTelefragDeath*)NULL );
			UTIL_SetOrigin( pDeath->pev, newPosition );
			pDeath->pev->owner = m_pPlayer->edict();
			pDeath->Spawn( this );
			UTIL_SetOrigin( m_pPlayer->pev, newPosition );
			UTIL_ScreenFade( m_pPlayer->edict(), GetTeamColor(), 0.25 * m_flDamageDelay, 0, 255, 0 );
			TeleporterFadePlayer( 1 );
			MESSAGE_BEGIN( MSG_ALL, gmsgSpecFade );
			WRITE_BYTE( m_pPlayer->entindex() ); WRITE_BYTE( 1 ); WRITE_BYTE( team_no );
			WRITE_SHORT( (int)(0.25 * m_flDamageDelay * 100) ); WRITE_BYTE( 255 );
			MESSAGE_END();
			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
				0, 0, 64, team_no, 1, 0 );
			m_iState = TELEPORTER_RECEIVED;
			m_flMyNextThink = gpGlobals->time + 0.25 * m_flDamageDelay;
		}
		break;
	case TELEPORTER_RECEIVED:
		m_pPlayer->tfstate &= ~0x10000;
		m_pPlayer->TeamFortress_SetSpeed();
		m_pPlayer->m_iBeingTeleported = 0;
		TeleporterResetFade( m_pPlayer );
		m_iState = TELEPORTER_RECHARGING;
		m_flMyNextThink = gpGlobals->time + 10 * m_flDamageDelay;
		m_pPlayer->pev->renderfx = kRenderFxGlowShell;
		m_pPlayer->pev->rendercolor = GetTeamColor();
		m_pPlayer->pev->renderamt = 15;
		m_pPlayer->m_flTeleporterEffectEndTime = gpGlobals->time + 12.0;
		m_pPlayer = NULL;
		pMatch = FindMatch();
		if ( pMatch )
			pMatch->m_pPlayer = NULL;
		break;
	case TELEPORTER_RECHARGING:
		m_iState = TELEPORTER_READY;
		m_flMyNextThink = gpGlobals->time + 0.1;
		break;
	default:
		if ( TeleportersReady() )
			m_iState = TELEPORTER_READY;
		m_flMyNextThink = gpGlobals->time + 0.1;
		break;
	}
	SetTeleporterRings( m_iState );
	SetTeleporterParticles( m_iState );
}

void PlayerStoppedTeleporting( CBasePlayer* pPlayer )
{
	if ( !pPlayer )
		return;
	CTFTeleporter* pTeleporter = NULL;
	while ( (pTeleporter = (CTFTeleporter*)UTIL_FindEntityByClassname( pTeleporter, "building_teleporter" )) != NULL )
	{
		if ( pTeleporter->m_pPlayer == pPlayer )
		{
			pTeleporter->m_iState = TELEPORTER_RECHARGING;
			pTeleporter->m_flMyNextThink = gpGlobals->time + 10 * pTeleporter->m_flDamageDelay;
			pTeleporter->SetTeleporterRings( TELEPORTER_RECHARGING );
			pTeleporter->SetTeleporterParticles( pTeleporter->m_iState );
		}
	}
	pPlayer->m_iBeingTeleported = 0;
	pPlayer->tfstate &= ~0x10000;
	MESSAGE_BEGIN( MSG_ONE, gmsgResetFade, NULL, pPlayer->edict() );
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_ALL, gmsgSpecFade );
	WRITE_BYTE( pPlayer->entindex() ); WRITE_BYTE( 1 ); WRITE_BYTE( 0 ); WRITE_SHORT( 1 ); WRITE_BYTE( 0 );
	MESSAGE_END();
	TeleporterResetFade( pPlayer );
}

void DoDamageEffects( entvars_t* pev, float damageLevelOne, float damageLevelTwo )
{
	CBaseEntity* pEntity = CBaseEntity::Instance( pev );
	if ( pev->health < damageLevelOne )
	{
		if ( !(pEntity->m_iBuildingEventState & 1) )
		{
			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, pEntity->edict(), pEntity->m_usBuildingEvent, 0,
				pev->origin, (float*)&g_vecZero, 0, 0, 1, pEntity->team_no, 1, 0 );
			pEntity->m_iBuildingEventState |= 1;
		}
	}
	else if ( pEntity->m_iBuildingEventState & 1 )
	{
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, pEntity->edict(), pEntity->m_usBuildingEvent, 0,
			pev->origin, (float*)&g_vecZero, 0, 0, 1, 0, 0, 0 );
		pEntity->m_iBuildingEventState &= ~1;
	}
	if ( pev->health < damageLevelTwo )
	{
		if ( !(pEntity->m_iBuildingEventState & 2) )
		{
			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, pEntity->edict(), pEntity->m_usBuildingEvent, 0,
				pev->origin, (float*)&g_vecZero, 0, 0, 2, pEntity->team_no, 1, 0 );
			pEntity->m_iBuildingEventState |= 2;
		}
	}
	else if ( pEntity->m_iBuildingEventState & 2 )
	{
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, pEntity->edict(), pEntity->m_usBuildingEvent, 0,
			pev->origin, (float*)&g_vecZero, 0, 0, 2, 0, 0, 0 );
		pEntity->m_iBuildingEventState &= ~2;
	}
}

void UpdateEntityEvents( CBaseEntity* pEntity )
{
	if ( pEntity && pEntity->pev->origin != pEntity->m_vOldOrigin )
	{
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, pEntity->edict(), pEntity->m_usBuildingEvent, 0,
			pEntity->pev->origin, (float*)&g_vecZero, 0, 0, 256, 0, 0, 0 );
		pEntity->m_vOldOrigin = pEntity->pev->origin;
	}
}

void SendBuildingEventInfo( CBasePlayer* pPlayer )
{
	const char* pszClasses[] = { "building_dispenser", "building_sentrygun", "building_teleporter" };
	for ( int i = 0; i < ARRAYSIZE( pszClasses ); ++i )
	{
		CBaseEntity* pEntity = NULL;
		while ( (pEntity = UTIL_FindEntityByClassname( pEntity, pszClasses[i] )) != NULL )
		{
			if ( pEntity->m_iBuildingEventState )
			{
				PLAYBACK_EVENT_FULL( FEV_HOSTONLY | FEV_RELIABLE, pPlayer->edict(), pEntity->m_usBuildingEvent, 0,
					pEntity->pev->origin, (float*)&g_vecZero, (float)pEntity->entindex(), 0,
					pEntity->m_iBuildingEventState | 2048, pEntity->team_no, 1, 0 );
			}
		}
	}
}

void TeleporterResetFade( CBasePlayer* pPlayer )
{
	if ( pPlayer )
	{
		pPlayer->pev->rendermode = kRenderNormal;
		pPlayer->pev->renderamt = 0;
		pPlayer->m_flFadeAmount = 0;
		pPlayer->m_iFadeDirection = 0;
	}
}

void TeleporterResetGlowshell( CBasePlayer* pPlayer )
{
	if ( pPlayer )
	{
		pPlayer->pev->renderfx = kRenderFxNone;
		pPlayer->pev->rendercolor = g_vecZero;
		pPlayer->pev->renderamt = 0;
		pPlayer->m_flTeleporterEffectEndTime = 0;
	}
}

void TeleporterResetEffects( CBasePlayer* pPlayer )
{
	TeleporterResetFade( pPlayer );
	TeleporterResetGlowshell( pPlayer );
}
