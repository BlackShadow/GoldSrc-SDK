#include "DemoPlayer.h"

#include "common/BaseSystemModule.h"
#include "common/IBaseSystem.h"
#include "common/IEngineWrapper.h"
#include "common/ObjectDictionary.h"
#include "common/ObjectList.h"
#include "common/TokenLine.h"
#include "common/event_args.h"
#include "common/ref_params.h"
#include "engine/cl_demo.h"
#include "HLTV/common/Bitbuffer.h"
#include "HLTV/common/DirectorCmd.h"
#include "HLTV/common/IServer.h"
#include "HLTV/common/IWorld.h"
#include "HLTV/common/byteorder.h"
#include "HLTV/common/mathlib.h"

#include "public/tier1/interface.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define MINIMUM_TIME_SCALE		0.05f
#define MAXIMUM_TIME_SCALE		4.0f
#define FRAME_GAP_THRESHOLD		2.0f
#define FRAME_BOUNDARY_OFFSET	0.01f

#define DEMO_STREAM_CAPACITY		65536
#define MAXIMUM_CAMERA_PATH_BYTES	250
#define DEMO_CONSOLE_COMMAND_BYTES	64
#define MAXIMUM_SOUND_NAME_BYTES	256
#define MAXIMUM_CLIENT_DEMO_BYTES	32768
#define CAMERA_PATH_HEADER_BYTES	2
#define CAMERA_PATH_TIME_BYTES		(int)sizeof( short )
#define PAUSE_MESSAGE_CAPACITY		32

namespace
{
	enum ServerMessage
	{
		ServerMessageTime		= 7,
		ServerMessageSetPause	= 24,
		ServerMessageSignon		= 25,
		ServerMessageDirector	= 51,
		ServerMessageTimeScale	= 55
	};

	enum WorldSignal
	{
		WORLD_SIGNAL_RESET		= 1,
		WORLD_SIGNAL_NEWGAME	= 2,
		WORLD_SIGNAL_PAUSE		= 5,
		WORLD_SIGNAL_UNPAUSE	= 6
	};

	enum ServerSignal
	{
		SERVER_SIGNAL_DISCONNECT = 6
	};

	enum DemoPlayerSignal
	{
		DEMOPLAYER_SIGNAL_COMMANDS_CHANGED	= 1,
		DEMOPLAYER_SIGNAL_COMMAND_EXECUTED	= 2
	};
}

static DemoPlayer g_DemoPlayerSingleton;

static void* CreateDemoPlayer();

static InterfaceReg g_DemoPlayerInterface( CreateDemoPlayer, "demoplayer001" );

static void* CreateDemoPlayer()
{
	return &g_DemoPlayerSingleton;
}

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
DemoPlayer::DemoPlayer()
{
	m_System = NULL;
	m_Engine = NULL;
	m_World = NULL;
	m_Server = NULL;
}

DemoPlayer::~DemoPlayer()
{
}

double DemoPlayer::GetWorldTime()
{
	return m_WorldTime;
}

double DemoPlayer::GetStartTime()
{
	frame_t* frame = m_World->GetFirstFrame();

	if ( frame )
		return frame->time;

	return 0.0;
}

double DemoPlayer::GetEndTime()
{
	frame_t* frame = m_World->GetLastFrame();

	if ( frame )
		return frame->time;

	return 0.0;
}

bool DemoPlayer::IsLoading()
{
	return m_Server != NULL && m_Server->IsConnected();
}

bool DemoPlayer::IsActive()
{
	return m_PlayerState != DEMO_INACTIVE;
}

char* DemoPlayer::GetType()
{
	return "demoplayer001";
}

char* DemoPlayer::GetStatusLine()
{
	return "No status available.\n";
}

IWorld* DemoPlayer::GetWorld()
{
	return m_World;
}

float DemoPlayer::GetTimeScale()
{
	return m_TimeScale;
}

char* DemoPlayer::GetModName()
{
	return "valve";
}

IObjectContainer* DemoPlayer::GetCommands()
{
	return &m_Commands;
}

void DemoPlayer::SetWorldTime( double time, bool relative )
{
	if ( relative )
		time += m_WorldTime;

	m_WorldTime = time;
}

void DemoPlayer::SetPaused( bool state )
{
	m_IsPaused = state;
}

void DemoPlayer::SetEditMode( bool state )
{
	m_EditorMode = state;
}

bool DemoPlayer::IsEditMode()
{
	return m_EditorMode;
}

void DemoPlayer::Stop()
{
	m_Server->Disconnect();
	m_World->Reset();
	m_PlayerState = DEMO_INACTIVE;
}

void DemoPlayer::ForceHLTV( bool state )
{
	m_ForceHLTV = state;
}

void DemoPlayer::SetMasterMode( bool state )
{
	m_MasterMode = state;
}

bool DemoPlayer::IsMasterMode()
{
	return m_MasterMode;
}

bool DemoPlayer::IsPaused()
{
	return m_IsPaused;
}

void DemoPlayer::ReadNetchanState( int* incoming_sequence, int* incoming_acknowledged,
		int* incoming_reliable_acknowledged, int* incoming_reliable_sequence, int* outgoing_sequence,
		int* reliable_sequence, int* last_reliable_sequence )
{
	*incoming_sequence = m_Outgoing_sequence;
	*incoming_acknowledged = m_Outgoing_sequence;
	*incoming_reliable_acknowledged = 0;
	*incoming_reliable_sequence = 0;
	*outgoing_sequence = m_Outgoing_sequence;
	*reliable_sequence = 0;
	*last_reliable_sequence = 0;
}

void DemoPlayer::RemoveFrames( double, double )
{
}

char* DemoPlayer::GetFileName()
{
	return m_DemoFileName;
}

DirectorCmd* DemoPlayer::GetLastCommand()
{
	return m_LastCmd;
}

void DemoPlayer::SetTimeScale( float scale )
{
	m_TimeScale = scale;

	if ( scale > MAXIMUM_TIME_SCALE )
		m_TimeScale = MAXIMUM_TIME_SCALE;
	else if ( scale < MINIMUM_TIME_SCALE )
		m_TimeScale = MINIMUM_TIME_SCALE;

	m_DemoStream.WriteByte( ServerMessageTimeScale );
	m_DemoStream.WriteFloat( m_TimeScale );
}

void DemoPlayer::GetDemoViewInfo( ref_params_t* rp, float* view, int* viewmodel )
{
	frame_t* frame = m_World->GetFrameBySeqNr( m_LastFrameSeqNr );
	frame_t* previous = m_World->GetFrameBySeqNr( m_LastFrameSeqNr - 1 );

	if ( !frame )
		return;

	demo_info_t* info = (demo_info_t*)frame->demoInfo;

	if ( !info )
		return;

	int viewport[4];
	memcpy( viewport, rp->viewport, sizeof( viewport ) );
	usercmd_s* cmd = rp->cmd;
	movevars_s* movevars = rp->movevars;

	*rp = info->rp;

	memcpy( rp->viewport, viewport, sizeof( viewport ) );
	rp->cmd = cmd;
	rp->movevars = movevars;

	view[0] = info->view[0];
	view[1] = info->view[1];
	view[2] = info->view[2];
	*viewmodel = info->viewmodel;

	if ( !previous )
		return;

	demo_info_t* from = (demo_info_t*)previous->demoInfo;

	if ( !from || !( previous->time < frame->time ) )
		return;

	float frac = (float)( ( m_WorldTime - previous->time ) / ( frame->time - previous->time ) );

	rp->vieworg[0] = ( info->rp.vieworg[0] - from->rp.vieworg[0] ) * frac + from->rp.vieworg[0];
	rp->vieworg[1] = ( info->rp.vieworg[1] - from->rp.vieworg[1] ) * frac + from->rp.vieworg[1];
	rp->vieworg[2] = ( info->rp.vieworg[2] - from->rp.vieworg[2] ) * frac + from->rp.vieworg[2];

	float delta = info->rp.viewangles[PITCH] - from->rp.viewangles[PITCH];

	if ( delta > 180.0f )
		delta -= 360.0f;
	else if ( delta < -180.0f )
		delta += 360.0f;

	rp->viewangles[PITCH] = from->rp.viewangles[PITCH] + delta * frac;

	delta = info->rp.viewangles[YAW] - from->rp.viewangles[YAW];

	if ( delta > 180.0f )
		delta -= 360.0f;
	else if ( delta < -180.0f )
		delta += 360.0f;

	rp->viewangles[YAW] = from->rp.viewangles[YAW] + delta * frac;

	delta = info->rp.viewangles[ROLL] - from->rp.viewangles[ROLL];

	if ( delta > 180.0f )
		delta -= 360.0f;
	else if ( delta < -180.0f )
		delta += 360.0f;

	rp->viewangles[ROLL] = from->rp.viewangles[ROLL] + delta * frac;
	NormalizeAngles( rp->viewangles );

	rp->simvel[0] = ( info->rp.simvel[0] - from->rp.simvel[0] ) * frac + from->rp.simvel[0];
	rp->simvel[1] = ( info->rp.simvel[1] - from->rp.simvel[1] ) * frac + from->rp.simvel[1];
	rp->simvel[2] = ( info->rp.simvel[2] - from->rp.simvel[2] ) * frac + from->rp.simvel[2];

	rp->simorg[0] = ( info->rp.simorg[0] - from->rp.simorg[0] ) * frac + from->rp.simorg[0];
	rp->simorg[1] = ( info->rp.simorg[1] - from->rp.simorg[1] ) * frac + from->rp.simorg[1];
	rp->simorg[2] = ( info->rp.simorg[2] - from->rp.simorg[2] ) * frac + from->rp.simorg[2];

	rp->viewheight[0] = ( info->rp.viewheight[0] - from->rp.viewheight[0] ) * frac + from->rp.viewheight[0];
	rp->viewheight[1] = ( info->rp.viewheight[1] - from->rp.viewheight[1] ) * frac + from->rp.viewheight[1];
	rp->viewheight[2] = ( info->rp.viewheight[2] - from->rp.viewheight[2] ) * frac + from->rp.viewheight[2];

	view[0] = ( info->view[0] - from->view[0] ) * frac + from->view[0];
	view[1] = ( info->view[1] - from->view[1] ) * frac + from->view[1];
	view[2] = ( info->view[2] - from->view[2] ) * frac + from->view[2];
}

bool DemoPlayer::LoadGame( char* filename )
{
	bool loaded = m_Server->LoadDemo( m_World, filename, m_ForceHLTV, false );

	if ( !loaded )
		return false;

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
	strncpy( m_DemoFileName, filename, sizeof( m_DemoFileName ) - 1 );
#if defined( _MSC_VER )
#pragma warning( pop )
#endif
	m_DemoFileName[sizeof( m_DemoFileName ) - 1] = 0;
	m_World->SetBufferSize( -1.0f );
	m_PlayerState = DEMO_LOADING;
	m_LastClockUpdateTime = 0.0;
	m_LastFrameTime = 0.0;
	m_Outgoing_sequence = 0;
	m_MasterMode = true;
	return true;
}

bool DemoPlayer::SaveGame( char* filename )
{
	if ( IsLoading() )
		return false;

	SetPaused( true );
	m_IsSaving = true;

	bool saved;

	if ( filename )
		saved = m_World->SaveAsDemo( filename, this );
	else
		saved = m_World->SaveAsDemo( m_DemoFileName, this );

	m_IsSaving = false;
	return saved;
}

void DemoPlayer::ExecuteDirectorCmd( DirectorCmd* cmd )
{
	int type = cmd->GetType();

	if ( type == DRC_CMD_CAMERA )
	{
		vec_t position[3];
		vec_t angles[3];
		float fov;
		int entity;

		cmd->GetCameraData( position, angles, fov, entity );
		cmd->SetCameraData( position, angles, fov, 0 );
		cmd->WriteToStream( &m_DemoStream );
		cmd->SetCameraData( position, angles, fov, entity );
	}
	else if ( type == DRC_CMD_TIMESCALE )
	{
		float factor;

		cmd->GetTimeScaleData( factor );
		SetTimeScale( factor );
	}
	else
	{
		cmd->WriteToStream( &m_DemoStream );
	}
}

bool DemoPlayer::Init( IBaseSystem* system, int serial, char* name )
{
	if ( !BaseSystemModule::Init( system, serial, name ) )
		return false;

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
	if ( name )
	{
		strncpy( m_Name, name, sizeof( m_Name ) - 1 );
		m_Name[sizeof( m_Name ) - 1] = 0;
	}
	else
	{
#ifdef _WIN32
		_snprintf( m_Name, sizeof( m_Name ), "demoplayer001" );
#else
		snprintf( m_Name, sizeof( m_Name ), "demoplayer001" );
#endif
	}
#if defined( _MSC_VER )
#pragma warning( pop )
#endif

	m_Engine = (IEngineWrapper*)m_System->GetModule( "enginewrapper002", "", NULL );

	if ( !m_Engine )
	{
		m_System->Printf( "DemoPlayer::Init: couldn't get engine interface.\n" );
		return false;
	}

	m_Server = (IServer*)m_System->GetModule( "server001", "core", "DemoServer" );

	if ( !m_Server )
	{
		m_System->Printf( "DemoPlayer::Init: couldn't load server module.\n" );
		return false;
	}

	m_Server->RegisterListener( this );
	m_Server->SetDirector( this );
	m_Server->SetDelayReconnect( false );

	m_World = (IWorld*)m_System->GetModule( "world001", "core", "DemoWorld" );

	if ( !m_World )
	{
		m_System->Printf( "DemoPlayer::Init: couldn't load world module.\n" );
		return false;
	}

	m_System->RegisterCommand( "dem_jump", this, CMD_ID_JUMP );
	m_System->RegisterCommand( "dem_forcehltv", this, CMD_ID_FORCEHLTV );
	m_System->RegisterCommand( "dem_pause", this, CMD_ID_PAUSE );
	m_System->RegisterCommand( "dem_speed", this, CMD_ID_SPEED );
	m_System->RegisterCommand( "dem_start", this, CMD_ID_START );
	m_System->RegisterCommand( "dem_save", this, CMD_ID_SAVE );
	m_World->RegisterListener( this );
	m_DemoStream.Resize( DEMO_STREAM_CAPACITY );
	SetTimeScale( 1.0f );
	SetPaused( false );
	m_LastCmd = NULL;
	m_Outgoing_sequence = 0;
	m_DeltaFrameSeqNr = 0;
	m_CommandCounter = 0;
	m_LastFrameSeqNr = 0;
	m_LastFrameTime = 0.0;
	m_PlayerState = DEMO_INACTIVE;
	m_EditorMode = false;
	m_MasterMode = true;
	m_IsSaving = false;
	memset( m_DemoFileName, 0, sizeof( m_DemoFileName ) );
	m_State = MODULE_RUNNING;
	m_System->DPrintf( "DemoPlayer module initialized.\n" );
	return true;
}

void DemoPlayer::RunFrame( double time )
{
	BaseSystemModule::RunFrame( time );

	if ( m_PlayerState != DEMO_INACTIVE )
		RunClocks();
}

double DemoPlayer::GetPlayerTime()
{
	return m_PlayerTime;
}

IDirector* DemoPlayer::GetDirector()
{
	return this;
}

void DemoPlayer::CMD_Start( char* )
{
	if ( !IsActive() )
	{
		m_System->Printf( "Not viewing a demo.\n" );
		return;
	}

	SetWorldTime( GetStartTime(), false );
}

void DemoPlayer::WriteSpawn( BitBuffer* stream )
{
	int client = 0;

	m_World->WriteSigonData( stream );
	stream->WriteByte( ServerMessageTime );
	stream->WriteFloat( 1.0f );

	while ( client < m_World->GetMaxClients() )
	{
		m_World->WriteClientUpdate( stream, client );
		client++;
	}

	m_World->WriteLightStyles( stream );
	stream->WriteByte( ServerMessageSignon );
	stream->WriteByte( 1 );
}

void DemoPlayer::RunClocks()
{
	float elapsed = (float)( m_SystemTime - m_LastClockUpdateTime );

	m_LastClockUpdateTime = m_SystemTime;
	m_PlayerTime += elapsed;

	if ( !m_World->IsActive() || m_IsPaused )
		return;

	m_WorldTime += elapsed;

	frame_t* current = m_World->GetFrameBySeqNr( m_LastFrameSeqNr );
	frame_t* next = m_World->GetFrameBySeqNr( m_LastFrameSeqNr + 1 );

	if ( current && next && next->time - current->time > FRAME_GAP_THRESHOLD )
		m_WorldTime = next->time - FRAME_BOUNDARY_OFFSET;

	frame_t* last = m_World->GetLastFrame();
	frame_t* first = m_World->GetFirstFrame();

	if ( !last || !first )
		return;

	if ( m_WorldTime > last->time )
		m_WorldTime = last->time;
	else if ( first->time > m_WorldTime )
		m_WorldTime = first->time - FRAME_BOUNDARY_OFFSET;
}

char* DemoPlayer::FormatTime( float time )
{
	static char timeCode[16];
	int whole = (int)time;
	int minutes = whole / 60;

	snprintf( timeCode, sizeof( timeCode ) - 1, "%02u:%02u:%02u", minutes, whole - minutes * 60,
		(int)( time * 100.0f ) % 100 );
	timeCode[sizeof( timeCode ) - 1] = 0;
	return timeCode;
}

void DemoPlayer::WriteDatagram( BitBuffer* stream )
{
	frame_t* frame = m_World->GetFrameByTime( m_WorldTime );

	if ( !frame )
		return;

	unsigned int sequence = frame->seqnr;

	if ( m_LastFrameSeqNr != 0 && m_LastFrameSeqNr <= sequence )
	{
		if ( m_LastFrameSeqNr >= sequence )
			return;
	}
	else
	{
		m_LastFrameSeqNr = sequence - 1;

		if ( m_LastFrameSeqNr >= sequence )
			return;
	}

	stream->WriteByte( ServerMessageTime );
	stream->WriteFloat( (float)( m_PlayerTime - ( m_WorldTime - frame->time ) ) );
	m_World->WriteFrame( frame, m_LastFrameSeqNr, stream, stream, m_DeltaFrameSeqNr, m_Outgoing_sequence, true );

	if ( m_MasterMode )
		WriteCommands( stream, (float)m_LastFrameTime, (float)m_WorldTime );

	m_LastFrameTime = m_WorldTime;

	if ( stream->sizeError )
	{
		m_System->Printf( "Demo data stream overflow.\n" );
		stream->Clear();
		m_DeltaFrameSeqNr = 0;
		m_LastFrameSeqNr = 0;
		return;
	}

	m_Outgoing_sequence++;
	m_DeltaFrameSeqNr = sequence;
	m_LastFrameSeqNr = sequence;
}

void DemoPlayer::ExecuteDemoFileCommands( BitBuffer* stream )
{
	int message;

	while ( ( message = stream->ReadByte() ) != -1 )
	{
		switch ( message )
		{
		case DEMO_CMD_CONSOLE:
		{
			char command[DEMO_CONSOLE_COMMAND_BYTES];

			stream->ReadBuf( sizeof( command ), command );

			if ( m_Engine->ValidStuffText( command ) )
			{
				m_Engine->Cbuf_AddFilteredText( command );
				m_Engine->Cbuf_AddFilteredText( "\n" );
			}
			else
			{
				m_System->Printf( "Demo tried to send invalid command:\"%s\"\n", command );
			}
			break;
		}

		case DEMO_CMD_CLIENTDATA:
		{
			client_data_t clientData;

			stream->ReadBuf( sizeof( clientData ), &clientData );
			m_Engine->DemoUpdateClientData( &clientData );
			break;
		}

		case DEMO_CMD_EVENT:
		{
			event_args_t event;
			int flags = LittleLong( stream->ReadLong() );
			int index = LittleLong( stream->ReadLong() );
			float delay = LittleFloat( stream->ReadFloat() );

			stream->ReadBuf( sizeof( event ), &event );
			m_Engine->CL_QueueEvent( flags, index, delay, &event );
			break;
		}

		case DEMO_CMD_WEAPONANIM:
		{
			int sequence = LittleLong( stream->ReadLong() );
			int body = LittleLong( stream->ReadLong() );

			m_Engine->HudWeaponAnim( sequence, body );
			break;
		}

		case DEMO_CMD_SOUND:
		{
			char soundName[MAXIMUM_SOUND_NAME_BYTES];
			unsigned int channel = stream->ReadLong();
			unsigned int length = stream->ReadLong();
			float attenuation;
			float volume;
			int entity;
			int pitch;

			if ( length >= MAXIMUM_SOUND_NAME_BYTES )
				length = MAXIMUM_SOUND_NAME_BYTES;

			stream->ReadBuf( length, soundName );
			soundName[length] = 0;
			attenuation = LittleFloat( stream->ReadFloat() );
			volume = LittleFloat( stream->ReadFloat() );
			entity = LittleLong( stream->ReadLong() );
			pitch = LittleLong( stream->ReadLong() );
			m_Engine->CL_DemoPlaySound( channel, soundName, attenuation, volume, entity, pitch );
			break;
		}

		case DEMO_CMD_CLIENTBUFFER:
		{
			unsigned char clientBuffer[MAXIMUM_CLIENT_DEMO_BYTES];
			unsigned int length = MAXIMUM_CLIENT_DEMO_BYTES;
#ifdef _WIN32
			memset( clientBuffer, 0, sizeof( clientBuffer ) );
			unsigned int stored = stream->ReadLong();
#else
			unsigned int stored = stream->ReadLong();
			memset( clientBuffer, 0, sizeof( clientBuffer ) );
#endif

			if ( stored < MAXIMUM_CLIENT_DEMO_BYTES )
				length = stored;

			stream->ReadBuf( length, clientBuffer );
			m_Engine->ClientDLL_ReadDemoBuffer( length, clientBuffer );
			break;
		}

		default:
			m_System->Printf(
				"WARNING! DemoPlayer::ExecuteDemoFileCommands: unexpected demo file command %i\n", message );
			return;
		}
	}
}

void DemoPlayer::CMD_Jump( char* cmdLine )
{
	if ( !IsActive() )
	{
		m_System->Printf( "Not viewing a demo.\n" );
		return;
	}

	TokenLine line( cmdLine );

	if ( line.CountToken() != 2 )
	{
		m_System->Printf( "dem_jump <seconds>\n" );
		return;
	}

	SetWorldTime( atof( line.GetToken( 1 ) ), true );
	SetPaused( true );
}

void DemoPlayer::CMD_ForceHLTV( char* cmdLine )
{
	TokenLine line( cmdLine );

	if ( line.CountToken() != 2 )
	{
		m_System->Printf( "dem_forcehltv <0|1>\n" );
		return;
	}

	ForceHLTV( atoi( line.GetToken( 1 ) ) == 1 );
}

void DemoPlayer::CMD_Save( char* cmdLine )
{
	TokenLine line( cmdLine );

	if ( line.CountToken() != 2 )
	{
		m_System->Printf( "dem_save <filename>\n" );
		return;
	}

	if ( !SaveGame( line.GetToken( 1 ) ) )
		m_System->Printf( "Warning! Could not save game as demo file.\n" );
}

void DemoPlayer::CMD_Pause( char* cmdLine )
{
	if ( !IsActive() )
	{
		m_System->Printf( "Not viewing a demo.\n" );
		return;
	}

	TokenLine line( cmdLine );

	if ( line.CountToken() != 2 )
	{
		m_System->Printf( "dem_pause <0|1>\n" );
		return;
	}

	SetPaused( atoi( line.GetToken( 1 ) ) == 1 );
}

void DemoPlayer::CMD_Speed( char* cmdLine )
{
	if ( !IsActive() )
	{
		m_System->Printf( "Not viewing a demo.\n" );
		return;
	}

	TokenLine line( cmdLine );

	if ( line.CountToken() != 2 )
	{
		m_System->Printf( "dem_speed <replayspeed>\n" );
		return;
	}

	SetTimeScale( (float)atof( line.GetToken( 1 ) ) );
}

void DemoPlayer::ExecuteCommand( int commandID, char* commandLine )
{
	switch ( commandID )
	{
	case CMD_ID_JUMP:
		CMD_Jump( commandLine );
		break;

	case CMD_ID_FORCEHLTV:
		CMD_ForceHLTV( commandLine );
		break;

	case CMD_ID_PAUSE:
		CMD_Pause( commandLine );
		break;

	case CMD_ID_SPEED:
		CMD_Speed( commandLine );
		break;

	case CMD_ID_START:
		CMD_Start( commandLine );
		break;

	case CMD_ID_SAVE:
		CMD_Save( commandLine );
		break;

	default:
		m_System->Printf( "ERROR! DemoPlayer::ExecuteCommand: unknown command ID %i.\n", commandID );
		break;
	}
}

void DemoPlayer::ReindexCommands()
{
	int index = 1;
	DirectorCmd* cmd = (DirectorCmd*)m_Commands.GetFirst();

	while ( cmd )
	{
		cmd->m_Index = index;
		index++;
		cmd = (DirectorCmd*)m_Commands.GetNext();
	}

	FireSignal( DEMOPLAYER_SIGNAL_COMMANDS_CHANGED, NULL );
}

int DemoPlayer::AddCommand( DirectorCmd* cmd )
{
	if ( cmd->GetType() == DRC_CMD_WAYPOINTS )
		return 0;

	DirectorCmd* copy = new DirectorCmd();

	copy->Copy( cmd );

	if ( !m_Commands.Add( copy, copy->GetTime() ) )
	{
		delete copy;
		return 0;
	}

	ReindexCommands();
	return copy->m_Index;
}

bool DemoPlayer::RemoveCommand( int index )
{
	DirectorCmd* cmd = (DirectorCmd*)m_Commands.GetFirst();

	while ( cmd )
	{
		if ( cmd->m_Index == index )
			break;

		cmd = (DirectorCmd*)m_Commands.GetNext();
	}

	if ( !cmd )
		return false;

	m_Commands.Remove( cmd );

	if ( m_LastCmd == cmd )
		m_LastCmd = NULL;

	delete cmd;
	ReindexCommands();
	return true;
}

void DemoPlayer::WriteCameraPath( DirectorCmd* cmd, BitBuffer* stream )
{
	ObjectList waypoints;

	waypoints.Init();

	float time = cmd->GetTime();
	DirectorCmd* node = (DirectorCmd*)m_Commands.FindExactKey( cmd->m_Time );
	bool firstMarker = true;

	while ( node && node->GetType() == DRC_CMD_CAMPATH )
	{
		vec_t position[3];
		vec_t angles[3];
		float fov;
		int flags;

		node->GetCamPathData( position, angles, fov, flags );

		if ( flags & DRC_FLAG_STARTPATH )
		{
			if ( !firstMarker )
				break;

			firstMarker = false;
		}

		waypoints.AddTail( node );
		node = (DirectorCmd*)m_Commands.GetNext();
	}

	DirectorCmd* path = (DirectorCmd*)waypoints.GetFirst();

	if ( !path )
	{
		m_System->Printf( "Warning! No waypoints in camera path!\n" );
		return;
	}

	int count = waypoints.CountElements();
	int bytes = ( path->m_Size + CAMERA_PATH_TIME_BYTES ) * count + CAMERA_PATH_HEADER_BYTES;

	if ( bytes > MAXIMUM_CAMERA_PATH_BYTES )
	{
		m_System->Printf( "Warning! Too many waypoints in a camera path!\n" );
		return;
	}

	stream->WriteByte( ServerMessageDirector );
	stream->WriteByte( bytes );
	stream->WriteByte( DRC_CMD_WAYPOINTS );
	stream->WriteByte( count );

	while ( path )
	{
		float delta = (float)( ( (double)path->GetTime() - time ) * 100.0f );

		stream->WriteShort( (int)delta );
		stream->WriteBuf( path->m_Data.data, path->m_Size );
		path = (DirectorCmd*)waypoints.GetNext();
	}

	waypoints.Clear( false );
}

void DemoPlayer::WriteCommands( BitBuffer* stream, float startTime, float endTime )
{
	DirectorCmd* cmd = (DirectorCmd*)m_Commands.FindClosestKey( startTime );

	while ( cmd )
	{
		if ( !( endTime >= cmd->m_Time ) )
			break;

		if ( !( cmd->m_Time > startTime ) )
		{
			cmd = (DirectorCmd*)m_Commands.GetNext();
			continue;
		}

		if ( m_IsSaving )
		{
			cmd->WriteToStream( stream );
		}
		else
		{
			if ( cmd->m_Type == DRC_CMD_TIMESCALE )
			{
				float factor;

				cmd->GetTimeScaleData( factor );
				stream->WriteByte( ServerMessageTimeScale );
				stream->WriteFloat( factor );
				m_TimeScale = factor;
			}
			else if ( cmd->m_Type == DRC_CMD_CAMPATH )
			{
				vec_t position[3];
				vec_t angles[3];
				float fov;
				int flags;

				cmd->GetCamPathData( position, angles, fov, flags );

				if ( flags & DRC_FLAG_STARTPATH )
					WriteCameraPath( cmd, stream );
			}
			else
			{
				cmd->WriteToStream( stream );
			}

			m_LastCmd = cmd;
			FireSignal( DEMOPLAYER_SIGNAL_COMMAND_EXECUTED, &cmd->m_Index );
			m_System->DPrintf( "Director Cmd %s, Time %.2f\n", cmd->GetName(), cmd->GetTime() );
		}

		cmd = (DirectorCmd*)m_Commands.GetNext();
	}
}

void DemoPlayer::NewGame( IWorld* world, IProxy* )
{
	static unsigned char cmd[] = { 1, DRC_CMD_START };

	m_World = world;
	m_PlayerTime = 1.0;
	m_PlayerState = DEMO_NEWGAME;
	m_Commands.Clear( true );
	m_LastCmd = NULL;
	FireSignal( DEMOPLAYER_SIGNAL_COMMANDS_CHANGED, NULL );

	if ( m_World->IsHLTV() || m_ForceHLTV )
		m_World->AddSignonData( ServerMessageDirector, cmd, sizeof( cmd ) );
}

void DemoPlayer::ShutDown()
{
	if ( m_State == MODULE_DISCONNECTED )
		return;

	if ( m_World )
		m_World->ShutDown();

	if ( m_Server )
		m_Server->ShutDown();

	m_DemoStream.Free();
	m_Commands.Clear( true );
	BaseSystemModule::ShutDown();
	m_System->DPrintf( "DemoPlayer module Shutdown.\n" );
}

void DemoPlayer::ReceiveSignal( ISystemModule* module, unsigned int signal, void* )
{
#ifdef _WIN32
	int serverSerial = m_Server->GetSerial();
	int senderSerial = module->GetSerial();
#else
	int senderSerial = module->GetSerial();
	int serverSerial = m_Server->GetSerial();
#endif
	if ( senderSerial == serverSerial )
	{
		if ( signal == SERVER_SIGNAL_DISCONNECT )
			m_System->Printf( "Demo file completely loaded.\n" );

		return;
	}

#ifdef _WIN32
	int worldSerial = m_World->GetSerial();
	senderSerial = module->GetSerial();
#else
	senderSerial = module->GetSerial();
	int worldSerial = m_World->GetSerial();
#endif
	if ( senderSerial != worldSerial )
		return;

	BitBuffer message( PAUSE_MESSAGE_CAPACITY );

	if ( signal == WORLD_SIGNAL_NEWGAME )
	{
		NewGame( m_World, NULL );
	}
	else if ( signal == WORLD_SIGNAL_RESET )
	{
		m_StartTime = 9999.0;
	}
	else if ( signal == WORLD_SIGNAL_PAUSE || signal == WORLD_SIGNAL_UNPAUSE )
	{
		message.WriteByte( ServerMessageSetPause );
		message.WriteByte( signal == WORLD_SIGNAL_PAUSE );
	}
}

int DemoPlayer::ReadDemoMessage( unsigned char* buffer, int size )
{
	int previous = m_LastFrameSeqNr;

	switch ( m_PlayerState )
	{
	case DEMO_INACTIVE:
	case DEMO_LOADING:
		return 0;

	case DEMO_NEWGAME:
		m_World->WriteNewData( &m_DemoStream );
		m_PlayerState = DEMO_SPAWNING;
		break;

	case DEMO_SPAWNING:
		m_LastFrameSeqNr = 0;
		m_DeltaFrameSeqNr = 0;
		WriteSpawn( &m_DemoStream );
		m_Engine->SetCvar( "spec_pip", "0" );
		m_StartTime = m_PlayerTime;
		m_WorldTime = 0.0;
		SetTimeScale( 1.0f );
		SetPaused( false );
		m_PlayerState = DEMO_PLAYING;
		break;

	case DEMO_PLAYING:
		WriteDatagram( &m_DemoStream );

		if ( previous < (int)m_LastFrameSeqNr )
		{
			int current = m_LastFrameSeqNr;

			for ( int sequence = previous + 1;; sequence++ )
			{
				frame_t* frame = m_World->GetFrameBySeqNr( sequence );

				if ( frame && frame->demoData && frame->demoDataSize )
				{
					BitBuffer commands( frame->demoData, frame->demoDataSize );
					ExecuteDemoFileCommands( &commands );
				}

				if ( sequence == current )
					break;
			}
		}
		break;

	default:
		break;
	}

	int length = m_DemoStream.CurrentSize();

	if ( length <= 0 )
		return 0;

	if ( length > size )
	{
		m_System->Printf( "ERROR! DemoPlayer::ReadDemoMessage: data overflow (%i bytes).\n", length );
		m_DemoStream.Clear();
		return 0;
	}

	memcpy( buffer, m_DemoStream.data, length );
	m_DemoStream.FastClear();
	return length;
}
