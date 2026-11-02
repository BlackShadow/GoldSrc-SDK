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

#ifndef FUNC_BREAK_H
#define FUNC_BREAK_H

typedef enum { expRandom, expDirected } Explosions;
typedef enum
{
	matGlass, matWood, matMetal, matFlesh, matCinderBlock, matCeilingTile,
	matComputer, matUnbreakableGlass, matRocks, matNone, matLastMaterial
} Materials;

#define NUM_SHARDS 6 // this many shards spawned when breakable objects break;

class CBreakable : public CBaseDelay
{
public:
	void RoundRespawn() override;
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* pkvd ) override;
	void EXPORT BreakTouch( CBaseEntity* pOther );
	void EXPORT BreakUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void RespawnUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT RespawnThink();
	void EXPORT RespawnFadeThink();
	void DamageSound();
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType ) override;
	void TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType ) override;
	void Killed( entvars_t* pevAttacker, int iGib ) override;
	BOOL IsBreakable();
	BOOL SparkWhenHit();
	int DamageDecal( int bitsDamageType ) override;
	void EXPORT Die();
	void EXPORT ObjectDie();
	STATE GetState() override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	void area_SetIndex( int index ) override;
	void area_SendStatus( CBasePlayer* pPlayer ) override;
	BOOL Explodable() { return ExplosionMagnitude() > 0; }
	int ExplosionMagnitude() { return pev->impulse; }
	void ExplosionSetMagnitude( int magnitude ) { pev->impulse = magnitude; }
	static void MaterialSoundPrecache( Materials material );
	static void MaterialSoundRandom( edict_t* pEdict, Materials material, float volume );
	static const char** MaterialSoundList( Materials material, int& soundCount );
	static const char* pSoundsWood[];
	static const char* pSoundsFlesh[];
	static const char* pSoundsGlass[];
	static const char* pSoundsMetal[];
	static const char* pSoundsConcrete[];
	static const char* pSpawnObjects[];
	static TYPEDESCRIPTION m_SaveData[];

	Materials	m_Material;
	Explosions	m_Explosion;
	int			m_idShard;
	float		m_angle;
	int			m_iszGibModel;
	int			m_iszSpawnObject;
	int			m_iRespawnTime;
	int			m_iInitialHealth;
	int			m_iInitialRenderAmt;
	int			m_iInitialRenderMode;
	char		sz_Group[256];
	char		sz_TntTrigger[256];
	float		m_tntdelay;
	int			m_iAreaIndex;
	char		sz_HudIcon[256];
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

#endif
