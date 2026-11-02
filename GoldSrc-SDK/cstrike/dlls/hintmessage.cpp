#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "hintmessage.h"

CHintMessage::CHintMessage( const char* hintString, bool isHint, CUtlVector<const char*>* args, float duration )
{
	m_hintString = hintString;
	m_duration = duration;
	m_isHint = isHint;
	if ( args )
	{
		for ( int i = 0; i < args->Count(); i++ )
		{
			const char* argument = ( *args )[i];
			char* copy;
			if ( argument )
			{
				copy = new char[strlen( argument ) + 1];
				strcpy( copy, argument );
			}
			else
			{
				copy = new char[1];
				copy[0] = 0;
			}
			m_args.AddToTail( copy );
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
CHintMessage::~CHintMessage()
{
	for ( int i = 0; i < m_args.Count(); i++ )
		delete[] m_args[i];
	m_args.RemoveAll();
}

void CHintMessage::Send( CBaseEntity& client )
{
	UTIL_ShowMessageArgs( m_hintString, &client, &m_args, m_isHint );
}

//--------------------------------------------------------------------------------------------------------------
void CHintMessageQueue::Reset()
{
	m_tmMessageEnd = 0;
	for ( int i = 0; i < m_messages.Count(); i++ )
		delete m_messages[i];
	m_messages.RemoveAll();
}

void CHintMessageQueue::Update( CBaseEntity& client )
{
	if ( gpGlobals->time > m_tmMessageEnd && m_messages.Count() )
	{
		CHintMessage* message = m_messages[0];
		m_tmMessageEnd = gpGlobals->time + message->GetDuration();
		message->Send( client );
		delete message;
		m_messages.Remove( 0 );
	}
}

bool CHintMessageQueue::AddMessage( const char* message, float duration, bool isHint, CUtlVector<const char*>* args )
{
	CHintMessage* hint = new CHintMessage( message, isHint, args, duration );
	m_messages.AddToTail( hint );
	return true;
}
