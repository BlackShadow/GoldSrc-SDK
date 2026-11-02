#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"

LINK_ENTITY_TO_CLASS( tf_weapon_axe, CTFAxe );
LINK_ENTITY_TO_CLASS( tf_weapon_spanner, CTFSpanner );
LINK_ENTITY_TO_CLASS( tf_weapon_knife, CTFKnife );
LINK_ENTITY_TO_CLASS( tf_weapon_medikit, CTFMedikit );

void CTFAxe::Spawn()
{
	Precache();
	m_iId = 5;
	m_iClip = -1;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

void CTFSpanner::Spawn()
{
	Precache();
	m_iId = 4;
	m_iClip = -1;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
	classid = 1;
}

void CTFKnife::Spawn()
{
	Precache();
	m_iId = 23;
	m_iClip = -1;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
	classid = 0;
}

void CTFMedikit::Spawn()
{
	Precache();
	m_iId = 3;
	m_iClip = -1;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
	classid = 2;
}

void CTFAxe::Precache()
{
	PRECACHE_MODEL( "models/v_umbrella.mdl" );
	PRECACHE_MODEL( "models/p_umbrella.mdl" );
	PRECACHE_MODEL( "models/v_tfc_crowbar.mdl" );
	PRECACHE_MODEL( "models/p_crowbar.mdl" );
	PRECACHE_MODEL( "models/p_crowbar2.mdl" );
	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod3.wav" );
	PRECACHE_SOUND( "weapons/cbar_miss1.wav" );
	classid = 3;
	m_usAxe = PRECACHE_EVENT( 1, "events/wpn/tf_axe.sc" );
	m_usAxeDecal = PRECACHE_EVENT( 1, "events/wpn/tf_axedecal.sc" );
}

void CTFSpanner::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_spanner.mdl" );
	PRECACHE_MODEL( "models/p_spanner.mdl" );
	PRECACHE_MODEL( "models/p_spanner2.mdl" );
	m_usAxe = PRECACHE_EVENT( 1, "events/wpn/tf_axe.sc" );
	m_usAxeDecal = PRECACHE_EVENT( 1, "events/wpn/tf_axedecal.sc" );
}

void CTFKnife::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_knife.mdl" );
	PRECACHE_MODEL( "models/p_knife.mdl" );
	PRECACHE_MODEL( "models/p_knife2.mdl" );
	m_usAxe = PRECACHE_EVENT( 1, "events/wpn/tf_axe.sc" );
	m_usAxeDecal = PRECACHE_EVENT( 1, "events/wpn/tf_axedecal.sc" );
	m_usKnife = PRECACHE_EVENT( 1, "events/wpn/tf_knife.sc" );
}

void CTFMedikit::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_medkit.mdl" );
	PRECACHE_MODEL( "models/p_medkit.mdl" );
	PRECACHE_MODEL( "models/p_medkit2.mdl" );
	m_usAxe = PRECACHE_EVENT( 1, "events/wpn/tf_axe.sc" );
	m_usAxeDecal = PRECACHE_EVENT( 1, "events/wpn/tf_axedecal.sc" );
	m_usNormalShot = PRECACHE_EVENT( 1, "events/wpn/tf_mednormal.sc" );
	m_usSuperShot = PRECACHE_EVENT( 1, "events/wpn/tf_medsuper.sc" );
	m_usSteamShot = PRECACHE_EVENT( 1, "events/wpn/tf_medsteam.sc" );
}

int CTFAxe::GetItemInfo( ItemInfo* info )
{
	info->pszAmmo1 = NULL;
	info->iAmmo1 = -1;
	info->pszName = STRING( pev->classname );
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = -1;
	info->iSlot = 0;
	info->iPosition = 3;
	info->iId = m_iId = 5;
	info->iWeight = 0;
	return 1;
}

int CTFSpanner::GetItemInfo( ItemInfo* info )
{
	info->pszAmmo1 = "uranium";
	info->iAmmo1 = -1;
	info->pszName = STRING( pev->classname );
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = -1;
	info->iSlot = 0;
	info->iPosition = 2;
	info->iId = m_iId = 4;
	info->iWeight = 0;
	return 1;
}

int CTFKnife::GetItemInfo( ItemInfo* info )
{
	info->pszAmmo1 = NULL;
	info->iAmmo1 = -1;
	info->pszName = STRING( pev->classname );
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = -1;
	info->iSlot = 0;
	info->iPosition = 4;
	info->iId = m_iId = 23;
	info->iWeight = 0;
	return 1;
}

int CTFMedikit::GetItemInfo( ItemInfo* info )
{
	info->pszAmmo1 = NULL;
	info->iAmmo1 = -1;
	info->pszName = STRING( pev->classname );
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = -1;
	info->iSlot = 0;
	info->iPosition = 1;
	info->iId = m_iId = 3;
	info->iWeight = 0;
	return 1;
}

BOOL CTFAxe::Deploy()
{
	if ( m_pPlayer && m_pPlayer->pev->playerclass == PC_CIVILIAN )
		return DefaultDeploy( "models/v_umbrella.mdl", "models/p_umbrella.mdl", 1, "crowbar", 1 );
	return DefaultDeploy( "models/v_tfc_crowbar.mdl", "models/p_crowbar.mdl", 1, "crowbar", 1 );
}

BOOL CTFAxe::IsUseable()
{
	return TRUE;
}

BOOL CTFSpanner::Deploy()
{
	return DefaultDeploy( "models/v_tfc_spanner.mdl", "models/p_spanner.mdl", 4, "crowbar", 1 );
}

BOOL CTFKnife::Deploy()
{
	return DefaultDeploy( "models/v_tfc_knife.mdl", "models/p_knife.mdl", 4, "knife", 1 );
}

BOOL CTFMedikit::Deploy()
{
	return DefaultDeploy( "models/v_tfc_medkit.mdl", "models/p_medkit.mdl", 5, "medkit", 1 );
}

void CTFAxe::Holster( int )
{
	m_pPlayer->m_flNextAttack = 0.5f;
	SendWeaponAnim( 2, 1 );
}

void CTFSpanner::Holster( int )
{
	m_pPlayer->m_flNextAttack = 0.5f;
	SendWeaponAnim( 5, 1 );
}

void CTFKnife::Holster( int )
{
	m_pPlayer->m_flNextAttack = 0.5f;
	SendWeaponAnim( 5, 1 );
}

void CTFMedikit::Holster( int )
{
	m_pPlayer->m_flNextAttack = 0.5f;
	SendWeaponAnim( 4, 1 );
}

void CTFAxe::PlayAnim( int )
{
}

void CTFMedikit::PlaySound( int, float )
{
}

void CTFMedikit::PlayDecal()
{
}

void CTFAxe::Smack()
{
	PlayDecal();
}

void CTFSpanner::PlayAnim( int animation )
{
	if ( animation == 0 )
		SendWeaponAnim( 1, 1 );
	else if ( animation == 1 )
		SendWeaponAnim( 3, 1 );
}

void CTFKnife::PlayAnim( int animation )
{
	if ( animation == 0 )
		SendWeaponAnim( 2, 1 );
	else if ( animation == 1 )
		SendWeaponAnim( 2, 1 );
}

void CTFMedikit::PlayAnim( int animation )
{
	if ( animation == 0 )
		SendWeaponAnim( 2, 1 );
	else if ( animation == 1 )
		SendWeaponAnim( 3, 1 );
}

void CTFSpanner::WeaponIdle()
{
	if ( m_flTimeWeaponIdle <= 0 )
	{
		m_flTimeWeaponIdle = 12.5f;
		SendWeaponAnim( 0, 1 );
	}
}

void CTFKnife::WeaponIdle()
{
	if ( m_flTimeWeaponIdle <= 0 )
	{
		m_flTimeWeaponIdle = 7.5f;
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			SendWeaponAnim( 0, 1 );
			break;
		case 1:
			SendWeaponAnim( 1, 1 );
			break;
		}
	}
}

void CTFMedikit::WeaponIdle()
{
	if ( m_flTimeWeaponIdle <= 0 )
	{
		m_flTimeWeaponIdle = 7.5f;
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			SendWeaponAnim( 0, 1 );
			break;
		case 1:
			SendWeaponAnim( 1, 1 );
			break;
		}
	}
}

void CTFAxe::PrimaryAttack()
{
	m_bHullHit = FALSE;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector start = m_pPlayer->GetGunPosition();
	Vector end = start + gpGlobals->v_forward * 32;
	TraceResult trace;
	UTIL_TraceLine( start, end, dont_ignore_monsters, m_pPlayer->edict(), &trace );
	DB_LogShots( 1 );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usAxe, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, classid, 0, 0, 0 );
	if ( trace.flFraction >= 1 )
	{
		m_bHullHit = TRUE;
#ifndef CLIENT_DLL
		UTIL_TraceHull( start, end, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &trace );
		if ( trace.flFraction < 1 )
		{
			CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
			if ( !hit || hit->IsBSPModel() )
			{
				Vector mins( -16, -16, -18 );
				Vector maxs( 16, 16, 18 );
				FindHullIntersection( start, trace, mins, maxs, m_pPlayer->edict() );
			}
			end = trace.vecEndPos;
		}
#endif
	}
	if ( trace.flFraction >= 1 )
	{
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.4f );
		m_flTimeWeaponIdle = 5;
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		return;
	}
	CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	BOOL hitTarget = AxeHit( hit, gpGlobals->v_forward, &trace );
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.4f );
	m_flTimeWeaponIdle = 5;
	int volume;
	if ( hitTarget && hit && hit->Classify() != CLASS_NONE && hit->Classify() != CLASS_MACHINE )
	{
		m_pPlayer->m_iWeaponVolume = 128;
		volume = 51;
		if ( !hit->IsAlive() )
			return;
	}
	else
		volume = 512;
	m_trHit = trace;
	SetThink( &CTFAxe::Smack );
	pev->nextthink = gpGlobals->time + 0.2;
	m_pPlayer->m_iWeaponVolume = volume;
}

BOOL CTFAxe::AxeHit( CBaseEntity* target, Vector direction, TraceResult* trace )
{
	ClearMultiDamage();
	if ( m_pPlayer->pev->playerclass == PC_SPY && FClassnameIs( target->pev, "player" ) )
	{
		UTIL_MakeVectors( target->pev->v_angle );
		Vector right = gpGlobals->v_right;
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		if ( right.x * gpGlobals->v_forward.y - right.y * gpGlobals->v_forward.x <= 0 )
			target->TraceAttack( m_pPlayer->pev, 40, direction, trace, DMG_CLUB );
		else
			target->TraceAttack( m_pPlayer->pev, m_bHullHit ? 120.0f : 60.0f, direction, trace, DMG_CLUB | DMG_IGNOREARMOR );
	}
	else
		target->TraceAttack( m_pPlayer->pev, 20, direction, trace, DMG_CLUB );
	ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
	return TRUE;
}

BOOL CTFSpanner::AxeHit( CBaseEntity* target, Vector direction, TraceResult* trace )
{
	if ( target->goal_activation & TFGA_SPANNER )
	{
		target->do_triggerwork = TRUE;
		target->Use( m_pPlayer, m_pPlayer, USE_TOGGLE, TFGA_SPANNER );
		target->do_triggerwork = FALSE;
		return TRUE;
	}
	if ( m_pPlayer->IsAlly( target ) )
		return target->EngineerUse( m_pPlayer ) != 0;
	ClearMultiDamage();
	target->TraceAttack( m_pPlayer->pev, 20, direction, trace, DMG_CLUB );
	ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
	return TRUE;
}

BOOL CTFMedikit::AxeHit( CBaseEntity* target, Vector direction, TraceResult* trace )
{
	if ( !FClassnameIs( target->pev, "player" ) || !( target->pev->health > 0 ) )
		return FALSE;
	if ( !m_pPlayer->IsAlly( target ) )
	{
		ClearMultiDamage();
		target->TraceAttack( m_pPlayer->pev, 10, direction, trace, DMG_CLUB );
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
		if ( target->tfstate & TFSTATE_INFECTED || target->pev->playerclass == PC_MEDIC ||
			cb_prematch_time > gpGlobals->time )
			return TRUE;
#ifndef CLIENT_DLL
		if ( target->pev->deadflag != DEAD_NO && target->pev->deadflag != 5 )
			return TRUE;
#endif
		target->tfstate |= TFSTATE_INFECTED;
#ifndef CLIENT_DLL
		CBaseEntity* timer = target->CreateTimer( TF_TIMER_INFECTION );
		timer->pev->nextthink = gpGlobals->time + 2;
		timer->SetThink( &CBaseEntity::Timer_Infection );
		timer->pev->enemy = m_pPlayer->edict();
#endif
		target->infection_team_no = m_pPlayer->team_no;
#ifndef CLIENT_DLL
		const char* targetTeam = target->team_no ? GetTeamName( target->team_no ) : "SPECTATOR";
		const char* targetAuth = GETPLAYERAUTHID( target->edict() );
		int targetId = GETPLAYERUSERID( target->edict() );
		const char* targetName = STRING( target->pev->netname );
		const char* playerTeam = m_pPlayer->team_no ? GetTeamName( m_pPlayer->team_no ) : "SPECTATOR";
		const char* playerAuth = GETPLAYERAUTHID( m_pPlayer->edict() );
		int playerId = GETPLAYERUSERID( m_pPlayer->edict() );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Medic_Infection\" against \"%s<%i><%s><%s>\"\n",
			STRING( m_pPlayer->pev->netname ), playerId, playerAuth, playerTeam,
			targetName, targetId, targetAuth, targetTeam );
#endif
		return TRUE;
	}
	CBaseEntity* timer = target->FindTimer( TF_TIMER_CONCUSSION );
	if ( timer )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Medic_cureconc",
			STRING( m_pPlayer->pev->netname ), STRING( target->pev->netname ) );
#ifndef CLIENT_DLL
		const char* targetTeam = target->team_no ? GetTeamName( target->team_no ) : "SPECTATOR";
		const char* targetAuth = GETPLAYERAUTHID( target->edict() );
		int targetId = GETPLAYERUSERID( target->edict() );
		const char* targetName = STRING( target->pev->netname );
		const char* playerTeam = m_pPlayer->team_no ? GetTeamName( m_pPlayer->team_no ) : "SPECTATOR";
		const char* playerAuth = GETPLAYERAUTHID( m_pPlayer->edict() );
		int playerId = GETPLAYERUSERID( m_pPlayer->edict() );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Medic_Cured_Concussion\" against \"%s<%i><%s><%s>\"\n",
			STRING( m_pPlayer->pev->netname ), playerId, playerAuth, playerTeam,
			targetName, targetId, targetAuth, targetTeam );
#endif
		if ( timer->team_no != m_pPlayer->team_no )
			m_pPlayer->TF_AddFrags( 1 );
		timer->pev->nextthink = gpGlobals->time;
		timer->SetThink( &CBaseEntity::SUB_Remove );
	}
	if ( target->tfstate & TFSTATE_HALLUCINATING )
	{
		timer = target->FindTimer( TF_TIMER_HALLUCINATION );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Medic_curehalluc",
			STRING( m_pPlayer->pev->netname ), STRING( target->pev->netname ) );
#ifndef CLIENT_DLL
		const char* targetTeam = target->team_no ? GetTeamName( target->team_no ) : "SPECTATOR";
		const char* targetAuth = GETPLAYERAUTHID( target->edict() );
		int targetId = GETPLAYERUSERID( target->edict() );
		const char* targetName = STRING( target->pev->netname );
		const char* playerTeam = m_pPlayer->team_no ? GetTeamName( m_pPlayer->team_no ) : "SPECTATOR";
		const char* playerAuth = GETPLAYERAUTHID( m_pPlayer->edict() );
		int playerId = GETPLAYERUSERID( m_pPlayer->edict() );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Medic_Cured_Hallucinations\" against \"%s<%i><%s><%s>\"\n",
			STRING( m_pPlayer->pev->netname ), playerId, playerAuth, playerTeam,
			targetName, targetId, targetAuth, targetTeam );
#endif
		if ( timer->team_no != m_pPlayer->team_no )
			m_pPlayer->TF_AddFrags( 1 );
		target->tfstate &= ~TFSTATE_HALLUCINATING;
		timer->pev->nextthink = gpGlobals->time;
		timer->SetThink( &CBaseEntity::SUB_Remove );
	}
	if ( target->tfstate & TFSTATE_TRANQUILISED )
	{
		timer = target->FindTimer( TF_TIMER_TRANQUILISATION );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Medic_curetranq",
			STRING( m_pPlayer->pev->netname ), STRING( target->pev->netname ) );
#ifndef CLIENT_DLL
		const char* targetTeam = target->team_no ? GetTeamName( target->team_no ) : "SPECTATOR";
		const char* targetAuth = GETPLAYERAUTHID( target->edict() );
		int targetId = GETPLAYERUSERID( target->edict() );
		const char* targetName = STRING( target->pev->netname );
		const char* playerTeam = m_pPlayer->team_no ? GetTeamName( m_pPlayer->team_no ) : "SPECTATOR";
		const char* playerAuth = GETPLAYERAUTHID( m_pPlayer->edict() );
		int playerId = GETPLAYERUSERID( m_pPlayer->edict() );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Medic_Cured_Tranquilisation\" against \"%s<%i><%s><%s>\"\n",
			STRING( m_pPlayer->pev->netname ), playerId, playerAuth, playerTeam,
			targetName, targetId, targetAuth, targetTeam );
#endif
		if ( timer->team_no != m_pPlayer->team_no )
			m_pPlayer->TF_AddFrags( 1 );
		target->tfstate &= ~TFSTATE_TRANQUILISED;
		( (CBasePlayer*)target )->TeamFortress_SetSpeed();
		timer->pev->nextthink = gpGlobals->time;
		timer->SetThink( &CBaseEntity::SUB_Remove );
	}
	if ( target->tfstate & TFSTATE_INFECTED )
	{
		float damage = (float)(int)( target->pev->health * 0.5f );
		SpawnBlood( target->pev->origin, target->BloodColor(), damage );
#ifndef CLIENT_DLL
		timer = target->FindTimer( TF_TIMER_INFECTION );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Medic_cureinfection",
			STRING( m_pPlayer->pev->netname ), STRING( target->pev->netname ) );
		const char* targetTeam = target->team_no ? GetTeamName( target->team_no ) : "SPECTATOR";
		const char* targetAuth = GETPLAYERAUTHID( target->edict() );
		int targetId = GETPLAYERUSERID( target->edict() );
		const char* targetName = STRING( target->pev->netname );
		const char* playerTeam = m_pPlayer->team_no ? GetTeamName( m_pPlayer->team_no ) : "SPECTATOR";
		const char* playerAuth = GETPLAYERAUTHID( m_pPlayer->edict() );
		int playerId = GETPLAYERUSERID( m_pPlayer->edict() );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Medic_Cured_Infection\" against \"%s<%i><%s><%s>\"\n",
			STRING( m_pPlayer->pev->netname ), playerId, playerAuth, playerTeam,
			targetName, targetId, targetAuth, targetTeam );
		if ( timer->team_no != m_pPlayer->team_no )
			m_pPlayer->TF_AddFrags( 1 );
		CBaseEntity* owner = CBaseEntity::Instance( timer->pev->owner );
		target->TakeDamage( pev, owner->pev, damage, DMG_IGNOREARMOR | DMG_NERVEGAS );
		target->tfstate &= ~TFSTATE_INFECTED;
		timer->pev->nextthink = gpGlobals->time;
		timer->SetThink( &CBaseEntity::SUB_Remove );
#endif
	}
	if ( target->numflames > 0 )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Medic_curefire",
			STRING( m_pPlayer->pev->netname ), STRING( target->pev->netname ) );
#ifndef CLIENT_DLL
		const char* targetTeam = target->team_no ? GetTeamName( target->team_no ) : "SPECTATOR";
		const char* targetAuth = GETPLAYERAUTHID( target->edict() );
		int targetId = GETPLAYERUSERID( target->edict() );
		const char* targetName = STRING( target->pev->netname );
		const char* playerTeam = m_pPlayer->team_no ? GetTeamName( m_pPlayer->team_no ) : "SPECTATOR";
		const char* playerAuth = GETPLAYERAUTHID( m_pPlayer->edict() );
		int playerId = GETPLAYERUSERID( m_pPlayer->edict() );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Medic_Doused_Fire\" against \"%s<%i><%s><%s>\"\n",
			STRING( m_pPlayer->pev->netname ), playerId, playerAuth, playerTeam,
			targetName, targetId, targetAuth, targetTeam );
#endif
		target->numflames = 0;
		PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usSteamShot, 0,
			(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	}
	if ( target->pev->max_health > target->pev->health )
	{
		PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usNormalShot, 0,
			(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
		target->TakeHealth( 200, 0 );
#ifndef CLIENT_DLL
		const char* targetTeam = target->team_no ? GetTeamName( target->team_no ) : "SPECTATOR";
		const char* targetAuth = GETPLAYERAUTHID( target->edict() );
		int targetId = GETPLAYERUSERID( target->edict() );
		const char* targetName = STRING( target->pev->netname );
		const char* playerTeam = m_pPlayer->team_no ? GetTeamName( m_pPlayer->team_no ) : "SPECTATOR";
		const char* playerAuth = GETPLAYERAUTHID( m_pPlayer->edict() );
		int playerId = GETPLAYERUSERID( m_pPlayer->edict() );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Medic_Heal\" against \"%s<%i><%s><%s>\"\n",
			STRING( m_pPlayer->pev->netname ), playerId, playerAuth, playerTeam,
			targetName, targetId, targetAuth, targetTeam );
#endif
		return TRUE;
	}
	if ( target->pev->max_health + 50 > target->pev->health )
	{
		PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usSuperShot, 0,
			(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
		target->TakeHealth( 5, DMG_IGNORE_MAXHEALTH );
#ifndef CLIENT_DLL
		const char* targetTeam = target->team_no ? GetTeamName( target->team_no ) : "SPECTATOR";
		const char* targetAuth = GETPLAYERAUTHID( target->edict() );
		int targetId = GETPLAYERUSERID( target->edict() );
		const char* targetName = STRING( target->pev->netname );
		const char* playerTeam = m_pPlayer->team_no ? GetTeamName( m_pPlayer->team_no ) : "SPECTATOR";
		const char* playerAuth = GETPLAYERAUTHID( m_pPlayer->edict() );
		int playerId = GETPLAYERUSERID( m_pPlayer->edict() );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Medic_Heal\" against \"%s<%i><%s><%s>\"\n",
			STRING( m_pPlayer->pev->netname ), playerId, playerAuth, playerTeam,
			targetName, targetId, targetAuth, targetTeam );
#endif
		if ( !( target->items & IT_SUPERHEALTH ) )
		{
			target->items |= IT_SUPERHEALTH;
#ifndef CLIENT_DLL
			timer = target->CreateTimer( TF_TIMER_ROTHEALTH );
			timer->pev->nextthink = gpGlobals->time + 5;
			timer->SetThink( &CBaseEntity::Timer_HealthRot );
#endif
		}
	}
	return TRUE;
}
