/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
/*
 
===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "skill.h"
#include "func_break.h"
#include "soundent.h"
#include "decals.h"

#define SF_DETONATE 1
#define DMG_NOBLOOD ( 1 << 27 )


//===================grenade


LINK_ENTITY_TO_CLASS( grenade, CGrenade );

void CGrenade::Precache()
{
	m_iBodyGibs = PRECACHE_MODEL( "models/rockgibs.mdl" );
}

void CGrenade::Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;

	switch ( g_GrenType )
	{
	default:
		MAKE_STRING_CLASS( "grenade", pev );
		break;
	case G_STICKGRENADE:
		MAKE_STRING_CLASS( "grenade2", pev );
		break;
	}

	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/w_grenade.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;
	pev->iuser1 = 0;
}

//
// Grenade Explode
//
void CGrenade::Explode( Vector vecSrc, Vector vecAim )
{
	(void)vecSrc;
	(void)vecAim;

	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -32 ), ignore_monsters, ENT( pev ), &tr );

	Explode( &tr, DMG_BLAST | DMG_NOBLOOD );
}

void CGrenade::Explode( TraceResult *pTrace, int bitsDamageType )
{
	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( UTIL_PointContents( pTrace->vecEndPos ) == CONTENTS_SOLID )
		pev->origin = pTrace->vecEndPos + ( pTrace->vecPlaneNormal * 8 );

	CBaseEntity *pAttacker = CBaseEntity::Instance( pev->owner );

	if ( pAttacker->pev->team == 3 || ( ( FClassnameIs( pev, "grenade" ) || FClassnameIs( pev, "grenade2" ) ) && pAttacker->pev->team != pev->team ) )
		pev->dmg = 0;

	BOOL bUnderwater = FALSE;
	int iContents = UTIL_PointContents( pev->origin );

	if ( iContents == CONTENTS_WATER )
		bUnderwater = TRUE;

	PLAYBACK_EVENT_FULL( 0, edict(), g_pGameRules->Explosion(), 0, (float *)&g_vecZero, (float *)&g_vecZero, ( pev->dmg - 100 ) * 0.6, 15, g_sModelIndexFireball, 0, bUnderwater, bitsDamageType & DMG_MORTAR );

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage( pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType );

	if ( RANDOM_FLOAT( 0, 1 ) < 0.5 )
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	else
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );

	PLAYBACK_EVENT_FULL( 0, NULL, g_pGameRules->SparkShower(), 0, (float *)&pev->origin, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	pev->effects |= EF_NODRAW;
	pev->velocity = g_vecZero;
	UTIL_Remove( this );
}

void CGrenade::Smoke()
{
}

void CGrenade::RemoveMe()
{
	pev->nextthink = gpGlobals->time;
	pev->model = 0;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->effects |= EF_NODRAW;
	pev->velocity = g_vecZero;
	UTIL_Remove( this );
}

void CGrenade::Killed( entvars_t* pevAttacker, int iGib )
{
	(void)pevAttacker;
	(void)iGib;
	Detonate();
}


// Timed grenade, this think is called when time runs out.
void CGrenade::DetonateUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	(void)pActivator;
	(void)pCaller;
	(void)useType;
	(void)value;
	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate()
{
	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 400, 0.3f );
	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time + 1;
}

void CGrenade::Detonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, ENT( pev ), &tr );

	UTIL_ScreenShake( tr.vecEndPos, 25, 150, 1, 750 );
	Explode( &tr, DMG_BLAST );
}


//
// Contact grenade, explode when it touches something
// 
void CGrenade::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT( pev ), &tr );

	Explode( &tr, DMG_BLAST );
}

void CGrenade::BounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	pev->avelocity.x = 0;
	pev->angles = pev->vuser1;

	// only do damage if we're moving fairly fast
	if ( m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100 )
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if ( pevOwner )
		{
			if ( !strcmp( "func_breakable", STRING( pOther->pev->classname ) ) )
			{
				CBreakable *pBreak = (CBreakable *)CBaseEntity::Instance( pOther->edict() );

				if ( pBreak->IsBreakable() && pBreak->m_Material == matGlass )
				{
					m_iOldMoveType = pev->movetype;
					pev->movetype = MOVETYPE_NOCLIP;
					m_flClipTime = (int)( gpGlobals->time + 0.5 );

					TraceResult tr = UTIL_GetGlobalTrace();
					ClearMultiDamage();
					pOther->TraceAttack( pevOwner, 1, gpGlobals->v_forward, &tr, DMG_NOBLOOD );
					ApplyMultiDamage( pev, pevOwner );
					return;
				}
			}

			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack( pevOwner, 1, gpGlobals->v_forward, &tr, DMG_NOBLOOD );
			ApplyMultiDamage( pev, pevOwner );
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity.
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity;
	vecTestVelocity.z *= 0.45;

	if ( !m_fRegisteredSound && vecTestVelocity.Length() <= 60 )
	{
		//ALERT( at_console, "Grenade Registered!: %f\n", vecTestVelocity.Length() );

		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving.
		// go ahead and emit the danger sound.

		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, (int)( pev->dmg / 0.4 ), 0.3f );
		m_fRegisteredSound = TRUE;
	}

	if ( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8f;

		pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if ( pev->framerate > 1.0 )
		pev->framerate = 1;
	else if ( pev->framerate < 0.5 )
		pev->framerate = 0;
}

void CGrenade::SlideTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector (300, 300, 300);

	if ( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95f;

		if ( pev->velocity.x != 0 || pev->velocity.y != 0 )
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CGrenade::BounceSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM );	break;
	case 1:	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM );	break;
	case 2:	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM );	break;
	}
}

void CGrenade::TumbleThink( void )
{
	if ( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	if ( pev->iuser1 == 1 )
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->movetype == MOVETYPE_NOCLIP && m_flClipTime < gpGlobals->time )
	{
		pev->movetype = m_iOldMoveType;
		pev->velocity = pev->velocity + gpGlobals->v_forward * 50;
	}

	if ( pev->dmgtime - 1 < gpGlobals->time )
	{
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * ( pev->dmgtime - gpGlobals->time ), 400, 0.1f );
	}

	if ( pev->dmgtime <= gpGlobals->time )
	{
		SetThink( &CGrenade::Detonate );
	}
	if ( pev->waterlevel != 0 )
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2f;
	}
}

void CGrenade::DangerSoundThink( void )
{
	if ( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, (int)pev->velocity.Length(), 0.2f );
	pev->nextthink = gpGlobals->time + 0.2;

	if ( pev->waterlevel != 0 )
	{
		pev->velocity = pev->velocity * 0.5;
	}
}

CGrenade *CGrenade::ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles( pGrenade->pev->velocity );
	pGrenade->pev->owner = ENT( pevOwner );

	pGrenade->SetTouch( &CGrenade::BounceTouch );	// Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( &CGrenade::TumbleThink );
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if ( time < 0.1 )
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector( 0, 0, 0 );
	}

	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1.0;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8f;

	SET_MODEL( ENT( pGrenade->pev ), "models/w_grenade.mdl" );
	pGrenade->pev->dmg = 100;

	return pGrenade;
}

CGrenade* CGrenade::ShootContact( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenade* pGrenade = GetClassPtr( (CGrenade*)NULL );
	pGrenade->Spawn();
	pGrenade->pev->gravity = 0.5f;
	UTIL_SetOrigin( pGrenade, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles( pGrenade->pev->velocity );
	pGrenade->pev->owner = ENT( pevOwner );

	// make monsters afaid of it while in the air
	pGrenade->SetThink( &CGrenade::DangerSoundThink );
	pGrenade->pev->nextthink = gpGlobals->time;

	// Tumble in air
	pGrenade->pev->avelocity.x = RANDOM_FLOAT( -100, -500 );

	// Explode on contact
	pGrenade->SetTouch( &CGrenade::ExplodeTouch );
	pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;
	return pGrenade;
}

CGrenade *CGrenade::ShootPickup( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, grenType value, float fFriction )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	UTIL_SetOrigin( pGrenade, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = pevOwner->angles;
	pGrenade->pev->vuser1 = pevOwner->angles;
	pGrenade->pev->owner = ENT( pevOwner );

	CBaseEntity *pAttacker = CBaseEntity::Instance( pevOwner );
	pGrenade->pev->iuser2 = pAttacker->pev->team;
	pGrenade->pev->team = pAttacker->pev->team;

	pGrenade->g_GrenType = value;
	pGrenade->Spawn();

	pGrenade->SetTouch( &CGrenade::BounceTouch );	// Bounce if touched
	pGrenade->SetUse( &CGrenade::PickupUse );

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( &CGrenade::TumbleThink );
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if ( time < 0.1 )
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector( 0, 0, 0 );
	}

	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1.0;

	// Tumble through the air
	pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = fFriction;

	switch ( pGrenade->g_GrenType )
	{
	case G_MILLSGRENADE:
		SET_MODEL( ENT( pGrenade->pev ), "models/w_mills.mdl" );
		pGrenade->pev->dmg = gSkillData.plrDmgHandGrenade;
		break;
	case G_HANDGRENADE:
		SET_MODEL( ENT( pGrenade->pev ), "models/w_grenade.mdl" );
		pGrenade->pev->dmg = gSkillData.plrDmgHandGrenade;
		break;
	default:
		SET_MODEL( ENT( pGrenade->pev ), "models/w_stick.mdl" );
		pGrenade->pev->dmg = gSkillData.plrDmgStickGrenade;
		break;
	}

	return pGrenade;
}

void CGrenade::PickupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	(void)pCaller;
	(void)useType;
	(void)value;

	CBasePlayerItem *pWeapon = NULL;

	if ( !pActivator->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pActivator;

	if ( !pPlayer )
		return;

	if ( pPlayer->pev->iuser3 == 2 )
		return;

	if ( pPlayer->PlayerHasExplodingGrenades() )
		return;

	switch ( g_GrenType )
	{
	case G_MILLSGRENADE:
	case G_HANDGRENADE:
		pPlayer->GiveNamedItem( "weapon_handgrenade_ex" );
		break;
	case G_STICKGRENADE:
		pPlayer->GiveNamedItem( "weapon_stickgrenade_ex" );
		break;
	}

	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		pWeapon = pPlayer->m_rgpPlayerItems[i];

		while ( pWeapon )
		{
			if ( FClassnameIs( pWeapon->pev, "weapon_stickgrenade_ex" ) || FClassnameIs( pWeapon->pev, "weapon_handgrenade_ex" ) )
				break;

			pWeapon = pWeapon->m_pNext;
		}

		if ( pWeapon )
		{
			pPlayer->SwitchWeapon( pWeapon, FALSE );
			pPlayer->m_pActiveItem->SetDmgTime( pev->dmgtime );
			UTIL_Remove( this );
		}
	}
}

void CGrenade::UseSatchelCharges( entvars_t* pevOwner, SATCHELCODE code )
{
	if ( !pevOwner )
		return;
	CBaseEntity* pOwner = CBaseEntity::Instance( pevOwner );
	edict_t* pGrenade = NULL;
	while ( !FNullEnt( pGrenade = FIND_ENTITY_BY_STRING( pGrenade, "classname", "grenade" ) ) )
	{
		CBaseEntity* pEntity = CBaseEntity::Instance( pGrenade );
		if ( pEntity && (pEntity->pev->spawnflags & SF_DETONATE) && pEntity->pev->owner == pOwner->edict() )
		{
			if ( code == SATCHEL_DETONATE )
				pEntity->Use( pOwner, pOwner, USE_ON, 0 );
			else
				pEntity->pev->owner = NULL;
		}
	}
}

void CGrenade::RocketExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	if ( !strcmp( "func_breakable", STRING( pOther->pev->classname ) ) )
	{
		CBreakable *pBreak = (CBreakable *)CBaseEntity::Instance( pOther->edict() );

		if ( pBreak && pBreak->m_Material == matGlass )
		{
			if ( pBreak->IsBreakable() )
			{
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4456 6246 )
#endif
				TraceResult tr = UTIL_GetGlobalTrace();
#ifdef _MSC_VER
#pragma warning( pop )
#endif
				ClearMultiDamage();
				pOther->TraceAttack( VARS( pev->owner ), 1, gpGlobals->v_forward, &tr, DMG_NOBLOOD );
				ApplyMultiDamage( pev, VARS( pev->owner ) );

				UTIL_MakeVectors( pev->angles );
				Vector vecSrc = pev->origin;

				if ( pOther->pev->health <= 0 )
				{
					char *clsname = (char *)STRING( pev->classname );

					if ( FStrEq( clsname, "shell_bazooka" ) )
					{
						CBazookaShell *pRocket = CBazookaShell::CreateRocket( vecSrc, pev->angles, CBaseEntity::Instance( pev->owner ) );
						pRocket->pev->velocity = pev->velocity;
					}
					else if ( FStrEq( clsname, "shell_piat" ) )
					{
						CPIATShell *pRocket = CPIATShell::CreateRocket( vecSrc, pev->angles, CBaseEntity::Instance( pev->owner ) );
						pRocket->pev->velocity = pev->velocity;
					}
					else if ( FStrEq( clsname, "shell_pschreck" ) )
					{
						CPschreckShell *pRocket = CPschreckShell::CreateRocket( vecSrc, pev->angles, CBaseEntity::Instance( pev->owner ) );
						pRocket->pev->velocity = pev->velocity;
					}
				}

				UTIL_Remove( this );
			}

			return;
		}
	}

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, dont_ignore_monsters, ENT( pev ), &tr );

	const char *pTextureName = TRACE_TEXTURE( ENT( 0 ), vecSpot, vecSpot + pev->velocity.Normalize() * 64 );

	if ( pTextureName && !_strnicmp( pTextureName, "sky", 3 ) )
	{
		UTIL_Remove( this );
		return;
	}

	Explode( &tr, DMG_BLAST | DMG_SONIC );
}

void CGrenade::MortarShellExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	if ( FStrEq( "func_breakable", STRING( pOther->pev->classname ) ) )
	{
		CBreakable *pBreak = (CBreakable *)CBaseEntity::Instance( pOther->edict() );

		if ( pBreak && pBreak->m_Material == matGlass )
		{
			if ( pBreak->IsBreakable() )
			{
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4456 6246 )
#endif
				TraceResult tr = UTIL_GetGlobalTrace();
#ifdef _MSC_VER
#pragma warning( pop )
#endif
				ClearMultiDamage();
				pOther->TraceAttack( VARS( pev->owner ), 1, gpGlobals->v_forward, &tr, DMG_NOBLOOD );
				ApplyMultiDamage( pev, VARS( pev->owner ) );

				UTIL_MakeVectors( pev->angles );
				Vector vecSrc = pev->origin;

				char *clsname = (char *)STRING( pev->classname );

				if ( FStrEq( clsname, "shell_bazooka" ) )
				{
					CBazookaShell *pRocket = CBazookaShell::CreateRocket( vecSrc, pev->angles, CBaseEntity::Instance( pev->owner ) );
					pRocket->pev->velocity = pev->velocity;
				}
				else if ( FStrEq( clsname, "shell_piat" ) )
				{
					CPIATShell *pRocket = CPIATShell::CreateRocket( vecSrc, pev->angles, CBaseEntity::Instance( pev->owner ) );
					pRocket->pev->velocity = pev->velocity;
				}
				else if ( FStrEq( clsname, "shell_pschreck" ) )
				{
					CPschreckShell *pRocket = CPschreckShell::CreateRocket( vecSrc, pev->angles, CBaseEntity::Instance( pev->owner ) );
					pRocket->pev->velocity = pev->velocity;
				}

				UTIL_Remove( this );
			}

			return;
		}
	}

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, dont_ignore_monsters, ENT( pev ), &tr );

	const char *pTextureName = TRACE_TEXTURE( ENT( 0 ), vecSpot, vecSpot + pev->velocity.Normalize() * 64 );

	if ( pTextureName && !_strnicmp( pTextureName, "sky", 3 ) )
	{
		UTIL_Remove( this );
		return;
	}

	Explode( &tr, DMG_BLAST | DMG_SONIC | DMG_MORTAR );
}

//======================end grenade

