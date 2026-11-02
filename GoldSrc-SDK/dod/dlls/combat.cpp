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

===== combat.cpp ========================================================

  functions dealing with damage infliction & death

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"
#include "soundent.h"
#include "decals.h"
#include "animation.h"
#include "weapons.h"
#include "func_break.h"
#include "gamerules.h"
#include "../cl_dll/dod_shared.h"

#define DMG_BAYONET		( 1 << 15 )
#define DMG_GARAND_BUTT	( 1 << 16 )
#define DMG_ENFIELD_BUTT	( 1 << 17 )
#define DMG_K43_BUTT		( 1 << 25 )
#define DMG_RIFLE		( 1 << 26 )
#define DMG_NOBLOOD		( 1 << 27 )
#define DMG_SELFBLAST		( 1 << 28 )

#define GIB_BAYONET		3
#define GIB_GARAND_BUTT	4
#define GIB_ENFIELD_BUTT	5
#define GIB_K43_BUTT		6

extern DLL_GLOBAL Vector g_vecAttackDir;
extern entvars_t* g_pevLastInflictor;


// HACKHACK -- The gib velocity equations don't work
void CGib :: LimitVelocity( void )
{
	float length = pev->velocity.Length();

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if ( length > 1500.0 )
		pev->velocity = pev->velocity.Normalize() * 1500;		// This should really be sv_maxvelocity * 0.75 or something
}

void CGib::SpawnStickyGibs( entvars_t* pevVictim, Vector vecOrigin, int cGibs )
{
	if ( g_Language == LANGUAGE_GERMAN )
		return;
	for ( int i = 0; i < cGibs; i++ )
	{
		CGib* pGib = GetClassPtr( (CGib*)NULL );
		pGib->Spawn( "models/stickygib.mdl" );
		pGib->pev->body = RANDOM_LONG( 0, 2 );
		if ( pevVictim )
		{
			pGib->pev->origin.x = vecOrigin.x + RANDOM_FLOAT( -3, 3 );
			pGib->pev->origin.y = vecOrigin.y + RANDOM_FLOAT( -3, 3 );
			pGib->pev->origin.z = vecOrigin.z + RANDOM_FLOAT( -3, 3 );
			pGib->pev->velocity = -g_vecAttackDir;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT( -0.15, 0.15 );
			pGib->pev->velocity.y += RANDOM_FLOAT( -0.15, 0.15 );
			pGib->pev->velocity.z += RANDOM_FLOAT( -0.15, 0.15 );
			pGib->pev->velocity = pGib->pev->velocity * 900;
			pGib->pev->avelocity.x = RANDOM_FLOAT( 250, 400 );
			pGib->pev->avelocity.y = RANDOM_FLOAT( 250, 400 );

			// copy owner's blood color
			pGib->m_bloodColor = CBaseEntity::Instance( pevVictim )->BloodColor();
			if ( pevVictim->health > -50 )
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			else if ( pevVictim->health > -200 )
				pGib->pev->velocity = pGib->pev->velocity * 2;
			else
				pGib->pev->velocity = pGib->pev->velocity * 4;
			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize( pGib->pev, g_vecZero, g_vecZero );
			pGib->SetTouch( &CGib::StickyGibTouch );
			pGib->SetThink( NULL );
		}
		pGib->LimitVelocity();
	}
}

void CGib::SpawnHeadGib( entvars_t* pevVictim, int allies )
{
	CGib* pGib = GetClassPtr( (CGib*)NULL );
	pGib->Spawn( allies ? "models/allied_gibs.mdl" : "models/axis_gibs.mdl" );
	pGib->pev->body = 0;
	if ( pevVictim )
	{
		pGib->pev->origin = pevVictim->origin + pevVictim->view_ofs;
		edict_t* pentPlayer = FIND_CLIENT_IN_PVS( pGib->edict() );
		if ( RANDOM_LONG( 0, 100 ) <= 5 && pentPlayer )
		{
			entvars_t* pevPlayer = VARS( pentPlayer );
			pGib->pev->velocity = ( pevPlayer->origin + pevPlayer->view_ofs - pGib->pev->origin ).Normalize() * 300;
			pGib->pev->velocity.z += 100;
		}
		else
		{
			float z = RANDOM_FLOAT( 200, 300 );
			float y = RANDOM_FLOAT( -100, 100 );
			float x = RANDOM_FLOAT( -100, 100 );
			pGib->pev->velocity = Vector( x, y, z );
		}
		pGib->pev->avelocity.x = RANDOM_FLOAT( 100, 200 );
		pGib->pev->avelocity.y = RANDOM_FLOAT( 100, 300 );

		// copy owner's blood color
		pGib->m_bloodColor = CBaseEntity::Instance( pevVictim )->BloodColor();
		if ( pevVictim->health > -50 )
			pGib->pev->velocity = pGib->pev->velocity * 0.7;
		else if ( pevVictim->health > -200 )
			pGib->pev->velocity = pGib->pev->velocity * 2;
		else
			pGib->pev->velocity = pGib->pev->velocity * 4;
	}
	pGib->LimitVelocity();
}

void CGib::SpawnRandomGibs( entvars_t*, int, int )
{
}

BOOL CBaseMonster :: HasHumanGibs( void )
{
	int myClass = Classify();

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 6288 )
#endif
	if ( myClass == CLASS_NONE || myClass == CLASS_MACHINE ||
		( myClass == CLASS_PLAYER_BIOWEAPON && myClass == CLASS_ALIEN_BIOWEAPON ) )
		return FALSE;
#ifdef _MSC_VER
#pragma warning( pop )
#endif

	return m_bloodColor == BLOOD_COLOR_RED;
}

BOOL CBaseMonster :: HasAlienGibs( void )
{
	int myClass = Classify();

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 6288 )
#endif
	if ( myClass == CLASS_NONE || myClass == CLASS_MACHINE ||
		( myClass == CLASS_PLAYER_BIOWEAPON && myClass == CLASS_ALIEN_BIOWEAPON ) )
		return FALSE;
#ifdef _MSC_VER
#pragma warning( pop )
#endif

	return m_bloodColor == BLOOD_COLOR_YELLOW;
}

void CBaseMonster::FadeMonster()
{
	StopAnimation();
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->avelocity = g_vecZero;
	pev->animtime = gpGlobals->time;
	pev->effects |= EF_NOINTERP;
	SUB_StartFadeOut();
}

void CBaseMonster::GibMonster()
{
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM );
	CGib::SpawnHeadGib( pev, pev->team == 1 );
	if ( !IsPlayer() )
	{
		// don't remove players!
		SetThink( &CBaseMonster::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
}

Activity CBaseMonster :: GetDeathActivity ( void )
{
	Activity	deathActivity;
	float		flDot;
	TraceResult	tr;
	Vector		vecSrc;

	vecSrc = Center();

	deathActivity = DOD_SIMPLE;

	UTIL_MakeVectors ( pev->angles );
	flDot = DotProduct ( gpGlobals->v_forward, g_vecAttackDir * -1 );

	bool bBack = ( flDot <= -0.3 );
	bool bFront = ( flDot > 0.3 );
	bool bProne = false;
	bool bDucking = false;

	if ( IsPlayer() )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)this;
		bProne = pPlayer->IsProne();
		bDucking = pPlayer->IsDucking();
	}

	if ( bProne )
	{
		deathActivity = DOD_PRONE1;
	}
	else if ( ( m_bitsDamageType & DMG_GARAND_BUTT ) || ( m_bitsDamageType & DMG_K43_BUTT ) )
	{
		deathActivity = DOD_BUTTSTOCK;
	}
	else if ( bBack && bDucking )
	{
		deathActivity = DOD_CROUCH_BACKWARD;
	}
	else if ( bFront && bDucking )
	{
		deathActivity = DOD_CROUCH_HEADSHOT;
	}
	else
	{
		switch ( m_LastHitGroup )
		{
			// try to pick a region-specific death.
		case HITGROUP_HEAD:
			deathActivity = ACT_DIE_HEADSHOT;
			break;

		case HITGROUP_STOMACH:
			if ( bBack )
				deathActivity = ACT_DIE_GUTSHOT;
			else
				deathActivity = ACT_DIEFORWARD;
			break;

		case HITGROUP_CHEST:
			if ( bBack )
				deathActivity = ACT_DIEBACKWARD;
			else
				deathActivity = ACT_DIEFORWARD;
			break;

		default:
			deathActivity = DOD_SIMPLE;
			break;
		}
	}

	return deathActivity;
}

//=========================================================
// GetSmallFlinchActivity - determines the best type of flinch
// anim to play.
//=========================================================
Activity CBaseMonster :: GetSmallFlinchActivity ( void )
{
	Activity	flinchActivity;
	BOOL		fTriedDirection;
	float		flDot;

	fTriedDirection = FALSE;
	UTIL_MakeVectors ( pev->angles );
	flDot = DotProduct ( gpGlobals->v_forward, g_vecAttackDir * -1 );

	switch ( m_LastHitGroup )
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchActivity = ACT_FLINCH_HEAD;
		break;
	case HITGROUP_STOMACH:
		flinchActivity = ACT_FLINCH_STOMACH;
		break;
	case HITGROUP_LEFTARM:
		flinchActivity = ACT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchActivity = ACT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchActivity = ACT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchActivity = ACT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchActivity = ACT_SMALL_FLINCH;
		break;
	}


	// do we have a sequence for the ideal activity?
	if ( LookupActivity ( flinchActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		flinchActivity = ACT_SMALL_FLINCH;
	}

	return flinchActivity;
}

void CBaseMonster :: BecomeDead( void )
{
	pev->takedamage = DAMAGE_YES;// don't let autoaim aim at corpses.

	// give the corpse half of the monster's original maximum health.
	pev->health = pev->max_health / 2;
	pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.

	// make the corpse fly away from the attack vector
	pev->movetype = MOVETYPE_TOSS;
	//pev->flags &= ~FL_ONGROUND;
	//pev->origin.z += 2;
	//pev->velocity = g_vecAttackDir * -1;
	//pev->velocity = pev->velocity * RANDOM_FLOAT( 300, 400 );
}

BOOL CBaseMonster::ShouldGibMonster( int )
{
	return FALSE;
}

void CBaseMonster::CallGibMonster()
{
	BOOL fade = FALSE;
	if ( HasHumanGibs() )
		fade = CVAR_GET_FLOAT( "violence_hgibs" ) == 0;
	else if ( HasAlienGibs() )
		fade = CVAR_GET_FLOAT( "violence_agibs" ) == 0;
	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT; // do something with the body. while monster blows up
	if ( fade )
		FadeMonster();
	else
		pev->effects = EF_NODRAW; // make the model invisible.
	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();
	if ( pev->health < -99 )
		pev->health = 0;
	if ( ShouldFadeOnDeath() && !fade )
		UTIL_Remove( this );
}


/*
============
Killed
============
*/
void CBaseMonster::Killed( entvars_t*, int )
{
	if ( HasMemory( bits_MEMORY_KILLED ) )
		return;
	Remember( bits_MEMORY_KILLED );

	// clear the deceased's sound channels.(may have been firing or reloading when killed)
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM );
	SetConditions( bits_COND_LIGHT_DAMAGE );
	m_IdealMonsterState = MONSTERSTATE_DEAD;

	// tell owner ( if any ) that we're dead.This is mostly for MonsterMaker functionality.
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner )
		pOwner->DeathNotice( pev );
	if ( pev->flags & FL_MONSTER )
	{
		SetTouch( NULL );
		BecomeDead();
	}
	if ( pev->health < -99 )
		pev->health = 0;
	m_IdealMonsterState = MONSTERSTATE_DEAD;
}

//
// fade out - slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER! 
// SET A FUTURE THINK AND A RENDERMODE!!
void CBaseEntity::SUB_StartFadeOut()
{
	if ( pev->rendermode == kRenderNormal )
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}
	pev->solid = SOLID_NOT;
	pev->avelocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.1;
	SetThink( &CBaseEntity::SUB_FadeOut );
}

void CBaseEntity::SUB_FadeOut()
{
	if ( pev->renderamt > 7 )
	{
		pev->renderamt -= 7;
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.2;
		SetThink( &CBaseEntity::SUB_Remove );
	}
}

//=========================================================
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop 
// bouncing to emit their scent. That's what this function
// does.
//=========================================================
void CGib::WaitTillLand()
{
	if ( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}
	if ( pev->velocity == g_vecZero )
	{
		SetThink( &CGib::SUB_StartFadeOut );
		pev->nextthink = gpGlobals->time + m_lifeTime;

		// If you bleed, you stink!
		if ( m_bloodColor != DONT_BLEED )
			CSoundEnt::InsertSound( bits_SOUND_MEAT, pev->origin, 384, 25 );
	}
	else
	{
		// wait and check again in another half second.
		pev->nextthink = gpGlobals->time + 0.5;
	}
}

//
// Gib bounces on the ground or wall, sponges some blood down, too!
//
void CGib::BounceGibTouch( CBaseEntity* )
{

	//if ( RANDOM_LONG(0,1) )
	//	return;// don't bleed everytime

	if ( pev->flags & FL_ONGROUND )
	{
		pev->velocity = pev->velocity * 0.9;
		pev->angles.x = pev->angles.z = 0;
		pev->avelocity.x = pev->avelocity.z = 0;
	}
	else
	{
		if ( g_Language != LANGUAGE_GERMAN && m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED )
		{
			TraceResult tr;
			Vector vecSpot = pev->origin + Vector( 0, 0, 8 );
			UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -24 ), ignore_monsters, edict(), &tr );
			UTIL_BloodDecalTrace( &tr, m_bloodColor );
			m_cBloodDecals--;
		}
		if ( m_material != matNone && RANDOM_LONG( 0, 2 ) == 0 )
		{
			float flZVel = fabs( pev->velocity.z );
			float volume = 0.8 * min( 1.0, flZVel / 450.0 );
			CBreakable::MaterialSoundRandom( edict(), (Materials)m_material, volume );
		}
	}
}

//
// Sticky gib puts blood on the wall and stays put. 
//
void CGib::StickyGibTouch( CBaseEntity* pOther )
{
	SetThink( &CGib::SUB_Remove );
	pev->nextthink = gpGlobals->time + 10;
	if ( !FClassnameIs( pOther->pev, "worldspawn" ) )
	{
		pev->nextthink = gpGlobals->time;
		return;
	}
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 32, ignore_monsters, edict(), &tr );
	UTIL_BloodDecalTrace( &tr, m_bloodColor );
	pev->velocity = -tr.vecPlaneNormal;
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
}

//
// Throw a chunk
//
void CGib::Spawn( const char* szGibModel )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55; // deading the bounce a bit

	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_SLIDEBOX; /// hopefully this will fix the VELOCITY TOO LOW crap
	MAKE_STRING_CLASS( "gib", pev );
	SET_MODEL( edict(), szGibModel );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->nextthink = gpGlobals->time + 4;
	SetTouch( &CGib::BounceGibTouch );
	m_material = matNone;
	m_lifeTime = 25;
	SetThink( &CGib::WaitTillLand );
	m_cBloodDecals = 5; // how many blood decals this gib can place (1 per bounce until none remain). 
}

// take health
int CBaseMonster::TakeHealth( float flHealth, int bitsDamageType )
{
	if ( !pev->takedamage )
		return 0;
	m_bitsDamageType &= ~( bitsDamageType & ~( DMG_TIMEBASED | DMG_BLAST ) );
	return CBaseEntity::TakeHealth( flHealth, bitsDamageType );
}

/*
============
TakeDamage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
bitsDamageType indicates the type of damage sustained, ie: DMG_SHOCK

Time-based damage: only occurs while the monster is within the trigger_hurt.
When a monster is poisoned via an arrow etc it takes all the poison damage at once.



GLOBALS ASSUMED SET:  g_iSkillLevel
============
*/
int CBaseMonster::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if ( !pev->takedamage )
		return 0;
	if ( !IsAlive() )
		return DeadTakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	if ( pev->deadflag == DEAD_NO )
		PainSound(); // "Ouch!"
	m_bitsDamageType = bitsDamageType;
	if ( !FNullEnt( pevInflictor ) )
	{
		CBaseEntity* pInflictor = CBaseEntity::Instance( pevInflictor );
		if ( pInflictor )
		{
			Vector vecCenter = Center();
			Vector vecDir = ( pInflictor->Center() - Vector( 0, 0, 10 ) - vecCenter ).Normalize();
			g_vecAttackDir = vecDir.Normalize();
		}
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if ( IsPlayer() )
	{
		if ( pevInflictor )
			pev->dmg_inflictor = ENT( pevInflictor );
		pev->dmg_take += flDamage;

		// check for godmode or invincibility
		if ( pev->flags & FL_GODMODE )
			return 0;
	}

	// do the damage
	pev->health -= flDamage;


	// HACKHACK Don't kill monsters in a script.  Let them break their scripts first
	if ( m_MonsterState == MONSTERSTATE_SCRIPT )
	{
		SetConditions( bits_COND_LIGHT_DAMAGE );
		return 0;
	}
	if ( pev->health <= 0 )
	{
		g_pevLastInflictor = pevInflictor;
		if ( bitsDamageType & DMG_ALWAYSGIB )
			Killed( pevAttacker, GIB_ALWAYS );
		else if ( bitsDamageType & DMG_NEVERGIB )
			Killed( pevAttacker, GIB_NEVER );
		else if ( bitsDamageType & DMG_BAYONET )
			Killed( pevAttacker, GIB_BAYONET );
		else if ( bitsDamageType & DMG_ENFIELD_BUTT )
			Killed( pevAttacker, GIB_ENFIELD_BUTT );
		else if ( bitsDamageType & DMG_GARAND_BUTT )
			Killed( pevAttacker, GIB_GARAND_BUTT );
		else if ( bitsDamageType & DMG_K43_BUTT )
			Killed( pevAttacker, GIB_K43_BUTT );
		else
			Killed( pevAttacker, GIB_NEVER );
		g_pevLastInflictor = NULL;
		return 0;
	}
	if ( ( pev->flags & FL_MONSTER ) && !FNullEnt( pevAttacker )
		&& ( pevAttacker->flags & ( FL_MONSTER | FL_CLIENT ) ) )
	{ // only if the attack was a monster or client!

		// enemy's last known position is somewhere down the vector that the attack came from.
		if ( pevInflictor )
		{
			if ( !m_hEnemy || m_hEnemy->pev == pevInflictor || !HasConditions( bits_COND_SEE_ENEMY ) )
				m_vecEnemyLKP = pevInflictor->origin;
		}
		else
		{
			m_vecEnemyLKP = pev->origin + g_vecAttackDir * 64;
		}
		MakeIdealYaw( m_vecEnemyLKP );

		// add pain to the conditions 
		// !!!HACKHACK - fudged for now. Do we want to have a virtual function to determine what is light and 
		// heavy damage per monster class?
		if ( flDamage > 0 )
			SetConditions( bits_COND_LIGHT_DAMAGE );
		if ( flDamage >= 20 )
			SetConditions( bits_COND_HEAVY_DAMAGE );
	}
	return 1;
}

//=========================================================
// DeadTakeDamage - takedamage function called when a monster's
// corpse is damaged.
//=========================================================
int CBaseMonster::DeadTakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if ( !FNullEnt( pevInflictor ) )
	{
		CBaseEntity* pInflictor = CBaseEntity::Instance( pevInflictor );
		if ( pInflictor )
		{
			Vector vecCenter = Center();
			Vector vecDir = ( pInflictor->Center() - Vector( 0, 0, 10 ) - vecCenter ).Normalize();
			g_vecAttackDir = vecDir.Normalize();
		}
	}
	if ( bitsDamageType & ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_CLUB ) )
	{
		if ( pev->health <= flDamage )
		{
			pev->health = -50;
			Killed( pevAttacker, GIB_ALWAYS );
			return 0;
		}
		// Accumulate corpse gibbing damage, so you can gib with multiple hits
		pev->health -= flDamage * 0.1;
	}
	return 1;
}

float CBaseMonster :: DamageForce( float damage )
{
	float force = damage * ((32 * 32 * 72.0) / (pev->size.x * pev->size.y * pev->size.z)) * 5;

	if ( force > 1000.0)
	{
		force = 1000.0;
	}

	return force;
}

//
// RadiusDamage - this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
// 
// only damage ents that can clearly be seen by the explosion!


void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	if ( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0;

	int bInWater;
	bInWater = (UTIL_PointContents ( vecSrc ) == CONTENTS_WATER);

	vecSrc.z += 1;// in case grenade is lying on the ground

	if ( !pevAttacker )
		pevAttacker = pevInflictor;

	CBaseEntity *pInflictor = CBaseEntity::Instance( pevAttacker );
	CBasePlayer *pPlayer;
	pPlayer = NULL;

	if ( pInflictor && pInflictor->IsPlayer() )
		pPlayer = (CBasePlayer *)pInflictor;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{// houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			vecSpot = pEntity->BodyTarget( vecSrc );

			UTIL_TraceLine ( vecSrc, vecSpot, dont_ignore_monsters, ENT(pevInflictor), &tr );

			Vector dist;
			dist = vecSrc - pEntity->pev->origin;

			if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() || dist.Length() < 10 )
			{// the explosion can 'see' this entity, so hurt them!
				if (tr.fStartSolid)
				{
					// if we're stuck inside them, fixup the position and distance
					tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0;
				}

				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;

				if ( flAdjustedDamage < 0 )
				{
					flAdjustedDamage = 0;
				}

				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if (tr.flFraction != 1.0)
				{
					ClearMultiDamage( );

					if ( pInflictor == pEntity )
						bitsDamageType |= DMG_SELFBLAST;

					pEntity->TraceAttack( pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize( ), &tr, bitsDamageType );
					ApplyMultiDamage( pevInflictor, pevAttacker );
				}
				else
				{
					pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
			}
		}
	}
}

void CBaseMonster::RadiusDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	float mod = 1.0;

	if ( bitsDamageType & DMG_SONIC )
		mod = 1.5;
	else
		mod = 2.5;

	::RadiusDamage( pev->origin, pevInflictor, pevAttacker, flDamage, flDamage * mod, iClassIgnore, bitsDamageType );
}

void CBaseMonster::RadiusDamage( Vector vecSrc, entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( vecSrc, pevInflictor, pevAttacker, flDamage, flDamage * 2.5, iClassIgnore, bitsDamageType );
}


//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount 
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
//
// Used for many contact-range melee attacks. Bites, claws, etc.
//=========================================================
CBaseEntity* CBaseMonster :: CheckTraceHullAttack( float flDist, int iDamage, int iDmgType )
{
	TraceResult tr;

	if (IsPlayer())
		UTIL_MakeVectors( pev->angles );
	else
		UTIL_MakeAimVectors( pev->angles );

	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist );

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( iDamage > 0 )
		{
			pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
		}

		return pEntity;
	}

	return NULL;
}


//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
BOOL CBaseMonster :: FInViewCone ( CBaseEntity *pEntity )
{
	Vector2D	vec2LOS;
	float	flDot;

	UTIL_MakeVectors ( pev->angles );

	vec2LOS = ( pEntity->pev->origin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

	if ( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
BOOL CBaseMonster :: FInViewCone ( Vector *pOrigin )
{
	Vector2D	vec2LOS;
	float		flDot;

	UTIL_MakeVectors ( pev->angles );

	vec2LOS = ( *pOrigin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

	if ( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target
//=========================================================
BOOL CBaseEntity :: FVisible ( CBaseEntity *pEntity )
{
	TraceResult tr;
	Vector		vecLookerOrigin;
	Vector		vecTargetOrigin;

	if (FBitSet( pEntity->pev->flags, FL_NOTARGET ))
		return FALSE;

	// don't look through water
	if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3)
		|| (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
		return FALSE;

	vecLookerOrigin = pev->origin + pev->view_ofs;//look through the caller's 'eyes'
	vecTargetOrigin = pEntity->EyePosition();

	UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT(pev)/*pentIgnore*/, &tr);

	if (tr.flFraction != 1.0)
	{
		return FALSE;// Line of sight is not established
	}
	else
	{
		return TRUE;// line of sight is valid.
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target vector
//=========================================================
BOOL CBaseEntity :: FVisible ( const Vector &vecOrigin )
{
	TraceResult tr;
	Vector		vecLookerOrigin;

	vecLookerOrigin = EyePosition();//look through the caller's 'eyes'

	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, ENT(pev)/*pentIgnore*/, &tr);

	if (tr.flFraction != 1.0)
	{
		return FALSE;// Line of sight is not established
	}
	else
	{
		return TRUE;// line of sight is valid.
	}
}

/*
================
TraceAttack
================
*/
void CBaseEntity::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4;

	if ( pev->takedamage )
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );

		int blood = BloodColor();

		if ( blood != DONT_BLEED && !( bitsDamageType & DMG_NOBLOOD ) )
		{
			SpawnBlood(vecOrigin, blood, flDamage);// a little surface blood.
			TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		}
	}
}

void CBaseMonster::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir,
	TraceResult* ptr, int bitsDamageType )
{
	if ( !pev->takedamage )
		return;
	m_LastHitGroup = ptr->iHitgroup;
	switch ( ptr->iHitgroup )
	{
	case HITGROUP_HEAD: flDamage *= gSkillData.monHead; break;
	case HITGROUP_CHEST: flDamage *= gSkillData.monChest; break;
	case HITGROUP_STOMACH: flDamage *= gSkillData.monStomach; break;
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM: flDamage *= gSkillData.monArm; break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG: flDamage *= gSkillData.monLeg; break;
	default: break;
	}
	SpawnBlood( ptr->vecEndPos, BloodColor(), flDamage ); // a little surface blood.
	TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

float MapBulletToDmg( int bullet )
{
	float flDmg = 0;

	switch ( bullet )
	{
	case BULLET_PLAYER_COLT:		flDmg = gSkillData.plrDmgCOLT;			break;
	case BULLET_PLAYER_LUGER:		flDmg = gSkillData.plrDmgLUGER;			break;
	case BULLET_PLAYER_GARAND:		flDmg = gSkillData.plrDmgGARAND;		break;
	case BULLET_PLAYER_SCOPEDKAR:	flDmg = gSkillData.plrDmgSCOPEDKAR;		break;
	case BULLET_PLAYER_THOMPSON:	flDmg = gSkillData.plrDmgTHOMPSON;		break;
	case BULLET_PLAYER_M1CARBINE:	flDmg = gSkillData.plrDmgCARBINE;		break;
	case BULLET_PLAYER_MG42:		flDmg = gSkillData.plrDmgMG42;			break;
	case BULLET_PLAYER_MG34:		flDmg = gSkillData.plrDmgMG34;			break;
	case BULLET_PLAYER_30CAL:		flDmg = gSkillData.plrDmg30CAL;			break;
	case BULLET_PLAYER_MP44:		flDmg = gSkillData.plrDmgMP44;			break;
	case BULLET_PLAYER_SPRING:		flDmg = gSkillData.plrDmgSPRING;		break;
	case BULLET_PLAYER_KAR:			flDmg = gSkillData.plrDmgKAR;			break;
	case BULLET_PLAYER_BAR:			flDmg = gSkillData.plrDmgBAR;			break;
	case BULLET_PLAYER_MP40:		flDmg = gSkillData.plrDmgMP40;			break;
	case BULLET_PLAYER_GREASEGUN:	flDmg = gSkillData.plrDmgGREASEGUN;		break;
	case BULLET_PLAYER_FG42:		flDmg = gSkillData.plrDmgFG42;			break;
	case BULLET_PLAYER_K43:			flDmg = gSkillData.plrDmgK43;			break;
	case BULLET_PLAYER_ENFIELD:		flDmg = gSkillData.plrDmgEnfield;		break;
	case BULLET_PLAYER_STEN:		flDmg = gSkillData.plrDmgSten;			break;
	case BULLET_PLAYER_BREN:		flDmg = gSkillData.plrDmgBren;			break;
	case BULLET_PLAYER_WEBLEY:		flDmg = gSkillData.plrDmgWebley;		break;
	case BULLET_NONE:				flDmg = 50;								break;
	default:						flDmg = 0;								break;
	}

	return flDmg;
}

float MapMaterialModFromTexture( char texture )
{
	float flMaterialMod = 1.0;

	switch ( texture )
	{
	case 'E':
		flMaterialMod = 1.3;
		break;
	case 'F':
		flMaterialMod = 1.5;
		break;
	case 'N':
	case 'T':
	case 'W':
	case 'Y':
	case 'Z':
		flMaterialMod = 2.0;
		break;
	default:
		flMaterialMod = 5.5;
		break;
	case 'A':
	case 'C':
	case 'K':
	case 'R':
		flMaterialMod = 7.3;
		break;
	case 'G':
	case 'H':
	case 'M':
		flMaterialMod = 15;
		break;
	}

	flMaterialMod *= 0.9;

	return flMaterialMod;
}

void CBaseEntity::FireTWalls( float flDamage, int iBulletType, entvars_t* pevAttacker,
	Vector vecDir, Vector vecSrc, Vector, Vector, Vector vecDest )
{
	if ( g_pGameRules->RoundState() != RND_RUNNING )
		return;
	(void)ENT( pevAttacker );
	TraceResult tr, trExit;
	UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, edict(), &tr );
	edict_t* pHit = tr.pHit;
	UTIL_TraceLine( tr.vecEndPos + vecDir * 2, vecDest, dont_ignore_monsters, pHit, &trExit );
	CBaseEntity* pEntity = Instance( pHit );
	BOOL fPlayer = pEntity->IsPlayer();
	char cTextureType = 'F';
	if ( !fPlayer )
	{
		edict_t* pWorld = INDEXENT( 0 );
		const char* pTexture = TRACE_TEXTURE( pHit ? pHit : pWorld, vecSrc, vecDest );
		if ( pTexture )
		{
			ALERT( at_aiconsole, "Texture: %s\n", pTexture );
			if ( *pTexture == '+' || *pTexture == '-' )
				pTexture += 2;
			if ( *pTexture == '!' || *pTexture == '{' || *pTexture == '~' || *pTexture == ' ' )
				pTexture++;
			char szTexture[64];
			strcpy( szTexture, pTexture );
			szTexture[12] = 0;
			cTextureType = TEXTURETYPE_Find( szTexture );
		}
		else
		{
			ALERT( at_aiconsole, "Texture: nothing!\n" );
			cTextureType = 'C';
		}
	}
	if ( trExit.fAllSolid )
		return;
	UTIL_TraceLine( trExit.vecEndPos, tr.vecEndPos, dont_ignore_monsters, fPlayer ? pHit : NULL, &trExit );
	float flThickness = ( trExit.vecEndPos - tr.vecEndPos ).Length();
	float flMaterialMod;
	switch ( cTextureType )
	{
	case 'A': case 'G': case 'H': case 'K': case 'M': flMaterialMod = 12; break;
	case 'C': case 'R': flMaterialMod = 7.3f; break;
	case 'E': case 'F': case 'N': case 'T': case 'W': case 'Y': case 'Z': flMaterialMod = 2; break;
	default: flMaterialMod = 5.5f; break;
	}
	ALERT( at_aiconsole, "Entering surface of materialmod %f, thickness %f, with damage %f\n", flMaterialMod, flThickness, flDamage );
	flDamage -= flThickness * flMaterialMod;
	if ( flDamage <= 0 )
	{
		ALERT( at_aiconsole, "Bullet was stopped by wall\n" );
		return;
	}
	ALERT( at_aiconsole, "Exiting surface  - damage %f\n", flDamage );
	DecalGunshot( &trExit, iBulletType );
	UTIL_TraceLine( trExit.vecEndPos, vecDest, dont_ignore_monsters, pHit, &tr );
	DecalGunshot( &tr, iBulletType );
	pEntity = Instance( tr.pHit );
	if ( pEntity && pEntity->pev->takedamage )
	{
		ClearMultiDamage();
		pEntity->TraceAttack( pev, flDamage, vecDir, &tr, DMG_BULLET );
		ApplyMultiDamage( pev, pev );
	}
}

void CBaseEntity::FireBullets( ULONG cShots, Vector vecSrc, Vector vecDirShooting,
	Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t* pevAttacker )
{
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	if ( g_pGameRules->RoundState() != RND_RUNNING )
		return;
	if ( !pevAttacker )
		pevAttacker = pev;  // the default attacker is ourselves
	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;
	static int tracerCount;
	for ( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{
		float x, y;
		do
		{
			x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
			y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		}
		while ( x * x + y * y > 1 );
		Vector vecDir = vecDirShooting + x * vecSpread.x * vecRight + y * vecSpread.y * vecUp;
		Vector vecEnd = vecSrc + vecDir * flDistance;
		TraceResult tr;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr );
		if ( iTracerFreq && tracerCount++ % iTracerFreq == 0 )
		{
			Vector vecTracerSrc = IsPlayer() ? vecSrc + Vector( 0, 0, -4 ) + gpGlobals->v_right * 2 + gpGlobals->v_forward * 16 : vecSrc;
			PLAYBACK_EVENT_FULL( 0, NULL, g_pGameRules->BulletTracers(), 0, vecTracerSrc, tr.vecEndPos, 0, 0, 0, 0, 0, 0 );
		}
		if ( tr.flFraction == 1 )
			continue;
		CBaseEntity* pEntity = Instance( tr.pHit );
		if ( iDamage )
		{
			pEntity->TraceAttack( pevAttacker, (float)iDamage, vecDir, &tr, DMG_BULLET );
			TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
			DecalGunshot( &tr, iBulletType );
		}
		else if ( iBulletType == BULLET_NONE )
		{
			pEntity->TraceAttack( pevAttacker, 50, vecDir, &tr, DMG_CLUB );
			TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
			if ( !FNullEnt( tr.pHit ) && VARS( tr.pHit )->rendermode != kRenderNormal )
				UTIL_DecalTrace( &tr, DECAL_GLASSBREAK1 + RANDOM_LONG( 0, 2 ) );
		}
		else if ( iBulletType >= BULLET_PLAYER_GARAND && iBulletType <= BULLET_PLAYER_WEBLEY )
		{
			pEntity->TraceAttack( pevAttacker, MapBulletToDmg( iBulletType ), vecDir, &tr, DMG_BULLET | DMG_RIFLE );
			float flDamage = MapBulletToDmg( iBulletType );
			FireTWalls( flDamage, iBulletType, pevAttacker, vecDir, vecSrc, vecRight, vecUp, vecEnd );
		}
		else
		{
			float flDamage = gSkillData.plrDmg9MM;
			if ( iBulletType == BULLET_PLAYER_MP5 )
				flDamage = gSkillData.plrDmgMP5;
			else if ( iBulletType == BULLET_PLAYER_COLT || iBulletType == BULLET_PLAYER_LUGER )
				flDamage = MapBulletToDmg( iBulletType );
			pEntity->TraceAttack( pevAttacker, flDamage, vecDir, &tr, DMG_BULLET );
		}
	}
	ApplyMultiDamage( pev, pevAttacker );
}

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4701 6001 )
#endif
Vector CBaseEntity::FireBulletsNC( Vector vecSrc, Vector vecDirShooting, float flSpread, float flDistance, int iBulletType,
	int iTracerFreq, int iDamage, entvars_t *pevAttacker, int shared_rand )
{
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	float x, y, z;

	if ( pevAttacker == NULL )
		pevAttacker = pev;  // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	if ( IsPlayer() )
	{
		// get circular gaussian spread
		x = UTIL_SharedRandomFloat( shared_rand, -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + 1, -0.5, 0.5 );
		y = UTIL_SharedRandomFloat( shared_rand + 2, -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + 3, -0.5, 0.5 );
		z = x * x + y * y;
	}
	else
	{
		do
		{
			x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
			y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
			z = x * x + y * y;
		}
		while ( z > 1 );
	}

	Vector vecDir = vecDirShooting + x * flSpread * vecRight + y * flSpread * vecUp;

	if ( g_pGameRules->RoundState() != 1 )
		return Vector( x * flSpread, y * flSpread, 0.0 );

	Vector vecEnd;
	int max_penetrations = 6;
	int penetrations = 0;
	float flDmg = MapBulletToDmg( iBulletType );
	int bitsDmg = DMG_BULLET;
	char chTextureType = CHAR_TEX_CONCRETE;
	char szbuffer[64];
	bool bPlayerHit;
	TraceResult reverseTr;

	edict_t *pentIgnore = ENT( pev );

	float flMaterialMod;

	CBaseEntity *pPlayer = NULL;
	int iHitgroup = -1;

	Vector origSrc = vecSrc;

	while ( penetrations < max_penetrations )
	{
		vecEnd = vecSrc + vecDir * flDistance;

		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pentIgnore, &tr );

		if ( tr.flFraction >= 1.0 )
			break;

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( penetrations )
		{
			UTIL_TraceLine( tr.vecEndPos, origSrc, dont_ignore_monsters, NULL, &reverseTr );

			if ( reverseTr.fAllSolid )
				break;

			float len = ( reverseTr.vecEndPos - vecSrc ).Length();
			flDmg -= flMaterialMod * len;

			if ( flDmg < 0 )
			{
				penetrations = max_penetrations;
				ALERT( at_aiconsole, "Bullet was stopped\n" );
				break;
			}

			ALERT( at_aiconsole, "Exiting surface  - damage %f\n", flDmg );

			if ( pEntity == pPlayer && iHitgroup == tr.iHitgroup )
			{
				vecSrc = vecSrc + vecDir * 2;
				ALERT( at_aiconsole, "Hit same hitgroup, skipping\n" );
				continue;
			}

			UTIL_GunshotDecalTrace( &tr, 0 );

			CBaseEntity *pReverseHit = CBaseEntity::Instance( reverseTr.pHit );

			if ( !pReverseHit->IsPlayer() )
				UTIL_GunshotDecalTrace( &reverseTr, 0 );
		}

		ALERT( at_aiconsole, "penetration: %d, starting dmg: %f\n", penetrations, flDmg );

		chTextureType = CHAR_TEX_CONCRETE;
		bPlayerHit = false;

		if ( pEntity->IsPlayer() )
		{
			pEntity->TraceAttack( pevAttacker, flDmg, vecDir, &tr, bitsDmg );
			chTextureType = CHAR_TEX_FLESH;
			bPlayerHit = true;
			pPlayer = pEntity;
			iHitgroup = tr.iHitgroup;
		}
		else
		{
			pEntity->TraceAttack( pevAttacker, flDmg, vecDir, &tr, bitsDmg );

			edict_t *pWorld = g_engfuncs.pfnPEntityOfEntIndex( 0 );

			if ( tr.pHit )
				pWorld = tr.pHit;

			const char *pTextureName = TRACE_TEXTURE( pWorld, vecSrc, vecEnd );

			if ( pTextureName )
			{
				// strip leading '-0' or '+0~' or '{' or '!'
				if ( *pTextureName == '-' || *pTextureName == '+' )
					pTextureName += 2;

				if ( *pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ' )
					pTextureName++;

				strcpy( szbuffer, pTextureName );
				szbuffer[CBTEXTURENAMEMAX - 1] = 0;

				// get texture type
				chTextureType = TEXTURETYPE_Find( szbuffer );
			}
		}

		flMaterialMod = MapMaterialModFromTexture( chTextureType );

		if ( iBulletType == BULLET_PLAYER_MG42 || iBulletType == BULLET_PLAYER_MG34 || iBulletType == BULLET_PLAYER_30CAL )
			flMaterialMod *= 0.65;

		vecSrc = tr.vecEndPos + vecDir * 2;

		if ( bPlayerHit && ( tr.iHitgroup == HITGROUP_LEFTARM || tr.iHitgroup == HITGROUP_RIGHTARM ) )
			pentIgnore = NULL;
		else
			pentIgnore = tr.pHit;

		penetrations++;
	}

	ApplyMultiDamage( pev, pevAttacker );

	return Vector( x * flSpread, y * flSpread, 0.0 );
}
#ifdef _MSC_VER
#pragma warning( pop )
#endif

void CBaseEntity::DecalThink()
{
	DecalGunshot( &m_trDecalTrace, BULLET_PLAYER_CROWBAR );
}

void CBaseEntity::RifleMeleeAttack( CBasePlayer *m_pPlayer, int weapon_id )
{
	int i, j, k;
	float distance;
	Vector vecDuckMins( -16, -16, -36 ), vecDuckMaxs( 16, 16, 36 );
	float *minmaxs[2] = { vecDuckMins, vecDuckMaxs };
	TraceResult tmpTrace;

	if ( g_pGameRules->RoundState() != 1 )
		return;

	int fDidHit;
	fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 64;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
			{
				Vector vecHullEnd = tr.vecEndPos;

				distance = 200;

				vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2 );
				UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, m_pPlayer->edict(), &tmpTrace );

				if ( tmpTrace.flFraction > 1.0 )
				{
					for ( i = 0; i < 2; i++ )
					{
						for ( j = 0; j < 2; j++ )
						{
							for ( k = 0; k < 2; k++ )
							{
								vecEnd.x = vecHullEnd.x + minmaxs[i][0];
								vecEnd.y = vecHullEnd.y + minmaxs[j][1];
								vecEnd.z = vecHullEnd.z + minmaxs[k][2];

								UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tmpTrace );
								if ( tmpTrace.flFraction < 1.0 )
								{
									float thisDistance = ( tmpTrace.vecEndPos - vecSrc ).Length();
									if ( thisDistance < distance )
									{
										tr = tmpTrace;
										distance = thisDistance;
									}
								}
							}
						}
					}
				}
			}
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	if ( tr.flFraction >= 1.0 )
		return;

	fDidHit = TRUE;
	CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

	ClearMultiDamage( );

	if ( weapon_id == WEAPON_KAR )
		pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgBayonet, gpGlobals->v_forward, &tr, DMG_CLUB | DMG_BAYONET );
	else if ( weapon_id == WEAPON_GARAND )
		pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgGarandButt, gpGlobals->v_forward, &tr, DMG_CLUB | DMG_GARAND_BUTT );
	else if ( weapon_id == WEAPON_K43 )
		pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgK43Butt, gpGlobals->v_forward, &tr, DMG_CLUB | DMG_K43_BUTT );
	else if ( weapon_id == WEAPON_ENFIELD )
		pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgEnfieldButt, gpGlobals->v_forward, &tr, DMG_CLUB | DMG_ENFIELD_BUTT );

	ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

	float flVol = 1.0;
	int fHitWorld = TRUE;

	if ( pEntity && pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
	{
		switch ( weapon_id )
		{
		case WEAPON_KAR:
		case WEAPON_ENFIELD:
			switch ( RANDOM_LONG( 0, 2 ) )
			{
			case 0:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit1.wav", 1, ATTN_NORM );
				break;
			case 1:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit2.wav", 1, ATTN_NORM );
				break;
			case 2:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit3.wav", 1, ATTN_NORM );
				break;
			}
			break;

		case WEAPON_GARAND:
		case WEAPON_K43:
			if ( pEntity->IsPlayer() )
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_VOICE, "weapons/butt_hit.wav", 1, ATTN_NORM );
			break;

		default:
			break;
		}

		m_pPlayer->m_iWeaponVolume = 128;

		if ( !pEntity->IsAlive() )
			return;
		else
			flVol = 0.1;

		fHitWorld = FALSE;
	}

	// play texture hit sound
	// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line
	if ( fHitWorld )
	{
		float fvolbar = TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + ( vecEnd - vecSrc ) * 2, BULLET_PLAYER_CROWBAR );

		if ( g_pGameRules->IsMultiplayer() )
		{
			// override the volume here, cause we don't play texture sounds in multiplayer,
			// and fvolbar is going to be 0 from the above call.
			fvolbar = 1;
		}

		if ( weapon_id == WEAPON_KAR || weapon_id == WEAPON_ENFIELD )
		{
			SetThink( &CBaseEntity::DecalThink );
			m_trDecalTrace = tr;
			pev->nextthink = gpGlobals->time + 0.1;

			switch ( RANDOM_LONG( 0, 1 ) )
			{
			case 0:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			case 1:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			}
		}
	}

	m_pPlayer->m_iWeaponVolume = flVol * 512;
}

void CBaseEntity :: TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if (BloodColor() == DONT_BLEED)
		return;

	if (flDamage == 0)
		return;

	if (! (bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_MORTAR)))
		return;

	// make blood decal on the wall!
	TraceResult Bloodtr;
	Vector vecTraceDir;
	float flNoise;
	int cCount;
	int i;

/*
	if ( !IsAlive() )
	{
		// dealing with a dead monster. 
		if ( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}
*/

	if (flDamage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (flDamage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	for ( i = 0 ; i < cCount ; i++ )
	{
		vecTraceDir = vecDir * -1;// trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172, ignore_monsters, ENT(pev), &Bloodtr);

		if ( Bloodtr.flFraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}

//=========================================================
//=========================================================
void CBaseMonster :: MakeDamageBloodDecal ( int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir )
{
	// make blood decal on the wall!
	TraceResult Bloodtr;
	Vector vecTraceDir;
	int i;

	if ( !IsAlive() )
	{
		// dealing with a dead monster.
		if ( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return;
		}
		else
		{
			pev->max_health--;
		}
	}

	for ( i = 0 ; i < cCount ; i++ )
	{
		vecTraceDir = vecDir;

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * 172, ignore_monsters, ENT(pev), &Bloodtr);

/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_COORD( ptr->vecEndPos.x );
			WRITE_COORD( ptr->vecEndPos.y );
			WRITE_COORD( ptr->vecEndPos.z );
			
			WRITE_COORD( Bloodtr.vecEndPos.x );
			WRITE_COORD( Bloodtr.vecEndPos.y );
			WRITE_COORD( Bloodtr.vecEndPos.z );
		MESSAGE_END();
*/

		if ( Bloodtr.flFraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}
