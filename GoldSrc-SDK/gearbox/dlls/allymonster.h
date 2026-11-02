#pragma once

#include "talkmonster.h"

class COFAllyMonster : public CBaseMonster
{
public:
	void TalkInit();
	CBaseEntity* FindNearestFriend( BOOL fPlayer );
	float TargetDistance();
	void StopTalking() { SentenceStop(); }
	void Precache();
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void Touch( CBaseEntity* pOther );
	void Killed( entvars_t* pevAttacker, int iGib );
	int IRelationship( CBaseEntity* pTarget );
	int CanPlaySentence( BOOL fDisregardState );
	void PlaySentence( const char* pszSentence, float duration, float volume, float attenuation );
	void PlayScriptedSentence( const char* pszSentence, float duration, float volume, float attenuation,
		BOOL bConcurrent, CBaseEntity* pListener );
	void KeyValue( KeyValueData* pkvd );
	Schedule_t* GetScheduleOfType( int Type );
	void StartTask( Task_t* pTask );
	void RunTask( Task_t* pTask );
	void HandleAnimEvent( MonsterEvent_t* pEvent );
	void PrescheduleThink();
	int GetVoicePitch();
	void IdleRespond();
	int FIdleSpeak();
	int FIdleStare();
	int FIdleHello();
	void IdleHeadTurn( Vector& vecFriend );
	int FOkToSpeak();
	void TrySmellTalk();
	CBaseEntity* EnumFriends( CBaseEntity* pentPrevious, int listNumber, BOOL bTrace );
	void AlertFriends();
	void ShutUpFriends();
	BOOL IsTalking();
	void Talk( float flDuration );
	BOOL CanFollow();
	BOOL IsFollowing() { return m_hTargetEnt != NULL && m_hTargetEnt->IsPlayer(); }
	void StopFollowing( BOOL clearSchedule );
	void StartFollowing( CBaseEntity* pLeader );
	virtual void DeclineFollowing() {}
	void LimitFollowers( CBaseEntity* pPlayer, int maxFollowers );
	void EXPORT FollowerUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	virtual void SetAnswerQuestion( COFAllyMonster* pSpeaker );
	virtual int FriendNumber( int arrayNumber ) { return arrayNumber; }
	int Save( CSave& save );
	int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];
	static char* m_szFriends[TLK_CFRIENDS];
	static float g_talkWaitTime;

	int			m_bitsSaid;
	int			m_nSpeak;
	int			m_voicePitch;
	const char*	m_szGrp[TLK_CGROUPS];
	float		m_useTime;
	int			m_iszUse;
	int			m_iszUnUse;
	float		m_flLastSaidSmelled;
	float		m_flStopTalkTime;
	EHANDLE		m_hTalkTarget;

	CUSTOM_SCHEDULES;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
