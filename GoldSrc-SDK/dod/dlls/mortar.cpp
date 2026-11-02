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

===== mortar.cpp ========================================================

  the "LaBuznik" mortar device              

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "weapons.h"
#include "soundent.h"

class CFuncMortarField : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* pkvd ) override;
	int ObjectCaps() override { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	void EXPORT FieldUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );

	int		m_iszXController;
	int		m_iszYController;
	float	m_flSpread;
	float	m_flDelay;
	int		m_iCount;
	int		m_fControl;
	int		m_iDamage;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( func_mortar_field, CFuncMortarField );

TYPEDESCRIPTION CFuncMortarField::m_SaveData[] =
{
	DEFINE_FIELD( CFuncMortarField, m_iszXController, FIELD_STRING ),
	DEFINE_FIELD( CFuncMortarField, m_iszYController, FIELD_STRING ),
	DEFINE_FIELD( CFuncMortarField, m_flSpread, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMortarField, m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMortarField, m_iCount, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncMortarField, m_fControl, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFuncMortarField, CBaseToggle );

void CFuncMortarField::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "m_iszXController" ) )
	{
		m_iszXController = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iszYController" ) )
	{
		m_iszYController = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_flSpread" ) )
	{
		m_flSpread = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_fControl" ) )
	{
		m_fControl = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iCount" ) )
	{
		m_iCount = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iDamage" ) )
	{
		m_iDamage = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
}

void CFuncMortarField::Spawn()
{
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), STRING( pev->model ) );    // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SetUse( &CFuncMortarField::FieldUse );
	Precache();
}

void CFuncMortarField::Precache()
{
	PRECACHE_SOUND( "weapons/mortar.wav" );
	PRECACHE_SOUND( "weapons/mortarhit.wav" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
}


// If connected to a table, then use the table controllers, else hit where the trigger is.
void CFuncMortarField::FieldUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	(void)pCaller;
	(void)useType;
	(void)value;
	Vector vecStart;
	vecStart.x = RANDOM_FLOAT( pev->mins.x, pev->maxs.x );
	vecStart.y = RANDOM_FLOAT( pev->mins.y, pev->maxs.y );
	vecStart.z = pev->maxs.z;
	if ( m_fControl == 1 && pActivator )
	{
		vecStart.x = pActivator->pev->origin.x;
		vecStart.y = pActivator->pev->origin.y;
	}
	else if ( m_fControl == 2 )
	{
		if ( m_iszXController )
		{
			CBaseEntity* pController = UTIL_FindEntityByTargetname( NULL, STRING( m_iszXController ) );
			if ( pController )
				vecStart.x = pev->mins.x + pController->pev->ideal_yaw * pev->size.x;
		}
		if ( m_iszYController )
		{
			CBaseEntity* pController = UTIL_FindEntityByTargetname( NULL, STRING( m_iszYController ) );
			if ( pController )
				vecStart.y = pev->mins.y + pController->pev->ideal_yaw * pev->size.y;
		}
	}
	int pitch = RANDOM_LONG( 95, 124 );
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/mortar.wav", 1, ATTN_NONE, 0, pitch );
	float t = 4;
	for ( int i = 0; i < m_iCount; ++i )
	{
		Vector vecSpot = vecStart;
		vecSpot.x += RANDOM_FLOAT( -m_flSpread, m_flSpread );
		vecSpot.y += RANDOM_FLOAT( -m_flSpread, m_flSpread );
		TraceResult tr;
		UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -4096 ), ignore_monsters, edict(), &tr );
		edict_t* pOwner = pActivator ? pActivator->edict() : NULL;
		CBaseEntity* pMortar = Create( "monster_mortar", tr.vecEndPos, g_vecZero, pOwner );
		pMortar->pev->nextthink = gpGlobals->time + t;
		t += RANDOM_FLOAT( 0.2f, 0.5f );
		pMortar->pev->dmg = (float)m_iDamage;
		if ( i == 0 )
			CSoundEnt::InsertSound( bits_SOUND_DANGER, tr.vecEndPos, 400, 0.3f );
	}
}

class CMortar : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT MortarExplode();

	int m_spriteTexture;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_mortar, CMortar );

void CMortar::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->dmg = 200;
	SetThink( &CMortar::MortarExplode );
	pev->nextthink = 0;
	Precache();
}

void CMortar::Precache()
{
	m_spriteTexture = PRECACHE_MODEL( "sprites/lgtning.spr" );
}

void CMortar::MortarExplode()
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin + Vector( 0, 0, 1024 ), pev->origin - Vector( 0, 0, 1024 ),
		dont_ignore_monsters, edict(), &tr );
	Explode( &tr, DMG_BLAST | DMG_MORTAR );
	UTIL_ScreenShake( tr.vecEndPos, 25, 150, 1, 750 );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}
