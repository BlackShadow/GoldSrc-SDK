#ifndef TUTOR_BASE_TUTOR_H
#define TUTOR_BASE_TUTOR_H

#include "GameEvent.h"
#include "vector.h"

class CBaseEntity;
class CBasePlayer;
class CBaseTutorStateSystem;
class TutorMessageEvent;
struct TutorMessage;

enum TutorMessageClass
{
	TUTORMESSAGECLASS_NORMAL,
	TUTORMESSAGECLASS_EXAMINE
};

enum TutorMessageType
{
	TUTORMESSAGETYPE_DEFAULT		= 1,
	TUTORMESSAGETYPE_FRIEND_DEATH	= 2,
	TUTORMESSAGETYPE_ENEMY_DEATH	= 4,
	TUTORMESSAGETYPE_SCENARIO		= 8,
	TUTORMESSAGETYPE_BUY			= 16,
	TUTORMESSAGETYPE_CAREER		= 32,
	TUTORMESSAGETYPE_HINT			= 64,
	TUTORMESSAGETYPE_INGAME_HINT	= 128,
	TUTORMESSAGETYPE_END_GAME		= 256,
	TUTORMESSAGETYPE_LAST			= 257,
	TUTORMESSAGETYPE_ALL			= 511
};

enum TutorMessageInterruptFlag
{
	TUTORMESSAGEINTERRUPTFLAG_DEFAULT,
	TUTORMESSAGEINTERRUPTFLAG_NOW_DAMMIT
};

enum TutorMessageKeepOldType
{
	TUTORMESSAGEKEEPOLDTYPE_DONT_KEEP_OLD,
	TUTORMESSAGEKEEPOLDTYPE_KEEP_OLD,
	TUTORMESSAGEKEEPOLDTYPE_UPDATE_CONTENT
};

struct TutorMessage
{
	char*					m_text;
	unsigned char			m_priority;
	unsigned char			m_duration;
	TutorMessageKeepOldType	m_keepOld;
	TutorMessageClass		m_class;
	unsigned char			m_decay;
	TutorMessageType			m_type;
	int						m_lifetime;
	TutorMessageInterruptFlag m_interruptFlag;
	int						m_duplicateID;
	float					m_examineStartTime;
	int						m_timesShown;
	float					m_minDisplayTimeOverride;
	float					m_minRepeatInterval;
	float					m_lastCloseTime;
};

struct TutorMessageEventParam
{
	char*					m_data;
	TutorMessageEventParam*	m_next;
};

class TutorMessageEvent
{
public:
	TutorMessageEvent( int mid, int duplicateID, float time, float lifetime, int priority );
	virtual ~TutorMessageEvent();
	bool IsActive( float time );
	int GetPriority();
	float GetTimeActive( float time );
	void SetActivationTime( float time );
	int GetID();
	int GetDuplicateID();
	void AddParameter( char* str );
	char* GetNextParameter( char* buf, int buflen );
	int GetNumParameters();
	void SetNext( TutorMessageEvent* next );
	TutorMessageEvent* GetNext();

protected:
	int m_messageID;
	int m_duplicateID;
	float m_activationTime;
	float m_lifetime;
	int m_priority;
	int m_numParameters;
	TutorMessageEventParam* m_paramList;
	TutorMessageEvent* m_next;
};

class CBaseTutor
{
public:
	CBaseTutor();
	virtual ~CBaseTutor();

	void StartFrame( float time );
	virtual void TutorThink( float time ) = 0;
	void OnEvent( GameEventType event, CBaseEntity* entity = NULL, CBaseEntity* other = NULL );
	void ShotFired( Vector source, Vector target );
	virtual void PurgeMessages() = 0;

protected:
	void DisplayMessageToPlayer( CBasePlayer* player, int id, const char* text, TutorMessageEvent* event );
	void DrawLineToEntity( CBasePlayer* player, int entindex, int id );
	void DisplayNewStateDescriptionToPlayer();
	void CloseCurrentWindow();
	virtual void CallEventHandler( GameEventType event, CBaseEntity* entity, CBaseEntity* other ) = 0;
	void CheckForStateTransition( GameEventType event, CBaseEntity* entity, CBaseEntity* other );
	virtual void ShowTutorMessage( TutorMessageEvent* event ) = 0;
	void CalculatePathForObjective( CBaseEntity* player );
	virtual bool IsEntityInViewOfPlayer( CBaseEntity* entity, CBasePlayer* player );
	virtual bool IsBombsiteInViewOfPlayer( CBaseEntity* entity, CBasePlayer* player );
	virtual bool IsEntityInBombsite( CBaseEntity* bombsite, CBaseEntity* entity );
	virtual bool IsPlayerLookingAtPosition( Vector* origin, CBasePlayer* player );
	virtual bool IsPlayerLookingAtEntity( CBaseEntity* entity, CBasePlayer* player );
	virtual void HandleShotFired( Vector source, Vector target ) = 0;
	virtual TutorMessage* GetTutorMessageDefinition( int id ) = 0;
	bool DoMessagesHaveSameID( int id1, int id2 );

	CBaseTutorStateSystem* m_stateSystem;
	TutorMessageEvent* m_eventList;
	float m_deadAirStartTime;
	float m_roundStartTime;
};

#endif
