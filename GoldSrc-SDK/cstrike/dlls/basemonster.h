/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#ifndef BASEMONSTER_H
#define BASEMONSTER_H

#include "activity.h"

//
// generic Monster
//
class CBaseMonster : public CBaseToggle
{
public:
	Activity m_Activity; // what the monster is doing (animation)
	Activity m_IdealActivity; // monster should switch to this activity
	int m_LastHitGroup; // the last body region that took damage
	int m_bitsDamageType;	// what types of damage has monster (player) taken
	BYTE m_rgbTimeBasedDamage[CDMG_TIMEBASED];
	MONSTERSTATE m_MonsterState; // monster's current state
	MONSTERSTATE m_IdealMonsterState; // monster should change to this state
	int m_afConditions;
	int m_afMemory;
	float m_flNextAttack;		// cannot attack again until this time



	// these fields have been added in the process of reworking the state machine. (sjb)
	EHANDLE m_hEnemy;		 // the entity that the monster is fighting.
	EHANDLE m_hTargetEnt;	 // the entity that the monster is trying to reach
	float m_flFieldOfView;
	int m_bloodColor;
	Vector m_HackedGunPos;	// HACK until we can query end of gun
	Vector m_vecEnemyLKP;

	void KeyValue( KeyValueData *pkvd );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	int TakeHealth( float flHealth, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	int BloodColor() { return m_bloodColor; }
	BOOL IsAlive() { return pev->deadflag != DEAD_DEAD; }

// Basic Monster AI functions
	virtual float ChangeYaw( int speed );
	virtual BOOL HasHumanGibs();
	virtual BOOL HasAlienGibs();
	virtual void FadeMonster();
	virtual void GibMonster();
	virtual Activity GetDeathActivity();
	virtual void BecomeDead();
	virtual BOOL ShouldFadeOnDeath();
	virtual int IRelationship( CBaseEntity *pTarget );
	virtual void PainSound() {}
	virtual void ResetMaxSpeed() {}
	virtual void ReportAIState();
	virtual void MonsterInitDead();
	virtual void Look( int iDistance );
	virtual CBaseEntity *BestVisibleEnemy();
	virtual BOOL FInViewCone( CBaseEntity *pEntity ); // see if pEntity is in monster's view cone
	virtual BOOL FInViewCone( Vector *pOrigin ); // see if given location is in monster's view cone

	float DamageForce( float damage );
	void StopAnimation() { pev->framerate = 0; }
	void BloodSplat( Vector &vecSrc, Vector &vecDir, int iHitLocation, int iVelocity );
	void RadiusDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType );
	void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage,
		int iClassIgnore, int bitsDamageType );
	void CallGibMonster();
	int DeadTakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	BOOL ShouldGibMonster( int iGib );
	CBaseEntity *CheckTraceHullAttack( float flDist, int iDamage, int iDmgType );
	void MakeDamageBloodDecal( int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir );
	Activity GetSmallFlinchActivity();
	void MakeIdealYaw( Vector vecTarget );
	void EXPORT CorpseFallThink();
	BOOL FCheckAITrigger();
	void ClearConditions( int conditions ) { m_afConditions &= ~conditions; }
	void SetConditions( int conditions ) { m_afConditions |= conditions; }
	BOOL HasConditions( int conditions ) { return (m_afConditions & conditions) != 0; }
	void Remember( int memory ) { m_afMemory |= memory; }
	BOOL HasMemory( int memory ) { return (m_afMemory & memory) != 0; }
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

#endif
