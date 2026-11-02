#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "bot_util.h"
#include "tutor.h"
#include "tutor_base_states.h"

extern int gmsgTutorText;
extern int gmsgTutorLine;
extern int gmsgTutorState;
extern int gmsgTutorClose;

TutorMessageEvent::TutorMessageEvent( int mid, int duplicateID, float time, float lifetime, int priority )
{
	m_messageID = mid;
	m_activationTime = time;
	m_lifetime = lifetime;
	m_paramList = NULL;
	m_duplicateID = duplicateID;
	m_numParameters = 0;
	m_next = NULL;
	m_priority = priority;
}

TutorMessageEvent::~TutorMessageEvent()
{
}

bool TutorMessageEvent::IsActive( float time )
{
	return time <= m_lifetime + m_activationTime;
}

int TutorMessageEvent::GetPriority()
{
	return m_priority;
}

float TutorMessageEvent::GetTimeActive( float time )
{
	return time - m_activationTime;
}

void TutorMessageEvent::SetActivationTime( float time )
{
	m_activationTime = time;
}

int TutorMessageEvent::GetID()
{
	return m_messageID;
}

int TutorMessageEvent::GetDuplicateID()
{
	return m_duplicateID;
}

void TutorMessageEvent::AddParameter( char* str )
{
	if ( str == NULL )
		return;
	TutorMessageEventParam* parameter = new TutorMessageEventParam;
	parameter->m_next = NULL;
	parameter->m_data = new char[strlen( str ) + 1];
	if ( parameter->m_data != NULL )
	{
		strcpy( parameter->m_data, str );
		parameter->m_data[strlen( str )] = '\0';
		m_numParameters++;
		if ( m_paramList != NULL )
		{
			TutorMessageEventParam* tail = m_paramList;
			while ( tail->m_next != NULL )
				tail = tail->m_next;
			tail->m_next = parameter;
		}
		else
		{
			m_paramList = parameter;
		}
	}
}

char* TutorMessageEvent::GetNextParameter( char* buf, int buflen )
{
	if ( m_paramList == NULL )
		return NULL;
	TutorMessageEventParam* parameter = m_paramList;
	m_numParameters--;
	m_paramList = parameter->m_next;
	strncpy( buf, parameter->m_data, buflen );
	delete parameter;
	return buf;
}

int TutorMessageEvent::GetNumParameters()
{
	return m_numParameters;
}

void TutorMessageEvent::SetNext( TutorMessageEvent* next )
{
	m_next = next;
}

TutorMessageEvent* TutorMessageEvent::GetNext()
{
	return m_next;
}

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CBaseTutor::CBaseTutor()
{
	m_deadAirStartTime = 0;
	m_roundStartTime = 0;
	m_eventList = NULL;
}

CBaseTutor::~CBaseTutor()
{
	TutorMessageEvent* event = m_eventList;
	while ( event != NULL )
	{
		TutorMessageEvent* next = event->GetNext();
		delete event;
		event = next;
	}
}

void CBaseTutor::StartFrame( float time )
{
	TutorThink( time );
}

void CBaseTutor::OnEvent( GameEventType event, CBaseEntity* entity, CBaseEntity* other )
{
	CallEventHandler( event, entity, other );
	CheckForStateTransition( event, entity, other );
}

void CBaseTutor::ShotFired( Vector source, Vector target )
{
	HandleShotFired( source, target );
}

void CBaseTutor::DisplayMessageToPlayer( CBasePlayer* player, int id, const char* text, TutorMessageEvent* event )
{
	TutorMessage* definition = GetTutorMessageDefinition( event->GetID() );
	MESSAGE_BEGIN( MSG_ONE, gmsgTutorText, NULL, player->edict() );
	WRITE_STRING( text );
	int numArgs = (unsigned char)event->GetNumParameters();
	WRITE_BYTE( numArgs );
	for ( int i = 0; i < numArgs; i++ )
	{
		char parameter[512];
		char* value = event->GetNextParameter( parameter, sizeof( parameter ) );
		WRITE_STRING( value != NULL ? value : "" );
	}
	WRITE_SHORT( (short)id );
	WRITE_SHORT( !player->IsAlive() );
	WRITE_SHORT( definition != NULL ? definition->m_type : TUTORMESSAGETYPE_DEFAULT );
	MESSAGE_END();
	m_deadAirStartTime = -1.0f;
	if ( definition != NULL )
	{
		if ( gpGlobals->time - m_roundStartTime > 1.0f )
		{
			if ( definition->m_type == TUTORMESSAGETYPE_FRIEND_DEATH )
				EMIT_SOUND_DYN( player->edict(), CHAN_ITEM, "events/friend_died.wav", VOL_NORM, ATTN_NORM, 0, 120 );
			else if ( definition->m_type == TUTORMESSAGETYPE_ENEMY_DEATH )
				EMIT_SOUND_DYN( player->edict(), CHAN_ITEM, "events/enemy_died.wav", VOL_NORM, ATTN_NORM, 0, 85 );
			else
				EMIT_SOUND_DYN( player->edict(), CHAN_ITEM, "events/tutor_msg.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
		}
		if ( definition->m_decay != 0 )
			g_engfuncs.pfnRegisterTutorMessageShown( id );
	}
}

void CBaseTutor::DrawLineToEntity( CBasePlayer* player, int entindex, int id )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgTutorLine, NULL, player->edict() );
	WRITE_SHORT( entindex );
	WRITE_SHORT( (short)id );
	MESSAGE_END();
}

void CBaseTutor::DisplayNewStateDescriptionToPlayer()
{
	CBasePlayer* player = UTIL_GetLocalPlayer();
	if ( player != NULL && m_stateSystem->GetCurrentStateString() == NULL )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgTutorState, NULL, player->edict() );
		WRITE_STRING( NULL );
		MESSAGE_END();
	}
}

void CBaseTutor::CloseCurrentWindow()
{
	CBasePlayer* player = UTIL_GetLocalPlayer();
	if ( player != NULL )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgTutorClose, NULL, player->edict() );
		MESSAGE_END();
		m_deadAirStartTime = gpGlobals->time;
	}
}

void CBaseTutor::CheckForStateTransition( GameEventType event, CBaseEntity* entity, CBaseEntity* other )
{
	if ( m_stateSystem->UpdateState( event, entity, other ) )
		DisplayNewStateDescriptionToPlayer();
}

void CBaseTutor::CalculatePathForObjective( CBaseEntity* )
{
}

bool CBaseTutor::IsEntityInViewOfPlayer( CBaseEntity* entity, CBasePlayer* player )
{
	if ( player == NULL || entity == NULL )
		return false;
	if ( (entity->pev->origin - player->pev->origin).Length() > cv_tutor_view_distance.value )
		return false;
	if ( !player->FInViewCone( entity ) )
		return false;
	Vector eye = player->pev->origin + player->pev->view_ofs;
	TraceResult trace;
	UTIL_TraceLine( eye, entity->pev->origin, ignore_monsters, ignore_glass, player->edict(), &trace );
	return trace.flFraction == 1.0f;
}

bool CBaseTutor::IsBombsiteInViewOfPlayer( CBaseEntity* entity, CBasePlayer* player )
{
	if ( player == NULL || entity == NULL )
		return false;
	Vector center = (entity->pev->absmax + entity->pev->absmin) * 0.5f;
	if ( (center - player->pev->origin).Length() <= cv_tutor_view_distance.value && player->FInViewCone( &center ) )
	{
		Vector eye = player->pev->origin + player->pev->view_ofs;
		TraceResult trace;
		UTIL_TraceLine( eye, center, ignore_monsters, ignore_glass, player->edict(), &trace );
		return trace.flFraction == 1.0f;
	}
	return false;
}

bool CBaseTutor::IsEntityInBombsite( CBaseEntity* bombsite, CBaseEntity* entity )
{
	if ( bombsite == NULL || entity == NULL )
		return false;
	const Vector& origin = entity->pev->origin;
	return origin.x <= bombsite->pev->absmax.x && origin.y <= bombsite->pev->absmax.y &&
		origin.z <= bombsite->pev->absmax.z && bombsite->pev->absmin.x <= origin.x &&
		bombsite->pev->absmin.y <= origin.y && bombsite->pev->absmin.z <= origin.z;
}

bool CBaseTutor::IsPlayerLookingAtPosition( Vector* origin, CBasePlayer* player )
{
	if ( origin == NULL || player == NULL )
		return false;
	if ( (*origin - player->pev->origin).Length() > cv_tutor_look_distance.value )
		return false;
	if ( !player->IsLookingAtPosition( origin, cv_tutor_look_angle.value ) )
		return false;
	Vector eye = player->pev->origin + player->pev->view_ofs;
	TraceResult trace;
	UTIL_TraceLine( eye, *origin, ignore_monsters, ignore_glass, player->edict(), &trace );
	return trace.flFraction == 1.0f;
}

bool CBaseTutor::IsPlayerLookingAtEntity( CBaseEntity* entity, CBasePlayer* player )
{
	if ( player == NULL || entity == NULL )
		return false;
	Vector source = player->pev->origin + player->pev->view_ofs;
	UTIL_MakeVectors( player->pev->v_angle );
	Vector target = gpGlobals->v_forward * cv_tutor_look_distance.value + source;
	TraceResult trace;
	UTIL_TraceLine( source, target, dont_ignore_monsters, ignore_glass, player->edict(), &trace );
	if ( trace.pHit != NULL && ENTINDEX( trace.pHit ) != 0 )
		return CBaseEntity::Instance( trace.pHit ) == entity;
	return false;
}

bool CBaseTutor::DoMessagesHaveSameID( int id1, int id2 )
{
	if ( id1 == id2 )
		return true;
	TutorMessage* message1 = GetTutorMessageDefinition( id1 );
	TutorMessage* message2 = GetTutorMessageDefinition( id2 );
	if ( message1 == NULL || message2 == NULL )
		return false;
	return message1->m_duplicateID != 0 && message1->m_duplicateID == message2->m_duplicateID;
}
