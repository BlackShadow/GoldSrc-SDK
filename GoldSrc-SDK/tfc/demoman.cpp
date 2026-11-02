#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "effects.h"
#include "tf_defs.h"

LINK_ENTITY_TO_CLASS( detpack, CDetpack );

void CDetpack::TeamFortress_TakeEMPBlast( entvars_t* )
{
	float time = gpGlobals->time + 1;
	pev->health = RANDOM_FLOAT( 0, 2 ) + time;
}

void CDetpack::Precache()
{
	PRECACHE_MODEL( "models/presentlg.mdl" );
	PRECACHE_MODEL( "models/detpack.mdl" );
	PRECACHE_MODEL( "sprites/flare3.spr" );
	PRECACHE_SOUND( "weapons/mortarhit.wav" );
	PRECACHE_SOUND( "weapons/mine_activate.wav" );
	PRECACHE_SOUND( "weapons/mine_charge.wav" );
}

void CDetpack::Spawn()
{
	Precache();
	bAlternateLight = FALSE;
	bTicked = FALSE;
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	m_pBlinkSprite = NULL;
	SET_MODEL( edict(), birthday ? "models/presentlg.mdl" : "models/detpack.mdl" );
	SetTouch( &CDetpack::DetpackTouch );
	SetThink( &CDetpack::DetpackThink );
	pev->nextthink = gpGlobals->time + 0.1;
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );
	UTIL_SetOrigin( pev, pev->origin );
	weaponmode = 0;
	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/mine_activate.wav", 1, ATTN_NORM );
}

void CDetpack::DetpackTouch( CBaseEntity* pOther )
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOther->Classify() != CLASS_PLAYER || pOther->pev->playerclass != PC_SCOUT ||
		!pOther->IsAlive() || pOwner->IsAlly( pOther ) )
		return;
	if ( birthday )
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Detpack_disarm_bday", STRING( pOther->pev->netname ), STRING( pOwner->pev->netname ) );
	else
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Detpack_disarm", STRING( pOwner->pev->netname ), STRING( pOther->pev->netname ) );
	const char* ownerTeam = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
	const char* ownerAuth = GETPLAYERAUTHID( pOwner->edict() );
	int ownerID = GETPLAYERUSERID( pOwner->edict() );
	const char* ownerName = STRING( pOwner->pev->netname );
	const char* otherTeam = pOther->team_no ? GetTeamName( pOther->team_no ) : "SPECTATOR";
	const char* otherAuth = GETPLAYERAUTHID( pOther->edict() );
	int otherID = GETPLAYERUSERID( pOther->edict() );
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Detpack_Disarmed\" against \"%s<%i><%s><%s>\"\n",
		STRING( pOther->pev->netname ), otherID, otherAuth, otherTeam, ownerName, ownerID, ownerAuth, ownerTeam );
	( (CBasePlayer*)pOther )->TF_AddFrags( 1 );
	dremove( this );
}

void CDetpack::Remove()
{
	CBasePlayer* pOwner = (CBasePlayer*)CBaseEntity::Instance( pev->owner );
	pOwner->ammo_detpack++;
	ClientPrint( pOwner->pev, HUD_PRINTCENTER, "#Detpack_blocking_door_destroyed" );
	const char* team = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
	const char* auth = GETPLAYERAUTHID( pOwner->edict() );
	int id = GETPLAYERUSERID( pOwner->edict() );
	UTIL_LogPrintf( "\"<-1><><>\" triggered \"Detpack_blocking_door_destroyed\" against \"%s<%i><%s><%s>\"\n",
		STRING( pOwner->pev->netname ), id, auth, team );
	if ( m_pBlinkSprite )
		dremove( m_pBlinkSprite );
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 30 );
		WRITE_BYTE( 15 );
		WRITE_BYTE( TE_EXPLFLAG_NOADDITIVE | TE_EXPLFLAG_NODLIGHTS );
	MESSAGE_END();
	dremove( this );
}

void CDetpack::DetpackExplode()
{
	CBasePlayer* pOwner = (CBasePlayer*)CBaseEntity::Instance( pev->owner );
	int contents = UTIL_PointContents( pev->origin );
	if ( contents != CONTENTS_SKY && contents != CONTENTS_SOLID && !pOwner->has_disconnected )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#FITH" );
		EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/mortarhit.wav", 1, ATTN_NORM );
		if ( birthday )
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#SpreadsCheer", STRING( pOwner->pev->netname ) );
		const char* team = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
		const char* auth = GETPLAYERAUTHID( pOwner->edict() );
		int id = GETPLAYERUSERID( pOwner->edict() );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Detpack_Explode\"\n", STRING( pOwner->pev->netname ), id, auth, team );
		Vector center = pev->origin + Vector( 0, 0, 16 );
		CBaseEntity* pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, center, WEAP_DETPACK_GOAL_SIZE ) ) != NULL )
		{
			Vector target = pEntity->Center() + pEntity->pev->view_ofs;
			TraceResult trace;
			UTIL_TraceLine( center, target, dont_ignore_monsters, edict(), &trace );
			if ( trace.flFraction < 1 && trace.pHit != pEntity->edict() )
				continue;
			if ( FClassnameIs( pEntity->pev, "info_tfgoal" ) )
			{
				if ( ( pEntity->goal_activation & TFGA_TOUCH_DETPACK ) && pEntity->search_time == 0 )
					ActivateDoResults( pEntity, pOwner, pEntity );
			}
			else if ( pEntity->pev->takedamage != DAMAGE_NO &&
				( pEntity->pev->origin - pev->origin ).Length() <= WEAP_DETPACK_SIZE )
			{
				Vector midpoint = pEntity->pev->origin + ( pEntity->pev->mins + pEntity->pev->maxs ) * 0.5f;
				float falloff = ( pev->origin - midpoint ).Length() * 0.5;
				float damage = falloff >= 0 ? WEAP_DETPACK_SIZE - falloff : WEAP_DETPACK_SIZE;
				if ( damage != 0 )
					pEntity->TakeDamage( pev, pOwner->pev, damage + damage, DMG_BLAST );
			}
		}
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 50 );
			WRITE_BYTE( 15 );
			WRITE_BYTE( TE_EXPLFLAG_NOADDITIVE | TE_EXPLFLAG_NODLIGHTS );
		MESSAGE_END();
	}
	else
	{
		ClientPrint( pOwner->pev, HUD_PRINTNOTIFY, "#Detpack_fizzle" );
		const char* team = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
		const char* auth = GETPLAYERAUTHID( pOwner->edict() );
		int id = GETPLAYERUSERID( pOwner->edict() );
		UTIL_LogPrintf( "\"<-1><><>\" triggered \"Detpack_Fizzle\" against \"%s<%i><%s><%s>\"\n", STRING( pOwner->pev->netname ), id, auth, team );
	}
	if ( weaponmode == 1 )
	{
		CBasePlayer* pDisarmer = (CBasePlayer*)CBaseEntity::Instance( pev->enemy );
		pDisarmer->tfstate &= ~TFSTATE_CANT_MOVE;
		pDisarmer->TeamFortress_SetSpeed();
		CBaseEntity* pTimer = pDisarmer->FindTimer( TF_TIMER_DETPACKDISARM );
		if ( pTimer )
			dremove( pTimer );
	}
	if ( m_pBlinkSprite )
		dremove( m_pBlinkSprite );
	dremove( this );
}

void CDetpack::DetpackThink()
{
	if ( pev->health != 0 && !( gpGlobals->time + 0.9 < pev->health ) )
	{
		DetpackExplode();
		return;
	}
	CheckBelowBuilding( 8 );
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	int seconds = (int)( pev->health - gpGlobals->time );
	pev->nextthink = gpGlobals->time + 1;
	if ( seconds > 10 )
		return;
	ClientPrint( pOwner->pev, HUD_PRINTNOTIFY, "#Detpack_countdown", UTIL_dtos1( seconds ) );
	if ( seconds > 5 )
		return;
	if ( !bTicked )
	{
		EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/mine_charge.wav", 1, ATTN_NORM );
		bTicked = TRUE;
		m_pBlinkSprite = CSprite::SpriteCreate( "sprites/flare3.spr", pev->origin + Vector( 0, 0, 64 ), FALSE );
		m_pBlinkSprite->pev->rendermode = kRenderGlow;
		m_pBlinkSprite->pev->rendercolor = Vector( 255, 0, 0 );
		m_pBlinkSprite->pev->renderamt = 0;
		m_pBlinkSprite->pev->renderfx = kRenderFxNoDissipation;
	}
	else if ( seconds != 5 )
	{
		bAlternateLight = !bAlternateLight;
		m_pBlinkSprite->pev->renderamt = bAlternateLight ? 64.0f : 0.0f;
	}
}

void CBasePlayer::TeamFortress_SetDetpack( int iTimer )
{
	if ( !( weapons_carried & WEAP_DETPACK ) || ammo_detpack <= 0 )
		return;
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 65 ) ) != NULL )
	{
		const char* message = NULL;
		if ( FClassnameIs( pEntity->pev, "player" ) && pEntity != this && !( pEntity->pev->effects & EF_NODRAW ) )
			message = "#Detpack_someone";
		else if ( FClassnameIs( pEntity->pev, "building_sentrygun" ) )
			message = "#Detpack_sentry";
		else if ( FClassnameIs( pEntity->pev, "building_dispenser" ) )
			message = "#Detpack_dispenser";
		else if ( FClassnameIs( pEntity->pev, "building_teleporter" ) )
			message = "#Detpack_teleporter";
		else if ( FClassnameIs( pEntity->pev, "detpack" ) )
			message = "#Detpack_stack";
		if ( message )
		{
			ClientPrint( pev, HUD_PRINTNOTIFY, message );
			return;
		}
	}
	if ( !( pev->flags & FL_ONGROUND ) )
	{
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Detpack_air" );
		return;
	}
	pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, "detpack" ) ) != NULL )
	{
		if ( pEntity->pev->owner == edict() )
		{
			ClientPrint( pev, HUD_PRINTNOTIFY, "#Detpack_oneactive" );
			return;
		}
	}
	if ( iTimer <= 4 )
	{
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Detpack_fivesec" );
		return;
	}
	ammo_detpack--;
	is_detpacking = TRUE;
	tfstate |= TFSTATE_CANT_MOVE;
	immune_to_check = gpGlobals->time + 10;
	TeamFortress_SetSpeed();
	m_pActiveItem->Holster();
	pev->weaponmodel = 0;
	ClientPrint( pev, HUD_PRINTNOTIFY, "#Detpack_set", UTIL_dtos1( iTimer ) );
	CBaseEntity* pTimer = CreateTimer( TF_TIMER_DETPACKSET );
	pTimer->pev->nextthink = gpGlobals->time + WEAP_DETPACK_SETTIME;
	pTimer->SetThink( &CBaseEntity::Timer_DetpackSet );
	pTimer->pev->health = (float)iTimer;
}

void CBasePlayer::TeamFortress_DetpackStop()
{
	CBaseEntity* pTimer = FindTimer( TF_TIMER_DETPACKSET );
	if ( !pTimer )
		return;
	ClientPrint( pev, HUD_PRINTNOTIFY, "#Detpack_retrieve" );
	ammo_detpack++;
	dremove( pTimer );
	tfstate &= ~TFSTATE_CANT_MOVE;
	is_detpacking = FALSE;
	TeamFortress_SetSpeed();
	m_pActiveItem->Deploy();
}

BOOL CBasePlayer::TeamFortress_RemoveDetpacks()
{
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, "detpack" ) ) != NULL )
	{
		if ( pEntity->pev->owner == edict() )
		{
			dremove( pEntity );
			return TRUE;
		}
	}
	return FALSE;
}
