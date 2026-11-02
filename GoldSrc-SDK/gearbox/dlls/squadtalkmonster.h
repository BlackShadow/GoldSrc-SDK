#pragma once

#include "allymonster.h"
#include "squadmonster.h"

class COFSquadTalkMonster : public COFAllyMonster
{
public:
	EHANDLE	m_hSquadLeader;
	EHANDLE	m_hSquadMember[MAX_SQUAD_MEMBERS - 1];
	int		m_afSquadSlots;
	float	m_flLastEnemySightTime;
	BOOL	m_fEnemyEluded;
	EHANDLE	m_hWaitMedic;
	float	m_flMedicWaitTime;
	float	m_flLastHitByPlayer;
	int		m_iPlayerHits;
	int		m_iMySlot;

	int CheckEnemy( CBaseEntity* pEnemy );
	void StartMonster();
	void VacateSlot();
	void ScheduleChange();
	void Killed( entvars_t* pevAttacker, int iGib );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	BOOL OccupySlot( int iDesiredSlots );
	BOOL NoFriendlyFire();
	COFSquadTalkMonster* MySquadLeader()
	{
		COFSquadTalkMonster* pLeader = (COFSquadTalkMonster*)(CBaseEntity*)m_hSquadLeader;
		return pLeader != NULL ? pLeader : this;
	}
	COFSquadTalkMonster* MySquadMember( int i )
	{
		return i >= MAX_SQUAD_MEMBERS - 1 ? this : (COFSquadTalkMonster*)(CBaseEntity*)m_hSquadMember[i];
	}
	int InSquad() { return m_hSquadLeader != NULL; }
	int IsLeader() { return m_hSquadLeader == this; }
	int SquadRecruit( int searchRadius, int maxMembers );
	int SquadCount();
	void SquadRemove( COFSquadTalkMonster* pRemove );
	BOOL SquadAdd( COFSquadTalkMonster* pAdd );
	void SquadMakeEnemy( CBaseEntity* pEnemy );
	void SquadPasteEnemyInfo();
	void SquadCopyEnemyInfo();
	BOOL SquadEnemySplit();
	BOOL SquadMemberInRange( const Vector& vecLocation, float flDist );
	COFSquadTalkMonster* MySquadTalkMonsterPointer() { return this; }
	static TYPEDESCRIPTION m_SaveData[];
	int Save( CSave& save );
	int Restore( CRestore& restore );
	BOOL FValidateCover( const Vector& vecCoverLocation );
	MONSTERSTATE GetIdealState();
	Schedule_t* GetScheduleOfType( int iType );
	void EXPORT FollowerUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void SquadStartFollowing( CBaseEntity* pLeader );
	void SquadStopFollowing( BOOL clearSchedule );
	void LimitSquadFollowers( CBaseEntity* pPlayer, int maxFollowers );
	COFSquadTalkMonster* MySquadMedic();
	COFSquadTalkMonster* FindSquadMedic( int searchRadius );
	virtual BOOL HealMe( COFSquadTalkMonster* pTarget ) { return FALSE; }
	static char* m_szSquadFriends[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
