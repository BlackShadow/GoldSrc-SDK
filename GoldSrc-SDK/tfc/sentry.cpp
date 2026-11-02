#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"

extern int gmsgDeathMsg;
void GetWeapon( entvars_t* pevInflictor, entvars_t* pKiller, char* weapon );
void UpdateEntityEvents( CBaseEntity* pEntity );
void DoDamageEffects( entvars_t* pev, float damageLevelOne, float damageLevelTwo );
void UTIL_ParametricRocket( entvars_t* pev, Vector vecOrigin, Vector vecAngles, edict_t* pOwner );

LINK_ENTITY_TO_CLASS( building_sentrygun, CTFSentrygun );
LINK_ENTITY_TO_CLASS( building_sentrygun_base, CTFSentrygunBase );

static void ClearSentryMenus( CBaseEntity* pSentry )
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->building == pSentry )
		{
			pPlayer->ResetMenu();
			pPlayer->building = NULL;
		}
	}
}

static void LogSentryUse( CBasePlayer* pPlayer, CBaseEntity* pOwner, char* pszOwnFormat, char* pszAgainstFormat )
{
	if ( pPlayer->pev == pOwner->pev )
	{
		UTIL_LogPrintf( pszOwnFormat, STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ),
			GETPLAYERAUTHID( pPlayer->edict() ), pPlayer->team_no ? GetTeamName( pPlayer->team_no ) : "SPECTATOR" );
	}
	else
	{
		const char* pszOwnerTeam = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
		const char* pszTeam = pPlayer->team_no ? GetTeamName( pPlayer->team_no ) : "SPECTATOR";
		UTIL_LogPrintf( pszAgainstFormat,
			STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ),
			pszTeam, STRING( pOwner->pev->netname ), GETPLAYERUSERID( pOwner->edict() ),
			GETPLAYERAUTHID( pOwner->edict() ), pszOwnerTeam );
	}
}

void CTFSentrygun::Precache()
{
	PRECACHE_MODEL( "models/sentry1.mdl" );
	PRECACHE_MODEL( "models/sentry2.mdl" );
	PRECACHE_MODEL( "models/sentry3.mdl" );
	PRECACHE_MODEL( "models/base.mdl" );
	PRECACHE_MODEL( "sprites/shellchrome.spr" );
	PRECACHE_SOUND( "weapons/turridle.wav" );
	PRECACHE_SOUND( "weapons/turrspot.wav" );
	PRECACHE_SOUND( "weapons/turrset.wav" );
	PRECACHE_SOUND( "weapons/rocketfire1.wav" );
	PRECACHE_SOUND( "weapons/pl_gun3.wav" );
	m_usBuildingEvent = PRECACHE_EVENT( 1, "events/misc/tf_buildingevent.sc" );
}

void CTFSentrygunBase::Spawn()
{
	pev->classname = MAKE_STRING( "building_sentrygun_base" );
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	pev->flags &= ~FL_ONGROUND;
	pev->velocity = Vector( 0, 0, 8 );
	pev->flags |= FL_MONSTER;
	SET_MODEL( edict(), "models/base.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );
}

CTFSentrygunBase* CTFSentrygunBase::CreateSentryBase( Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner )
{
	(void)vecAngles;
	CTFSentrygunBase* pBase = GetClassPtr( (CTFSentrygunBase*)NULL );
	UTIL_SetOrigin( pBase->pev, vecOrigin );
	pBase->pev->angles = g_vecZero;
	pBase->pev->angles.y = UTIL_AngleMod( pOwner->pev->angles.y + 180 );
	pBase->Spawn();
	pBase->real_owner = pOwner;
	pBase->team_no = pOwner->team_no;
	return pBase;
}

void CTFSentrygunBase::Finished()
{
	real_owner->has_sentry = 1;
	ClientPrint( real_owner->pev, HUD_PRINTNOTIFY, "#Sentry_finish" );
	teamsprint( team_no, real_owner, HUD_PRINTNOTIFY, "#Sentry_built", STRING( real_owner->pev->netname ), NULL, NULL );
	const char* pszTeam = real_owner->team_no ? GetTeamName( real_owner->team_no ) : "SPECTATOR";
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Sentry_Built_Level_1\"\n", STRING( real_owner->pev->netname ),
		GETPLAYERUSERID( real_owner->edict() ), GETPLAYERAUTHID( real_owner->edict() ), pszTeam );
	pev->takedamage = DAMAGE_AIM;
	real_owner->ammo_cells -= 130;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 4 ) );
	CTFSentrygun* pSentry = GetClassPtr( (CTFSentrygun*)NULL );
	pSentry->Spawn();
	pSentry->real_owner = real_owner;
	pSentry->team_no = team_no;
	int topColor = teamcolors[team_no][10].topColor;
	pSentry->pev->colormap = topColor + (unsigned short)((topColor + 10) << 8);
	pSentry->m_pOtherSection = this;
	m_pOtherSection = pSentry;
	UTIL_SetOrigin( pSentry->pev, pev->origin + Vector( 0, 0, 21.2f ) );
	pSentry->pev->angles.y = pev->angles.y;
	pSentry->m_vecCurAngles.y = UTIL_AngleMod( pSentry->pev->angles.y );
	pSentry->m_iRightBound = (int)UTIL_AngleMod( (float)((int)pSentry->pev->angles.y - 50) );
	pSentry->m_iLeftBound = (int)UTIL_AngleMod( (float)((int)pSentry->pev->angles.y + 50) );
	if ( pSentry->m_iLeftBound < pSentry->m_iRightBound )
	{
		pSentry->m_iRightBound = pSentry->m_iLeftBound;
		pSentry->m_iLeftBound = (int)UTIL_AngleMod( (float)((int)pSentry->pev->angles.y - 50) );
	}
	pSentry->m_bTurningRight = TRUE;
	pSentry->m_vecCurAngles.x = 0;
	pSentry->m_vecGoalAngles.x = 0;
	pSentry->m_vecGoalAngles.y = (float)pSentry->m_iRightBound;
	pSentry->m_vOldOrigin = pSentry->pev->origin;
	EMIT_SOUND_DYN( pSentry->edict(), CHAN_ITEM, "weapons/turrset.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	pSentry->m_flNextCheck = gpGlobals->time + 0.1;
}

int CTFSentrygunBase::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if ( !m_pOtherSection || (bitsDamageType & (DMG_BLAST | 0x06000000)) )
		return 0;
	return m_pOtherSection->TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CTFSentrygun::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING( "building_sentrygun" );
	pev->health = 150;
	pev->max_health = pev->health;
	pev->movetype = MOVETYPE_FLY;
	pev->sequence = TFTURRET_ANIM_NONE;
	pev->frame = 0;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_AIM;
	pev->view_ofs = Vector( 0, 0, 22 );
	weapon = 1;
	deathtype = MAKE_STRING( "sentrygun" );
	m_iRightBound = 45;
	m_iLeftBound = 315;
	m_iBaseTurnRate = 6;
	m_vecCurDishAngles = g_vecZero;
	m_flFieldOfView = 0.7f;
	ammo_shells = 25;
	maxammo_shells = 100;
	maxammo_rockets = 20;
	pev->flags |= FL_MONSTER;
	SET_MODEL( edict(), "models/sentry1.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );
	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );
	m_hEnemy = NULL;
	SetThink( &CTFSentrygun::SentryRotate );
	pev->nextthink = gpGlobals->time + 0.5;
}

void CTFSentrygun::Sentry_Explode()
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLODEMODEL );
	WRITE_COORD( pev->origin.x ); WRITE_COORD( pev->origin.y ); WRITE_COORD( pev->origin.z );
	WRITE_COORD( 400 ); WRITE_SHORT( m_iShardIndex ); WRITE_SHORT( 15 ); WRITE_BYTE( 25 );
	MESSAGE_END();
	pev->takedamage = DAMAGE_NO;
	if ( !real_owner->has_disconnected )
		RadiusDamage( pev, real_owner->pev, (float)(8 * ammo_rockets + 75), CLASS_NONE, DMG_BLAST | 0x04000000 );
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLOSION );
	WRITE_COORD( pev->origin.x ); WRITE_COORD( pev->origin.y ); WRITE_COORD( pev->origin.z );
	WRITE_SHORT( g_sModelIndexFireball ); WRITE_BYTE( 30 ); WRITE_BYTE( 15 ); WRITE_BYTE( 3 );
	MESSAGE_END();
	dremove( m_pOtherSection );
	dremove( this );
}

void CTFSentrygun::Killed( entvars_t* pevInflictor, entvars_t* pevAttacker, int iGib )
{
	(void)iGib;
	if ( pev->solid == SOLID_NOT )
		return;
	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
		0, 0, 1024, 0, 0, 0 );
	CBasePlayer* pOwner = (CBasePlayer*)(CBaseEntity*)real_owner;
	if ( !pOwner->no_sentry_message )
		ClientPrint( pOwner->pev, HUD_PRINTCENTER, "#Sentry_destroyed" );
	pOwner->has_sentry = 0;
	pOwner->no_sentry_message = 0;
	ClearSentryMenus( this );
	pev->solid = SOLID_NOT;
	m_pOtherSection->pev->solid = SOLID_NOT;
	SetThink( &CTFSentrygun::Sentry_Explode );
	pev->nextthink = gpGlobals->time + 0.1;
	if ( pevAttacker )
	{
		CBasePlayer* pAttacker = (CBasePlayer*)CBaseEntity::Instance( pevAttacker );
		if ( pAttacker->IsPlayer() && !pAttacker->IsAlly( pOwner ) )
		{
			pAttacker->TF_AddFrags( 1 );
			MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
			WRITE_BYTE( pAttacker->entindex() ); WRITE_BYTE( -1 ); WRITE_STRING( "sentrygun" );
			MESSAGE_END();
		}
		if ( real_owner )
		{
			char weaponName[30];
			GetWeapon( pevInflictor, pevAttacker, weaponName );
			const char* pszOwnerTeam = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
			if ( pevAttacker == pevInflictor )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Sentry_Destroyed\"\n", STRING( pOwner->pev->netname ),
					GETPLAYERUSERID( pOwner->edict() ), GETPLAYERAUTHID( pOwner->edict() ), pszOwnerTeam );
			}
			else if ( pevAttacker != pOwner->pev )
			{
				const char* pszTeam = pAttacker->team_no ? GetTeamName( pAttacker->team_no ) : "SPECTATOR";
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Sentry_Destroyed\" against \"%s<%i><%s><%s>\" with \"%s\"\n",
					STRING( pevAttacker->netname ), GETPLAYERUSERID( pAttacker->edict() ), GETPLAYERAUTHID( pAttacker->edict() ),
					pszTeam, STRING( pOwner->pev->netname ), GETPLAYERUSERID( pOwner->edict() ),
					GETPLAYERAUTHID( pOwner->edict() ), pszOwnerTeam, weaponName );
			}
		}
	}
}

int CTFSentrygun::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	int result = CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	if ( result )
	{
		pev->renderfx = kRenderFxGlowShell;
		switch ( team_no )
		{
		case 1: pev->rendercolor = Vector( 0, 0, 255 ); break;
		case 2: pev->rendercolor = Vector( 255, 0, 0 ); break;
		case 3: pev->rendercolor = Vector( 245, 255, 0 ); break;
		default: pev->rendercolor = Vector( 0, 215, 45 ); break;
		}
		pev->renderamt = 150;
	}
	return result;
}

void CTFSentrygun::TeamFortress_TakeEMPBlast( entvars_t* pevGren )
{
	CBaseEntity* pOther = CBaseEntity::Instance( pevGren->owner );
	ENT( pevGren );
	if ( !((int)gpGlobals->teamplay & 0x1E) || !IsAlly( pOther ) || real_owner == pOther )
		TakeDamage( pevGren, pOther->pev, 200, DMG_BLAST );
}

void CTFSentrygunBase::Remove()
{
	CBasePlayer* pOwner = (CBasePlayer*)(CBaseEntity*)real_owner;
	CBaseEntity* pSentry = NULL;
	while ( (pSentry = UTIL_FindEntityByClassname( pSentry, "building_sentrygun" )) != NULL )
	{
		if ( pSentry->real_owner == pOwner )
			break;
	}
	if ( pSentry )
	{
		if ( !pOwner->no_sentry_message )
			ClientPrint( pOwner->pev, HUD_PRINTCENTER, "#Sentry_destroyed" );
		const char* pszTeam = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
		UTIL_LogPrintf( "\"<-1><><>\" triggered \"Sentry_destroyed\" against \"%s<%i><%s><%s>\"\n",
			STRING( pOwner->pev->netname ), GETPLAYERUSERID( pOwner->edict() ), GETPLAYERAUTHID( pOwner->edict() ), pszTeam );
		pOwner->has_sentry = 0;
		pOwner->no_sentry_message = 0;
		ClearSentryMenus( pSentry );
		dremove( pSentry );
	}
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLOSION );
	WRITE_COORD( pev->origin.x ); WRITE_COORD( pev->origin.y ); WRITE_COORD( pev->origin.z );
	WRITE_SHORT( g_sModelIndexFireball ); WRITE_BYTE( 30 ); WRITE_BYTE( 15 ); WRITE_BYTE( 3 );
	MESSAGE_END();
	dremove( this );
}

void CTFSentrygun::Remove()
{
	CBasePlayer* pOwner = (CBasePlayer*)(CBaseEntity*)real_owner;
	if ( !pOwner->no_sentry_message )
		ClientPrint( pOwner->pev, HUD_PRINTCENTER, "#Sentry_destroyed" );
	const char* pszTeam = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
	UTIL_LogPrintf( "\"<-1><><>\" triggered \"Sentry_destroyed\" against \"%s<%i><%s><%s>\"\n",
		STRING( pOwner->pev->netname ), GETPLAYERUSERID( pOwner->edict() ), GETPLAYERAUTHID( pOwner->edict() ), pszTeam );
	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
		0, 0, 1024, 0, 0, 0 );
	pOwner->has_sentry = 0;
	pOwner->no_sentry_message = 0;
	ClearSentryMenus( this );
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLOSION );
	WRITE_COORD( pev->origin.x ); WRITE_COORD( pev->origin.y ); WRITE_COORD( pev->origin.z );
	WRITE_SHORT( g_sModelIndexFireball ); WRITE_BYTE( 30 ); WRITE_BYTE( 15 ); WRITE_BYTE( 3 );
	MESSAGE_END();
	dremove( this );
	CBaseEntity* pBase = NULL;
	while ( (pBase = UTIL_FindEntityByClassname( pBase, "building_sentrygun_base" )) != NULL )
	{
		if ( pBase->real_owner == pOwner )
		{
			dremove( pBase );
			break;
		}
	}
}

void CTFSentrygun::SetSentryAnim( TFTURRET_ANIM anim )
{
	if ( pev->sequence != anim )
	{
		if ( (anim != TFTURRET_ANIM_FIRE && anim != TFTURRET_ANIM_SPIN) ||
			(pev->sequence != TFTURRET_ANIM_FIRE && pev->sequence != TFTURRET_ANIM_SPIN) )
			pev->frame = 0;
		pev->sequence = anim;
		ResetSequenceInfo();
	}
}

void CTFSentrygun::CheckShield()
{
	pev->renderamt -= 40;
	if ( pev->renderamt <= 0 )
	{
		pev->renderfx = kRenderFxNone;
		pev->renderamt = 0;
		pev->rendercolor = g_vecZero;
	}
}

BOOL CTFSentrygun::FInViewCone( CBaseEntity* pEntity )
{
	UTIL_MakeVectors( m_vecCurAngles );
	Vector2D vecDir = (pEntity->pev->origin - pev->origin).Make2D().Normalize();
	return DotProduct( vecDir, gpGlobals->v_forward.Make2D() ) > m_flFieldOfView;
}

BOOL CTFSentrygun::ValidTarget( CBaseEntity* pTarget )
{
	return pTarget && pTarget->IsAlive() && !pTarget->is_feigning &&
		(gpGlobals->teamplay == 0 || team_no == 0 || (!IsAlly( pTarget ) && pTarget->undercover_team != team_no)) &&
		!(pTarget->pev->flags & FL_NOTARGET) && !(pTarget->items & 0x80000) && FVisible( pTarget );
}

int CTFSentrygun::Range( CBaseEntity* pTarget )
{
	int targetDistance = (int)(pTarget->pev->origin + pTarget->pev->view_ofs - (pev->origin + pev->view_ofs)).Length();
	if ( targetDistance < 120 ) return 0;
	if ( targetDistance < 500 ) return 1;
	if ( targetDistance < 1000 ) return 2;
	return 3;
}

BOOL CTFSentrygun::FindTarget()
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer != (CBaseEntity*)real_owner && ValidTarget( pPlayer ) )
		{
			int range = Range( pPlayer );
			if ( range < 2 || (range == 2 && FInViewCone( pPlayer )) )
			{
				m_hEnemy = pPlayer;
				FoundTarget();
				return TRUE;
			}
		}
	}
	CBaseEntity* pEntity = NULL;
	while ( (pEntity = UTIL_FindEntityByClassname( pEntity, "building_teleporter" )) != NULL )
	{
		if ( ValidTarget( pEntity ) )
		{
			int range = Range( pEntity );
			if ( range < 2 || (range == 2 && FInViewCone( pEntity )) )
			{
				m_hEnemy = pEntity;
				FoundTarget();
				return TRUE;
			}
		}
	}
	return FALSE;
}

void CTFSentrygun::FoundTarget()
{
	if ( ammo_shells > 0 || (ammo_rockets > 0 && weapon == 3) )
		EMIT_SOUND_DYN( edict(), CHAN_ITEM, "weapons/turrspot.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	SetThink( &CTFSentrygun::Attack );
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextAttack = gpGlobals->time + 0.1;
	if ( gpGlobals->time > super_damage_finished )
		super_damage_finished = gpGlobals->time + 0.5;
}

BOOL CTFSentrygun::Fire()
{
	UTIL_MakeAimVectors( m_vecCurAngles );
	Vector vecAimDir = (m_hEnemy->pev->origin - pev->origin).Normalize();
	BOOL bFiredRocket = FALSE;
	if ( weapon == 3 && ammo_rockets > 0 && gpGlobals->time > super_damage_finished )
	{
		Vector vecSrc, vecAng;
		GetAttachment( (ammo_rockets & 1) ? 3 : 2, vecSrc, vecAng );
		vecAng = UTIL_VecToAngles( vecAimDir );
		vecAng.x = -vecAng.x;
		CTFRpgRocket* pRocket = CTFRpgRocket::CreateRpgRocket( vecSrc, vecAng, real_owner, NULL );
		pRocket->deathtype = MAKE_STRING( "sentrygun" );
		pRocket->pev->velocity = vecAimDir * 800;
		UTIL_ParametricRocket( pRocket->pev, vecSrc, vecAng, real_owner->edict() );
		EMIT_SOUND_DYN( edict(), CHAN_ITEM, "weapons/rocketfire1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		--ammo_rockets;
		super_damage_finished = gpGlobals->time + 3;
		if ( ammo_rockets == 10 )
			ClientPrint( real_owner->pev, HUD_PRINTNOTIFY, "#Sentry_rocketslow" );
		if ( ammo_rockets == 0 )
			ClientPrint( real_owner->pev, HUD_PRINTNOTIFY, "#Sentry_rocketsout" );
		bFiredRocket = TRUE;
	}
	if ( ammo_shells > 0 )
	{
		SetSentryAnim( TFTURRET_ANIM_FIRE );
		pev->effects |= EF_MUZZLEFLASH;
		Vector vecSrc, vecAng;
		GetAttachment( weapon == 3 && (ammo_shells & 1) ? 1 : 0, vecSrc, vecAng );
		FireBullets( 1, vecSrc, vecAimDir, g_vecZero, 2048, BULLET_MONSTER_9MM, 4, 16, real_owner->pev );
		EMIT_SOUND_DYN( edict(), CHAN_ITEM, "weapons/pl_gun3.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		--ammo_shells;
	}
	else if ( bFiredRocket )
		return FALSE;
	if ( ammo_shells == 0 && RANDOM_FLOAT( 0, 1 ) < 0.1 )
	{
		ClientPrint( real_owner->pev, HUD_PRINTNOTIFY, "#Sentry_shellsout" );
		return TRUE;
	}
	if ( ammo_shells == 20 )
		ClientPrint( real_owner->pev, HUD_PRINTNOTIFY, "#Sentry_shellslow" );
	return TRUE;
}

BOOL CTFSentrygun::CheckSentry()
{
	BOOL bValid = m_pOtherSection && (pev->origin - m_pOtherSection->pev->origin).Length() <= 24;
	CBaseEntity* pArea = NULL;
	while ( (pArea = UTIL_FindEntityByClassname( pArea, "func_nobuild" )) != NULL )
	{
		if ( pev->origin.x >= pArea->pev->mins.x && pev->origin.y >= pArea->pev->mins.y && pev->origin.z >= pArea->pev->mins.z &&
			pev->origin.x <= pArea->pev->maxs.x && pev->origin.y <= pArea->pev->maxs.y && pev->origin.z <= pArea->pev->maxs.z )
			bValid = FALSE;
	}
	if ( bValid )
		return TRUE;
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Sentry_malfunc", STRING( real_owner->pev->netname ) );
	const char* pszTeam = real_owner->team_no ? GetTeamName( real_owner->team_no ) : "SPECTATOR";
	UTIL_LogPrintf( "\"<-1><><>\" triggered \"Sentry_Malfunction\" against \"%s<%i><%s><%s>\"\n",
		STRING( real_owner->pev->netname ), GETPLAYERUSERID( real_owner->edict() ), GETPLAYERAUTHID( real_owner->edict() ), pszTeam );
	Killed( NULL, NULL, 0 );
	return FALSE;
}

void CTFSentrygun::Upgrade()
{
	++weapon;
	pev->max_health *= 1.2;
	pev->health = pev->max_health;
	maxammo_shells = (int)(maxammo_shells * 1.2);
	EMIT_SOUND_DYN( edict(), CHAN_ITEM, "weapons/turrset.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	SET_MODEL( edict(), weapon == 2 ? "models/sentry2.mdl" : "models/sentry3.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );
	if ( weapon != 2 )
		SetBoneController( 2, -m_vecCurAngles.x );
	ResetSequenceInfo();
}

BOOL CTFSentrygun::EngineerUse( CBasePlayer* pPlayer )
{
	if ( weapon <= 2 && pPlayer->ammo_cells >= 130 )
	{
		pPlayer->ammo_cells -= 130;
		ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "#Sentry_upgrade", UTIL_dtos1( weapon + 1 ) );
		if ( weapon == 1 )
			LogSentryUse( pPlayer, real_owner, "\"%s<%i><%s><%s>\" triggered \"Sentry_Upgrade_Level_2\"\n",
				"\"%s<%i><%s><%s>\" triggered \"Sentry_Upgrade_Level_2\" against \"%s<%i><%s><%s>\"\n" );
		else if ( weapon == 2 )
			LogSentryUse( pPlayer, real_owner, "\"%s<%i><%s><%s>\" triggered \"Sentry_Upgrade_Level_3\"\n",
				"\"%s<%i><%s><%s>\" triggered \"Sentry_Upgrade_Level_3\" against \"%s<%i><%s><%s>\"\n" );
		Upgrade();
	}
	else
	{
		int cells = min( pPlayer->ammo_cells, (int)ceil( (pev->max_health - pev->health) / 5 ) );
		if ( pev->health < pev->max_health && cells > 0 )
		{
			pPlayer->ammo_cells -= cells;
			pev->health = min( pev->health + 5 * cells, pev->max_health );
			ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "#Sentry_repair" );
			LogSentryUse( pPlayer, real_owner, "\"%s<%i><%s><%s>\" triggered \"Sentry_Repair\"\n",
				"\"%s<%i><%s><%s>\" triggered \"Sentry_Repair\" against \"%s<%i><%s><%s>\"\n" );
		}
		else
		{
			if ( ammo_shells < maxammo_shells && pPlayer->ammo_shells > 0 )
			{
				int shells = min( min( pPlayer->ammo_shells, 40 ), maxammo_shells - ammo_shells );
				pPlayer->ammo_shells -= shells;
				ammo_shells += shells;
				ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "#Sentry_inshells" );
			}
			if ( ammo_rockets < maxammo_rockets && weapon == 3 && pPlayer->ammo_rockets > 0 )
			{
				int rockets = min( min( pPlayer->ammo_rockets, 20 ), maxammo_rockets - ammo_rockets );
				pPlayer->ammo_rockets -= rockets;
				ammo_rockets += rockets;
				ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "#Sentry_inrockets" );
			}
		}
	}
	return TRUE;
}

int CTFSentrygun::MoveTurret()
{
	int state = 0;
	if ( m_vecCurAngles.x != m_vecGoalAngles.x )
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1.0f : -1.0f;
		m_vecCurAngles.x += 0.1 * (5 * m_iBaseTurnRate) * flDir;
		if ( flDir == 1 )
			m_vecCurAngles.x = min( m_vecCurAngles.x, m_vecGoalAngles.x );
		else
			m_vecCurAngles.x = max( m_vecCurAngles.x, m_vecGoalAngles.x );
		SetBoneController( 1, -m_vecCurAngles.x );
		if ( weapon == 3 )
			SetBoneController( 2, -m_vecCurAngles.x );
		state = 1;
	}
	if ( m_vecCurAngles.y != m_vecGoalAngles.y )
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1.0f : -1.0f;
		float flDist = fabs( m_vecGoalAngles.y - m_vecCurAngles.y );
		BOOL bCrossesZero = flDist > 180;
		if ( bCrossesZero )
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}
		if ( m_hEnemy )
		{
			if ( flDist > 30 && m_fTurnRate < 30 * m_iBaseTurnRate )
				m_fTurnRate += (float)(3 * m_iBaseTurnRate);
		}
		else if ( flDist > 30 )
		{
			if ( m_fTurnRate < 10 * m_iBaseTurnRate )
				m_fTurnRate += (float)m_iBaseTurnRate;
		}
		else if ( m_fTurnRate > 5 * m_iBaseTurnRate )
			m_fTurnRate -= (float)m_iBaseTurnRate;

		m_vecCurAngles.y += 0.1 * m_fTurnRate * flDir;
		if ( (flDir == -1 && !bCrossesZero) || (flDir != -1 && bCrossesZero) )
		{
			if ( m_vecCurAngles.y < m_vecGoalAngles.y )
				m_vecCurAngles.y = m_vecGoalAngles.y;
		}
		else if ( m_vecCurAngles.y > m_vecGoalAngles.y )
			m_vecCurAngles.y = m_vecGoalAngles.y;
		if ( m_vecCurAngles.y < 0 )
			m_vecCurAngles.y += 360;
		else if ( m_vecCurAngles.y >= 360 )
			m_vecCurAngles.y -= 360;
		if ( flDist < 0.05 * m_iBaseTurnRate )
			m_vecCurAngles.y = m_vecGoalAngles.y;
		SetBoneController( 0, m_vecCurAngles.y - pev->angles.y );
		state = 1;
	}
	if ( !state )
		m_fTurnRate = (float)m_iBaseTurnRate;
	return state;
}

void CTFSentrygun::SentryRotate()
{
	SetSentryAnim( TFTURRET_ANIM_SPIN );
	StudioFrameAdvance();
	DoDamageEffects( pev, pev->max_health * 0.66, pev->max_health * 0.33 );
	if ( gpGlobals->time > m_flNextCheck )
	{
		if ( !CheckSentry() )
			return;
		m_flNextCheck = gpGlobals->time + 3;
	}
	if ( pev->renderfx != kRenderFxNone )
		CheckShield();
	pev->nextthink = gpGlobals->time + 0.1;
	if ( !FindTarget() && !MoveTurret() )
	{
		m_pOtherSection->CheckBelowBuilding( 24 );
		UpdateEntityEvents( this );
		if ( RANDOM_FLOAT( 0, 1 ) < 0.1 )
			EMIT_SOUND_DYN( edict(), CHAN_ITEM, "weapons/turridle.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		m_vecGoalAngles.y = (float)(m_bTurningRight ? m_iLeftBound : m_iRightBound);
		m_bTurningRight = !m_bTurningRight;
		if ( RANDOM_FLOAT( 0, 1 ) < 0.3 )
			m_vecGoalAngles.x = (float)(int)RANDOM_FLOAT( -10, 10 );
	}
}

void CTFSentrygun::Attack()
{
	if ( pev->renderfx != kRenderFxNone )
		CheckShield();
	StudioFrameAdvance();
	DoDamageEffects( pev, pev->max_health * 0.66, pev->max_health * 0.33 );
	pev->nextthink = gpGlobals->time + 0.1;
	if ( !ValidTarget( m_hEnemy ) || (weapon == 3 ? ammo_shells <= 0 && ammo_rockets <= 0 : ammo_shells <= 0) )
	{
		SetThink( &CTFSentrygun::SentryRotate );
		pev->nextthink = gpGlobals->time + 0.1;
		m_hEnemy = NULL;
		return;
	}
	Vector vecMid = pev->origin + pev->view_ofs;
	Vector vecDirToEnemy = m_hEnemy->BodyTarget( vecMid ) - vecMid;
	Vector angToTarget = UTIL_VecToAngles( vecDirToEnemy );
	angToTarget.y = UTIL_AngleMod( angToTarget.y );
	if ( angToTarget.x < -180 ) angToTarget.x += 360;
	if ( angToTarget.x > 180 ) angToTarget.x -= 360;
	angToTarget.x = min( max( angToTarget.x, -50.0f ), 50.0f );
	m_vecGoalAngles.y = angToTarget.y;
	m_vecGoalAngles.x = angToTarget.x;
	MoveTurret();
	if ( gpGlobals->time >= m_flNextAttack && (m_vecGoalAngles - m_vecCurAngles).Length() <= 10 )
	{
		Fire();
		m_flNextAttack = gpGlobals->time + (weapon == 1 ? 0.2 : 0.1);
	}
	else
		SetSentryAnim( TFTURRET_ANIM_SPIN );
}
