#include "BaseClient.h"

#include "HLTV/common/IWorld.h"
#include "HLTV/common/INetSocket.h"
#include "HLTV/common/NetPacket.h"
#include "HLTV/common/common.h"
#include "HLTV/common/munge.h"
#include "common/IBaseSystem.h"
#include "common/TokenLine.h"
#include "common/hltv.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const BaseClient::clc_func_t BaseClient::PARSEFUNCS[] =
{
	{ clc_bad, "clc_bad", &BaseClient::ParseBad },
	{ clc_nop, "clc_nop", &BaseClient::ParseNop },
	{ clc_move, "clc_move", &BaseClient::ParseMove },
	{ clc_stringcmd, "clc_stringcmd", &BaseClient::ParseStringCmd },
	{ clc_delta, "clc_delta", &BaseClient::ParseDelta },
	{ clc_resourcelist, "clc_resourcelist", NULL },
	{ clc_tmove, "clc_tmove", NULL },
	{ clc_fileconsistency, "clc_fileconsistency", NULL },
	{ clc_voicedata, "clc_voicedata", &BaseClient::ParseVoiceData },
	{ clc_hltv, "clc_hltv", &BaseClient::ParseHLTV },
	{ clc_cvarvalue, "clc_cvarvalue", &BaseClient::ParseCvarValue },
	{ clc_cvarvalue2, "clc_cvarvalue2", &BaseClient::ParseCvarValue2 }
};

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
BaseClient::BaseClient()
{
}

BaseClient::~BaseClient()
{
}

bool BaseClient::Init( IBaseSystem* system, int serial, char* moduleName )
{
	if ( !BaseSystemModule::Init( system, serial, moduleName ) )
		return false;

	if ( moduleName == NULL )
	{
#ifdef _WIN32
		_snprintf( m_Name, sizeof( m_Name ), "client001" );
#else
		snprintf( m_Name, sizeof( m_Name ), "client001" );
#endif
	}

	SetState( CLIENT_INITILAZING );
	type = TYPE_CLIENT;
	voiceEnabled = false;
	voiceQuery = false;
	m_Socket = NULL;
	m_World = NULL;
	userinfo.SetMaxSize( 256 );
	return true;
}

char* BaseClient::GetClientName()
{
	return name;
}

InfoString* BaseClient::GetUserInfo()
{
	return &userinfo;
}

bool BaseClient::IsActive()
{
	return state == CLIENT_RUNNING;
}

bool BaseClient::IsHearingVoices()
{
	return !BanMask[m_World->GetSlotNumber()] && voiceEnabled;
}

bool BaseClient::HasChatEnabled()
{
	return false;
}

int BaseClient::GetClientType()
{
	return type;
}

char* BaseClient::GetType()
{
	return "client001";
}

void BaseClient::QueryVoiceEnabled( BitBuffer* stream )
{
	int msg = m_World->FindUserMsgByName( "ReqState" );
	if ( msg && m_World->IsVoiceEnabled() )
		stream->WriteByte( msg );
}

void BaseClient::ParseStringCmd( NetPacket* packet )
{
	ProcessStringCmd( packet->data.ReadString() );
}

void BaseClient::ParseNop( NetPacket* )
{
}

void BaseClient::ParseCvarValue( NetPacket* packet )
{
	packet->data.ReadString();
}

void BaseClient::PrintfToClient( char* fmt, ... )
{
	static char string[1024];
	va_list args;
	va_start( args, fmt );
#ifdef _WIN32
	_vsnprintf( string, sizeof( string ), fmt, args );
#else
	vsnprintf( string, sizeof( string ), fmt, args );
#endif
	va_end( args );

	clientChannel.reliableStream.WriteByte( svc_print );
	clientChannel.reliableStream.WriteString( string );
}

void BaseClient::WriteSpawn( BitBuffer* stream )
{
	stream->WriteByte( svc_time );
	stream->WriteFloat( 1.0f );
	for ( int i = 0; i < m_World->GetMaxClients(); i++ )
		m_World->WriteClientUpdate( stream, i );

	m_World->WriteLightStyles( stream );
	stream->WriteByte( svc_signonnum );
	stream->WriteByte( 1 );
}

void BaseClient::ParseMove( NetPacket* packet )
{
	int size = packet->data.ReadByte();
	packet->data.SkipBytes( size + 1 );
}

void BaseClient::ParseVoiceData( NetPacket* packet )
{
	int size = packet->data.ReadShort();
	packet->data.SkipBytes( size );
}

void BaseClient::ParseCvarValue2( NetPacket* packet )
{
	packet->data.ReadWord();
	packet->data.ReadString();
	packet->data.ReadString();
}

void BaseClient::Send( unsigned char* data, int length, bool isReliable )
{
	if ( isReliable )
		clientChannel.reliableStream.WriteBuf( data, length );
	else
		clientChannel.unreliableStream.WriteBuf( data, length );
}

void BaseClient::SetName( char* newName )
{
	char temp[1024];
	COM_RemoveEvilChars( newName );
	COM_TrimSpace( newName, temp );
	if ( strlen( temp ) > sizeof( name ) - 1 )
		temp[sizeof( name )] = 0;

	if ( temp[0] == 0 || !strcasecmp( temp, "console" ) )
		strcpy( temp, "unnamed" );

	userinfo.SetValueForKey( "name", temp );
	strncpy( name, temp, sizeof( name ) - 1 );
	name[sizeof( name ) - 1] = 0;
}

void BaseClient::UpdateVoiceMask( BitBuffer* stream )
{
	int msg = m_World->FindUserMsgByName( "VoiceMask" );
	if ( !msg || !m_World->IsVoiceEnabled() )
	{
		m_System->DPrintf( "WARNING! Client::UpdateVoiceMask(): could find User Msg VoiceMaks.\n" );
		return;
	}

	CPlayerBitVec gameRulesMask;
	gameRulesMask[m_World->GetSlotNumber()] = voiceEnabled;
	if ( gameRulesMask != sentGameRulesMask || BanMask != sentBanMask )
	{
		sentBanMask = BanMask;
		sentGameRulesMask = gameRulesMask;
		stream->WriteByte( msg );
		for ( int i = 0; i < VOICE_MAX_PLAYERS_DW; i++ )
		{
			stream->WriteLong( gameRulesMask.GetDWord( i ) );
			stream->WriteLong( BanMask.GetDWord( i ) );
		}
	}
}

void BaseClient::ParseDelta( NetPacket* packet )
{
	if ( state == CLIENT_RUNNING )
	{
		clientDelta = packet->data.ReadByte();
		deltaFrameSeqNr = SeqNrMap[clientDelta];
	}
	else
	{
		packet->data.SkipBytes( 1 );
	}
}

void BaseClient::DownloadFailed( char* fileName )
{
	if ( fileName == NULL || !fileName[0] )
		return;

	clientChannel.reliableStream.WriteByte( svc_filetxferfailed );
	clientChannel.reliableStream.WriteString( fileName );
}

void BaseClient::DownloadFile( char* fileName )
{
	if ( fileName && fileName[0] )
		DownloadFailed( fileName );
}

char* BaseClient::GetStatusLine()
{
	static char string[256];
	float in, out;
	clientChannel.GetFlowStats( &in, &out );
#ifdef _WIN32
	_snprintf( string, sizeof( string ), "ID: %i, Name \"%s\", Time %s, IP %s, In %.2f, Out %.2f.\n",
#else
	snprintf( string, sizeof( string ), "ID: %i, Name \"%s\", Time %s, IP %s, In %.2f, Out %.2f.\n",
#endif
			BaseSystemModule::GetSerial(), name, COM_FormatTime( (float)( m_SystemTime - clientChannel.connect_time ) ),
			clientChannel.GetTargetAddress()->ToString(), in, out );
	return string;
}

NetAddress* BaseClient::GetAddress()
{
	return clientChannel.GetTargetAddress();
}

void BaseClient::UpdateUserInfo( char* userinfostring )
{
	if ( userinfostring )
		userinfo.SetString( userinfostring );

	char buffer[1024];
	strncpy( buffer, userinfo.ValueForKey( "name" ), sizeof( buffer ) - 1 );
	buffer[sizeof( buffer ) - 1] = 0;
	SetName( buffer );

	type = atoi( userinfo.ValueForKey( "*hltv" ) );
	if ( type < 0 )
	{
		m_System->DPrintf( "WARNING! BaseClient::UpdateUserInfo: invalid client ype %i\n", type );
		type = TYPE_CLIENT;
	}

	char* value = userinfo.ValueForKey( "rate" );
	if ( value[0] )
		clientChannel.SetRate( atoi( value ) );

	value = userinfo.ValueForKey( "cl_updaterate" );
	if ( value[0] )
		clientChannel.SetUpdateRate( atoi( value ) );
}

void BaseClient::ReplyNew()
{
	BitBuffer msg( 65536 );
	Reset();
	m_World->WriteNewData( &msg );
	clientChannel.CreateFragmentsFromBuffer( msg.data, msg.CurrentSize(), 0, NULL );
	clientChannel.FragSend();
	msg.Free();
}

void BaseClient::ReplyFullUpdate()
{
	BitBuffer msg( 9216 );
	int maxClients = m_World->GetMaxClients();
	for ( int i = 0; i < maxClients; i++ )
		m_World->WriteClientUpdate( &msg, i );

	clientChannel.CreateFragmentsFromBuffer( msg.data, msg.CurrentSize(), 0, NULL );
	clientChannel.FragSend();
	msg.Free();
}

void BaseClient::ReplySpawn( int spawncount, int crcMap )
{
	BitBuffer msg( 65536 );
	crcValue = crcMap;
	COM_UnMunge2( (unsigned char*)&crcValue, sizeof( crcValue ), (unsigned char)~m_World->GetServerCount() );
	if ( m_World->GetServerCount() != spawncount )
	{
		ReplyNew();
		return;
	}

	m_World->WriteSigonData( &msg );
	WriteSpawn( &msg );
	clientChannel.CreateFragmentsFromBuffer( msg.data, msg.CurrentSize(), 0, NULL );
	clientChannel.FragSend();
	lastFrameSeqNr = 0;
}

void BaseClient::Disconnect( char* reason )
{
	SetState( CLIENT_DISCONNECTED );
	clientChannel.reliableStream.WriteByte( svc_disconnect );
	clientChannel.reliableStream.WriteString( reason ? reason : "" );
	clientChannel.unreliableStream.WriteByte( svc_disconnect );
	clientChannel.unreliableStream.WriteString( reason ? reason : "" );
	clientChannel.TransmitOutgoing();
}

void BaseClient::ParseHLTV( NetPacket* )
{
	Disconnect( "TODO Client::ParseHLTV: Invalid client command.\n" );
}

void BaseClient::ParseBad( NetPacket* )
{
	m_System->Printf( "Client::ParseBad: bad command char\n" );
	Disconnect( "Bad command character in client command" );
}

void BaseClient::ShutDown()
{
	if ( m_State == MODULE_DISCONNECTED )
		return;

	if ( state != CLIENT_DISCONNECTED )
		Disconnect( "Connection dropped (hard).\n" );

	clientChannel.Close();
	BaseSystemModule::ShutDown();
}

void BaseClient::ProcessMessage( NetPacket* packet )
{
	clientDelta = 0;
	deltaFrameSeqNr = 0;
	while ( !packet->data.sizeError )
	{
		int command = packet->data.ReadByte();
		if ( command == -1 )
			return;

		if ( (unsigned int)command >= sizeof( PARSEFUNCS ) / sizeof( PARSEFUNCS[0] ) )
		{
			m_System->Printf( "Client::ProcessMessage: unknown command char\n" );
			Disconnect( "Bad command character in client command" );
			return;
		}

		const clc_func_t* parser = &PARSEFUNCS[command];
		if ( !parser->func )
		{
			m_System->Printf( "TODO! Client::ProcessMessage: missing parsing routine for %s.\n", parser->pszname );
			return;
		}

		( this->*parser->func )( packet );
	}

	m_System->Printf( "Client::ProcessMessage: packet read overflow\n" );
	Disconnect( "Dropped due to bad message format!\n" );
}

void BaseClient::RunFrame( double time )
{
	BaseSystemModule::RunFrame( time );
	if ( state != CLIENT_DISCONNECTED )
	{
		if ( clientChannel.IsTimedOut() )
		{
			m_System->Printf( "Client %s timed out.\n", clientChannel.GetTargetAddress()->ToString() );
			Disconnect( "Timed out.\n" );
		}

		if ( state != CLIENT_DISCONNECTED && clientChannel.IsCrashed() )
		{
			m_System->Printf( "Client %s netchannel crashed.\n", clientChannel.GetTargetAddress()->ToString() );
			Disconnect( "Netchannel crashed.\n" );
		}
	}

	int count = 0;
	NetPacket* packet;
	while ( ( packet = clientChannel.GetPacket() ) != NULL )
	{
		if ( packet->connectionless )
		{
			m_Socket->AddPacket( packet );
		}
		else if ( count < 32 )
		{
			count++;
			ProcessMessage( packet );
		}

		clientChannel.FreePacket( packet );
	}

	switch ( state )
	{
	case CLIENT_INITILAZING:
	case CLIENT_CONNECTING:
		break;

	case CLIENT_RUNNING:
		if ( m_World->IsActive() && clientChannel.IsReadyToSend() )
			SendDatagram();
		break;

	case CLIENT_DISCONNECTED:
		ShutDown();
		break;

	default:
		m_System->Errorf( "Client::RunFrame: not valid state.\n" );
		break;
	}
}

void BaseClient::SetState( ClientState newState )
{
	if ( state == newState )
		return;

	bool invalid = false;
	switch ( newState )
	{
	case CLIENT_INITILAZING:
		break;

	case CLIENT_CONNECTING:
		invalid = state != CLIENT_INITILAZING && state != CLIENT_RUNNING;
		clientChannel.SetKeepAlive( true );
		clientChannel.SetTimeOut( 60.0f );
		break;

	case CLIENT_RUNNING:
		invalid = state != CLIENT_CONNECTING;
		clientChannel.SetKeepAlive( false );
		clientChannel.SetTimeOut( 30.0f );
		break;

	case CLIENT_DISCONNECTED:
		clientChannel.SetKeepAlive( true );
		break;

	default:
		invalid = true;
		break;
	}

	if ( invalid )
	{
		m_System->Errorf( "Client::SetState: not valid state (%i -> %i).\n", state, newState );
		return;
	}

	state = newState;
}

bool BaseClient::Connect( INetSocket* socket, NetAddress* adr, char* userinfostring )
{
	if ( adr == NULL || userinfostring == NULL )
		return false;

	type = TYPE_CLIENT;
	m_Socket = socket;
	if ( adr->Equal( clientChannel.GetTargetAddress() ) )
	{
		clientChannel.Clear();
		clientChannel.Reset();
	}
	else
	{
		clientChannel.Create( m_System, m_Socket, adr );
	}

	SetState( CLIENT_CONNECTING );
	clientChannel.SetUpdateRate( 20 );
	clientChannel.SetRate( 20000 );
	clientChannel.SetConnected( true );
	UpdateUserInfo( userinfostring );
	clientChannel.OutOfBandPrintf( "%c0000000000000000", 'B' );
	return true;
}

void BaseClient::Reset()
{
	SetState( CLIENT_CONNECTING );
	deltaFrameSeqNr = 0;
	clientDelta = 0;
	lastFrameSeqNr = 0;
	memset( SeqNrMap, 0, sizeof( SeqNrMap ) );
	voiceQuery = true;
	clientChannel.Clear();
}

void BaseClient::Reconnect()
{
	Reset();
	clientChannel.reliableStream.WriteByte( svc_stufftext );
	clientChannel.reliableStream.WriteString( "reconnect\n" );
}

void BaseClient::SetWorld( IWorld* world )
{
	if ( m_World != world && m_World != NULL )
		Reconnect();

	m_World = world;
}

bool BaseClient::ProcessStringCmd( char* string )
{
	TokenLine cmdLine;
	if ( !cmdLine.SetLine( string ) )
	{
		m_System->Printf( "WARNING! Client::ParseStringCmd: string command too long.\n" );
		return false;
	}

	char* command = cmdLine.GetToken( 0 );
	if ( !strcasecmp( command, "name" ) )
	{
		SetName( cmdLine.GetRestOfLine( 1 ) );
	}
	else if ( !strcasecmp( command, "spawn" ) )
	{
		if ( cmdLine.CountToken() == 3 )
		{
			int crc = atoi( cmdLine.GetToken( 2 ) );
			ReplySpawn( atoi( cmdLine.GetToken( 1 ) ), crc );
		}
		else
		{
			m_System->Printf( "Client::ParseStringCmd: spawn is not valid\n" );
			Disconnect( "Spawn is not valid." );
		}
	}
	else if ( !strcasecmp( command, "new" ) )
	{
		ReplyNew();
	}
	else if ( !strcasecmp( command, "sendres" ) )
	{
	}
	else if ( !strcasecmp( command, "fullupdate" ) )
	{
		ReplyFullUpdate();
	}
	else if ( !strcasecmp( command, "dropclient" ) )
	{
		Disconnect( NULL );
	}
	else if ( !strcasecmp( command, "ping" ) )
	{
	}
	else if ( !strcasecmp( command, "dlfile" ) )
	{
		DownloadFile( cmdLine.GetToken( 1 ) );
	}
	else if ( !strcasecmp( command, "setinfo" ) )
	{
		if ( cmdLine.CountToken() == 3 )
		{
			char* value = cmdLine.GetToken( 2 );
			userinfo.SetValueForKey( cmdLine.GetToken( 1 ), value );
			UpdateUserInfo( NULL );
		}
	}
	else if ( !strcasecmp( command, "showinfo" ) )
	{
		PrintfToClient( "%s\n", m_World->GetServerInfoString() );
	}
	else if ( !strcasecmp( command, "sendents" ) )
	{
		m_System->DPrintf( "Client fully connected.\n" );
		SetState( CLIENT_RUNNING );
	}
	else if ( !strcasecmp( command, "spectate" ) || !strcasecmp( command, "spk" ) ||
			!strcasecmp( command, "unpause" ) || !strcasecmp( command, "setpause" ) || !strcasecmp( command, "pause" ) )
	{
	}
	else if ( !strcasecmp( command, "VModEnable" ) )
	{
		if ( cmdLine.CountToken() != 2 )
		{
			Disconnect( "VModEnable with wrong parameters.\n" );
			return true;
		}

		voiceEnabled = atoi( cmdLine.GetToken( 1 ) ) != 0;
		voiceQuery = false;
		UpdateVoiceMask( &clientChannel.reliableStream );
	}
	else if ( !strcasecmp( command, "vban" ) )
	{
		if ( cmdLine.CountToken() <= 1 )
			return true;

		for ( int i = 1; i < cmdLine.CountToken(); i++ )
		{
			uint32 mask = 0;
			sscanf( cmdLine.GetToken( i ), "%x", &mask );
			if ( i <= VOICE_MAX_PLAYERS_DW )
				BanMask.SetDWord( i - 1, mask );
		}

		UpdateVoiceMask( &clientChannel.reliableStream );
	}
	else
	{
		return false;
	}

	return true;
}

void BaseClient::WriteDatagram( double time, frame_s* frame )
{
	float idleTime = clientChannel.GetIdleTime();
	if ( frame == NULL )
		return;

	if ( lastFrameSeqNr == 0 || lastFrameSeqNr > frame->seqnr )
	{
		lastFrameSeqNr = frame->seqnr - 1;
		clientDelta = 0;
		deltaFrameSeqNr = 0;
	}

	if ( !( frame->seqnr > lastFrameSeqNr || idleTime > 2.0f ) )
		return;

	clientChannel.unreliableStream.WriteByte( svc_time );
	clientChannel.unreliableStream.WriteFloat( (float)time );
	m_World->WriteFrame( frame, lastFrameSeqNr, &clientChannel.reliableStream, &clientChannel.unreliableStream,
			deltaFrameSeqNr, clientDelta, IsHearingVoices() );
	if ( voiceQuery )
		QueryVoiceEnabled( &clientChannel.unreliableStream );

	if ( clientChannel.reliableStream.sizeError )
	{
		Disconnect( "Reliable data stream overflow." );
		return;
	}

	if ( clientChannel.unreliableStream.sizeError )
	{
		m_System->DPrintf( "Unreliable data stream overflow.\n" );
		clientChannel.unreliableStream.Clear();
		lastFrameSeqNr = 0;
	}

	lastFrameSeqNr = frame->seqnr;
	SeqNrMap[clientChannel.outgoing_sequence & 255] = frame->seqnr;
	clientChannel.TransmitOutgoing();
}

void BaseClient::SendDatagram()
{
	frame_t* frame = m_World->GetLastFrame();
	if ( frame )
		WriteDatagram( frame->time, frame );
}
