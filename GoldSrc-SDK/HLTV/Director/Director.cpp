#include "Director.h"

#include "HLTV/common/DirectorCmd.h"
#include "HLTV/common/IProxy.h"
#include "HLTV/common/IWorld.h"
#include "HLTV/common/common.h"
#include "HLTV/common/mathlib.h"
#include "HLTV/common/random.h"
#include "HLTV/Core/BSPModel.h"
#include "common/IBaseSystem.h"
#include "common/TokenLine.h"
#include "common/hltv.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
Director::Director()
{
}

Director::~Director()
{
}

bool Director::Init( IBaseSystem* system, int serial, char* name )
{
	BaseSystemModule::Init( system, serial, name );
	m_System->RegisterCommand( "slowmotion", this, CMD_ID_SLOWMOTION );

	historyLength = MAX_DIRECTOR_HISTORY;
	history = (worldHistory_t*)malloc( historyLength * sizeof( worldHistory_t ) );
	if ( !history )
	{
		m_System->Printf( "ERROR!Director::Init: not enough memory for world history.\n" );
		return false;
	}

	m_LastExecTime = 0.0f;
	currentTime = 0.0f;
	m_slowMotion = 0.5f;
	for ( int i = 0; i < MAX_DIRECTOR_HISTORY; i++ )
	{
#ifdef _WIN32
		float exponent = (float)( i * i ) / 10000.0f;
#else
		double exponent = (double)( i * i ) / 10000.0;
#endif
		gaussFilter[i] = (float)( 1.0 / exp( (double)exponent ) );
	}

	m_World = NULL;
	active = false;
	m_System->ExecuteFile( "director.cfg" );
	m_System->Printf( "Director module initialized.\n" );
	return true;
}

void Director::ShutDown()
{
	if ( m_State == 4 )
		return;

	if ( history )
		free( history );
	if ( m_World )
		m_World->RemoveListener( this );

	ClearDirectorCommands();
	BaseSystemModule::ShutDown();
	m_System->Printf( "Director module shutdown.\n" );
}

void Director::RunFrame( double time )
{
	BaseSystemModule::RunFrame( time );
	if ( !m_World || !m_World->IsActive() || !nextCutTime || !active )
		return;

	ExecuteDirectorCommands();
	if ( currentTime - nextCutTime > 10.0f )
	{
		float duration = AddBestMODCut();
		if ( !duration )
			duration = AddBestGenericCut();

		nextCutTime += duration;
		while ( history[nextCutSeqnr % historyLength].time < nextCutTime )
			nextCutSeqnr++;
	}
}

void Director::ReceiveSignal( ISystemModule* module, unsigned int signal, void* )
{
	if ( module->GetSerial() != m_World->GetSerial() || signal != 3 )
		return;

	if ( !m_World )
	{
		m_System->Printf( "Director::ReceiveSignal: world == NULL\n" );
		return;
	}

	frame_t frame;
	m_World->GetUncompressedFrame( m_World->GetLastFrame()->seqnr, &frame );
	AnalyseFrame( &frame );
}

char* Director::GetModName()
{
	return "valve";
}

void Director::NewGame( IWorld* world, IProxy* proxy )
{
	m_World = world;
	m_Proxy = proxy;
	m_World->RegisterListener( this );
	worldModel = m_World->GetWorldModel();
	memset( history, 0, historyLength * sizeof( worldHistory_t ) );
	memset( &frameEvent, 0, sizeof( frameEvent ) );
	ClearDirectorCommands();
	nextCutSeqnr = 0;
	lastCut = NULL;
	m_LastExecTime = 0.0f;
	nextCutTime = 0.0f;
	currentTime = 0.0f;
	maxRank = 0.0f;
	WriteSignonData();
	active = true;
}

void Director::AnalyseFrame( frame_t* frame )
{
	unsigned int maxclients = m_World->GetMaxClients();
	unsigned int seqnr = frame->seqnr;
	worldHistory_t* now = &history[seqnr % historyLength];
	float ranks[MAX_DIRECTOR_PLAYERS];

	for ( int i = 0; i < MAX_DIRECTOR_PLAYERS; i++ )
		ranks[i] = now->players[i].rank;
	memset( now, 0, sizeof( *now ) );
	for ( int i = 0; i < MAX_DIRECTOR_PLAYERS; i++ )
		now->players[i].rank = ranks[i];

	if ( frame->delta || !active )
	{
		m_System->DPrintf( "Director::AnalyseFrame: frame must be uncompressed.\n" );
		return;
	}

	if ( !currentTime )
	{
		nextCutSeqnr = seqnr;
		nextCutTime = frame->time;
	}
	currentSeqnr = seqnr;
	now->seqNr = seqnr;
	currentTime = frame->time;
	now->time = frame->time;

	unsigned int count = frame->entitynum < maxclients ? frame->entitynum : maxclients;
	entity_state_t* entities = (entity_state_t*)frame->entities;
	for ( unsigned int i = 0; i < count; i++ )
	{
		entity_state_t* entity = &entities[i];
		unsigned int playerNum = entity->number - 1;
		if ( playerNum >= maxclients )
			continue;

		playerData_t* player = &now->players[playerNum];
		player->active = entity->solid != 0;
		if ( player->active )
		{
			memcpy( player->origin, entity->origin, sizeof( player->origin ) );
			AngleVectors( entity->angles, player->angles, NULL, NULL );
		}
	}

	for ( int i = 0; i < MAX_DIRECTOR_PLAYERS; i++ )
		AnalysePlayer( i );

	now->event = frameEvent;
	memset( &frameEvent, 0, sizeof( frameEvent ) );
}

void Director::SmoothRank( int playerNum, float rank )
{
	history[( currentSeqnr + 40 ) % historyLength].players[playerNum].rank = 0.0f;
	if ( !( rank > 0.0f ) )
		return;

	for ( unsigned int i = 0; i < 40; i++ )
	{
		float weightedRank = gaussFilter[i * 5] * rank;
		history[( currentSeqnr - i ) % historyLength].players[playerNum].rank += weightedRank;
		history[( currentSeqnr + i ) % historyLength].players[playerNum].rank += weightedRank;
	}
}

float WeightedAngle( float* vec1, float* vec2 )
{
	float angle = AngleBetweenVectors( vec1, vec2 );
	if ( angle < 11.25f )
		return 1.0f;
	if ( angle < 22.5f )
		return 0.5f;
	if ( angle < 45.0f )
		return 0.25f;
	if ( angle >= 90.0f )
		return 0.05f;
	return 0.125f;
}

void Director::AnalysePlayer( int playerNum )
{
	worldHistory_t* now = &history[currentSeqnr % historyLength];
	playerData_t* player = &now->players[playerNum];
	float rank = RandomFloat( 0.0f, 0.01f );
	int target = 0;

	if ( player->active )
	{
		rank += 1.0f;
		if ( worldModel->IsValid() )
		{
			worldModel->SetPVS( player->origin );
			float bestTargetRank = 0.0f;
			for ( int i = 0; i < MAX_DIRECTOR_PLAYERS; i++ )
			{
				playerData_t* other = &now->players[i];
				if ( !other->active || !worldModel->InPVS( other->origin ) )
					continue;

				vec3_t offset;
				for ( int axis = 0; axis < 3; axis++ )
					offset[axis] = other->origin[axis] - player->origin[axis];
				float distance = VectorDistance( other->origin, player->origin );
				if ( distance < 1.0f )
					continue;

				float targetRank = (float)( 1.0 / distance );
				targetRank += WeightedAngle( other->angles, offset );
				for ( int axis = 0; axis < 3; axis++ )
					offset[axis] = -offset[axis];
				targetRank *= WeightedAngle( other->angles, offset );
				rank += targetRank;
				if ( targetRank > bestTargetRank )
				{
					target = i + 1;
					bestTargetRank = targetRank;
				}
			}
		}
	}
	else
	{
		rank = 0.0f;
	}

	player->target = target;
	player->rank += rank;
	SmoothRank( playerNum, rank );
}

void Director::WriteCommands( BitBuffer* stream, float startTime, float endTime )
{
	DirectorCmd* command = (DirectorCmd*)m_Commands.FindClosestKey( startTime );
	for ( ; command; command = (DirectorCmd*)m_Commands.GetNext() )
	{
		if ( command->m_Time > endTime )
			break;
		if ( command->m_Time <= startTime )
			continue;

		if ( command->m_Type == DRC_CMD_TIMESCALE )
		{
			float timescale;
			command->GetTimeScaleData( timescale );
			stream->WriteByte( svc_timescale );
			stream->WriteFloat( timescale );
		}
		command->WriteToStream( stream );
	}
}

void Director::WriteSignonData()
{
	static unsigned char cmd[] = { 1, DRC_CMD_START };
	m_World->AddSignonData( svc_director, cmd, sizeof( cmd ) );
}

void Director::AddEvent( int entity1, int entity2, unsigned int flags )
{
	if ( ( flags & DRC_FLAG_PRIO_MASK ) > ( frameEvent.flags & DRC_FLAG_PRIO_MASK ) )
	{
		frameEvent.flags = flags;
		frameEvent.entity1 = entity1;
		frameEvent.entity2 = entity2;
	}
}

float Director::AddBestGenericCut()
{
	float endTime = nextCutTime + 4.0f;
	float bestRank = 0.0f;
	int newTarget = 0;
	int newTarget2 = 0;
	unsigned int start = nextCutSeqnr % historyLength;

	for ( int playerNum = 0; playerNum < MAX_DIRECTOR_PLAYERS; playerNum++ )
	{
		float sumTarget2Rank[MAX_DIRECTOR_PLAYERS] = {};
		float sumRank = 0.0f;
		unsigned int index = start;
		while ( history[index].time < endTime )
		{
			playerData_t* player = &history[index].players[playerNum];
			sumRank += player->rank;
			if ( player->target )
				sumTarget2Rank[player->target - 1] += player->rank;
			if ( ++index == (unsigned int)historyLength )
				index = 0;
		}

		int target2 = 0;
		float targetRank = 0.0f;
		for ( int i = 0; i < MAX_DIRECTOR_PLAYERS; i++ )
		{
			if ( sumTarget2Rank[i] > targetRank )
			{
				targetRank = sumTarget2Rank[i];
				target2 = i + 1;
			}
		}
		if ( sumRank > bestRank )
		{
			bestRank = sumRank;
			newTarget = playerNum + 1;
			newTarget2 = target2;
		}
	}

	if ( bestRank > maxRank )
		maxRank = bestRank;
	if ( !newTarget )
		return 1.0f;

	float duration = 4.0f;
	if ( lastCut && lastCut->m_Type == DRC_CMD_EVENT )
	{
		int lastTarget1, lastTarget2, flags;
		lastCut->GetEventData( lastTarget1, lastTarget2, flags );
		if ( newTarget == lastTarget1 )
		{
			duration = 2.0f;
			if ( newTarget2 == lastTarget2 )
				return 1.0f;
		}
	}

	DirectorCmd* command = new DirectorCmd;
	command->SetEventData( newTarget, newTarget2, 0 );
	command->SetTime( nextCutTime );
	RandomizeCommand( command );
	m_Commands.Add( command, nextCutTime );
	lastCut = command;
	return duration;
}

float Director::AddBestMODCut()
{
	worldHistory_t* bestEvent = FindBestEvent();
	if ( !bestEvent )
		return 0.0f;

	DirectorCmd* command = new DirectorCmd;
	command->SetEventData( bestEvent->event.entity1, bestEvent->event.entity2, bestEvent->event.flags );
	command->SetTime( nextCutTime );
	RandomizeCommand( command );
	m_Commands.Add( command, command->m_Time );
	lastCut = command;

	int target1, target2, flags;
	command->GetEventData( target1, target2, flags );
	float duration = 2.0f;
	if ( ( flags & DRC_FLAG_SLOWMOTION ) && m_slowMotion != 0.0f )
	{
		command = new DirectorCmd;
		command->SetTimeScaleData( m_slowMotion );
		command->SetTime( bestEvent->time - 0.25f );
		m_Commands.Add( command, command->m_Time );

		command = new DirectorCmd;
		command->SetTimeScaleData( 1.0f );
		command->SetTime( bestEvent->time + 1.5f );
		m_Commands.Add( command, command->m_Time );
		duration = 1.5f;
	}
	return duration + bestEvent->time - nextCutTime;
}

int Director::GetClosestPlayer( frame_t* frame, int entityIndex )
{
	unsigned int maxclients = m_World->GetMaxClients();
	if ( !worldModel->IsValid() )
		return 0;

	entity_state_t* entities = (entity_state_t*)frame->entities;
	unsigned int index;
	for ( index = 0; index < frame->entitynum; index++ )
	{
		if ( entities[index].number == entityIndex )
			break;
	}
	if ( index == frame->entitynum )
		return 0;

	vec3_t origin;
	memcpy( origin, entities[index].origin, sizeof( origin ) );
	worldModel->SetPVS( origin );
	unsigned int count = frame->entitynum < maxclients ? frame->entitynum : maxclients;
	int bestPlayer = 0;
	float minDistance = 32000.0f;
	for ( unsigned int i = 0; i < count; i++ )
	{
		entity_state_t* entity = &entities[i];
		if ( (unsigned int)( entity->number - 1 ) >= maxclients || !worldModel->InPVS( entity->origin ) )
			continue;

		float distance = VectorDistance( entity->origin, origin );
		if ( distance >= 1.0f && distance < minDistance )
		{
			minDistance = distance;
			bestPlayer = entity->number;
		}
	}
	return bestPlayer;
}

void Director::ClearDirectorCommands()
{
	DirectorCmd* command = (DirectorCmd*)m_Commands.GetFirst();
	while ( command )
	{
		delete command;
		command = (DirectorCmd*)m_Commands.GetNext();
	}
	m_Commands.Clear( false );
}

void Director::RandomizeCommand( DirectorCmd* command )
{
	if ( ( command->m_Type & DRC_FLAG_NO_RANDOM ) || command->m_Type != DRC_CMD_EVENT )
		return;

	int target1, target2, flags;
	command->GetEventData( target1, target2, flags );
	if ( flags & DRC_FLAG_SLOWMOTION )
	{
		if ( RandomFloat( 0.0f, 1.0f ) > 0.25f )
			flags &= ~DRC_FLAG_SLOWMOTION;
		else if ( (unsigned int)( ( flags & DRC_FLAG_PRIO_MASK ) - 7 ) <= 2
			&& RandomFloat( 0.0f, 1.0f ) < 0.15f )
			flags |= DRC_FLAG_SLOWMOTION;
	}
	else if ( (unsigned int)( ( flags & DRC_FLAG_PRIO_MASK ) - 7 ) <= 2
		&& RandomFloat( 0.0f, 1.0f ) < 0.15f )
	{
		flags |= DRC_FLAG_SLOWMOTION;
	}

	if ( RandomFloat( 0.0f, 1.0f ) < 0.33f )
		flags ^= DRC_FLAG_DRAMATIC;
	if ( target1 && target2 && !( flags & DRC_FLAG_SLOWMOTION ) && RandomFloat( 0.0f, 1.0f ) < 0.33f )
	{
		int target = target1;
		target1 = target2;
		target2 = target;
	}

	worldHistory_t* now = &history[nextCutSeqnr % historyLength];
	playerData_t* player1 = &now->players[target1 % MAX_DIRECTOR_PLAYERS];
	playerData_t* player2 = &now->players[target2 % MAX_DIRECTOR_PLAYERS];
	vec3_t direction;
	direction[0] = player2->origin[0] - player1->origin[0];
	direction[1] = player2->origin[1] - player1->origin[1];
	direction[2] = 0.0f;
	VectorAngles( direction, direction );
	vec3_t angles;
	memcpy( angles, player1->angles, sizeof( angles ) );
	if ( AngleLeftOfOther( direction, angles ) )
		flags &= ~DRC_FLAG_SIDE;
	else
		flags |= DRC_FLAG_SIDE;
	if ( target1 == target2 )
		target2 = 0;
	command->SetEventData( target1, target2, flags );
}

void Director::ExecuteCommand( int commandID, char* commandLine )
{
	if ( commandID == CMD_ID_SLOWMOTION )
		CMD_SlowMotion( commandLine );
	else
		m_System->Printf( "ERROR! Director::ExecuteCommand: unknown command ID %i.\n", commandID );
}

char* Director::GetStatusLine()
{
	return "Default Director";
}

char* Director::GetType()
{
	return "director001";
}

void Director::ExecuteDirectorCommands()
{
	unsigned char bufferdata[4096];
	BitBuffer buffer( bufferdata, sizeof( bufferdata ) );
	buffer.Clear();
	float newTime = (float)m_Proxy->GetSpectatorTime();
	DirectorCmd* command = (DirectorCmd*)m_Commands.FindClosestKey( m_LastExecTime );
	if ( !command )
		return;

	for ( ; command; command = (DirectorCmd*)m_Commands.GetNext() )
	{
		if ( command->m_Time > newTime )
			break;
		if ( command->m_Time <= m_LastExecTime )
			continue;

		if ( command->m_Type == DRC_CMD_TIMESCALE )
		{
			float timescale;
			command->GetTimeScaleData( timescale );
			m_Proxy->SetClientTimeScale( timescale );
			if ( timescale < 1.0f )
			{
				DirectorCmd slowmo;
				vec3_t pos = { 0.02f, 0.85f, 0.0f };
				slowmo.SetMessageData( 0, 0xFFFFA000, pos, 0.3f, 0.1f, 2.0f, 0.0f, "Slow Motion" );
				slowmo.WriteToStream( &buffer );
			}
		}
		else
		{
			command->WriteToStream( &buffer );
		}
	}

	if ( buffer.sizeError )
		m_System->Printf( "Director::ExecuteDirectorCommands: command overflow.\n" );
	else
		m_Proxy->Broadcast( bufferdata, buffer.CurrentSize(), GROUP_CLIENT | GROUP_PROXY | GROUP_DEMO | GROUP_UNUSED, true );
	m_LastExecTime = newTime;
}

void Director::CMD_SlowMotion( char* commandLine )
{
	TokenLine params( commandLine );
	if ( params.CountToken() == 2 )
	{
		float factor = (float)atof( params.GetToken( 1 ) );
		if ( factor > 4.0f )
			m_slowMotion = 4.0f;
		else if ( factor < 0.05f )
			m_slowMotion = 0.05f;
		else
			m_slowMotion = factor;
	}
	else
	{
		m_System->Printf( "Syntax: slowmotion <factor>\n" );
		m_System->Printf( "Currently slowmotion factor is %.2f\n", m_slowMotion );
	}
}

Director::worldHistory_t* Director::FindBestEvent()
{
	unsigned int index = nextCutSeqnr % historyLength;
	unsigned int bestEvent[4];
	unsigned int bestEventPrio[4];
	for ( int i = 0; i < 4; i++ )
	{
		bestEventPrio[i] = 0;
		bestEvent[i] = 0;
		float endTime = nextCutTime + ( i + 1 ) * 2.0f;
		while ( history[index].time < endTime )
		{
			unsigned int priority = history[index].event.flags & DRC_FLAG_PRIO_MASK;
			if ( priority > bestEventPrio[i] )
			{
				bestEventPrio[i] = priority;
				bestEvent[i] = index;
			}
			if ( ++index == (unsigned int)historyLength )
				index = 0;
		}
	}

	if ( !bestEventPrio[0] && !bestEventPrio[1] && !bestEventPrio[2] )
		return NULL;
	if ( bestEventPrio[0] > bestEventPrio[1] )
	{
		if ( bestEventPrio[0] > bestEventPrio[2] )
			return &history[bestEvent[0]];
	}
	else if ( bestEventPrio[1] >= bestEventPrio[2] )
	{
		if ( bestEventPrio[1] >= bestEventPrio[3] )
			return &history[bestEvent[1]];
		return bestEventPrio[0] ? &history[bestEvent[0]] : NULL;
	}

	if ( bestEventPrio[2] > bestEventPrio[3] )
		return &history[bestEvent[2]];
	return bestEventPrio[0] ? &history[bestEvent[0]] : NULL;
}

int Director::AddCommand( DirectorCmd* command )
{
	if ( command->GetType() > DRC_CMD_LAST )
		return 0;
	if ( command->GetType() == DRC_CMD_EVENT )
	{
		int target1, target2, flags;
		command->GetEventData( target1, target2, flags );
		AddEvent( target1, target2, flags );
		return 0;
	}

	DirectorCmd* copy = new DirectorCmd;
	*copy = *command;
	m_Commands.Add( copy, command->m_Time );
	return 1;
}

IObjectContainer* Director::GetCommands()
{
	return &m_Commands;
}

bool Director::RemoveCommand( int )
{
	return false;
}

DirectorCmd* Director::GetLastCommand()
{
	return NULL;
}

void Director::WriteProxyStatus( BitBuffer* )
{
}
