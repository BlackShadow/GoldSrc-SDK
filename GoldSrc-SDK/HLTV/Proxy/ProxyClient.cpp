#include "HLTV/Proxy/ProxyClient.h"
#include "HLTV/common/IProxy.h"
#include "HLTV/common/IServer.h"
#include "HLTV/common/IWorld.h"
#include "HLTV/common/NetPacket.h"
#include "HLTV/common/common.h"
#include "common/IBaseSystem.h"
#include "common/IObjectContainer.h"
#include "common/TokenLine.h"
#include "common/hltv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
ProxyClient::ProxyClient( IProxy* proxy )
{
	m_Proxy = proxy;
}

ProxyClient::~ProxyClient()
{
}

bool ProxyClient::Init( IBaseSystem* system, int serial, char* moduleName )
{
	BaseClient::Init( system, serial, moduleName );
	type = TYPE_CLIENT;
	chatEnabled = true;
	lastChatTime = 0.0f;
	lastCheerTime = 0.0f;
	nextDecalTime = 0.0f;
	return true;
}

bool ProxyClient::HasChatEnabled()
{
	return chatEnabled;
}

void ProxyClient::ParseHLTV( NetPacket* packet )
{
	unsigned char command = (unsigned char)packet->data.ReadByte();
	if ( command != HLTV_STATUS )
	{
		m_System->Printf( "WARNING! unknown HLTV client msg %i\n", command );
		return;
	}

	if ( type == TYPE_PROXY )
	{
		m_Proxy->ParseStatusReport( GetAddress(), &packet->data );
	}
	else
	{
		m_System->DPrintf( "WARNING! HLTV status data from spectator client\n" );
		packet->data.SkipBytes( 12 );
	}
}

void ProxyClient::ReplySpawn( int spawncount, int crcMap )
{
	BaseClient::ReplySpawn( spawncount, crcMap );
	if ( type == TYPE_PROXY )
		voiceQuery = false;

	m_Proxy->WriteSignonData( type, &clientChannel.reliableStream );
}

void ProxyClient::DownloadFile( char* fileName )
{
	if ( !fileName || !fileName[0] )
		return;

	if ( !strstr( fileName, ".." ) &&
			( strlen( fileName ) != 36 || strncasecmp( fileName, "!MD5", 4 ) ) )
	{
		resource_t* resource = m_Proxy->GetResource( fileName );
		if ( resource )
		{
			clientChannel.CreateFragmentsFromBuffer( resource->data, resource->nDownloadSize, 1, fileName );
			return;
		}

		if ( clientChannel.CreateFragmentsFromFile( fileName ) )
		{
			clientChannel.FragSend();
			return;
		}
	}

	DownloadFailed( fileName );
}

void ProxyClient::ShutDown()
{
	const char* clientTypeString[] =
	{
		"Spectator",
		"Relay Proxy",
		"Director",
		"Commentator",
		"Fake Client"
	};

	if ( m_State == MODULE_DISCONNECTED )
		return;

	m_System->Printf( "%s disconnected (%s)\n", clientTypeString[type], clientChannel.remote_address.ToString() );
	m_Proxy->GetClients()->Remove( this );
	BaseClient::ShutDown();
	delete this;
}

bool ProxyClient::ProcessStringCmd( char* string )
{
	if ( BaseClient::ProcessStringCmd( string ) )
		return true;

	TokenLine cmdLine;
	if ( !cmdLine.SetLine( string ) )
	{
		m_System->Printf( "WARNING! Client::ParseStringCmd: string command too long.\n" );
		return true;
	}

	char* command = cmdLine.GetToken( 0 );
	if ( !strcasecmp( command, "cheer" ) )
	{
		if ( m_SystemTime > lastCheerTime + 6.0f )
		{
			lastCheerTime = (float)m_SystemTime;
			m_Proxy->IncreaseCheering( 1 );
		}
	}
	else if ( !strcasecmp( command, "say" ) )
	{
		char* text;
		if ( cmdLine.CountToken() <= 2 )
			text = cmdLine.GetToken( 1 );
		else
			text = cmdLine.GetRestOfLine( 1 );

		if ( type == TYPE_COMMENTATOR )
		{
			m_Proxy->ChatCommentator( NULL, text );
		}
		else if ( type == TYPE_PROXY && m_Proxy->GetChatMode() == 2 )
		{
			m_Proxy->ChatSpectator( "Unknown", text );
		}
		else if ( lastChatTime + 6.0f <= m_SystemTime )
		{
			m_Proxy->ChatSpectator( name, text );
			lastChatTime = (float)m_SystemTime;
		}
	}
	else if ( !strcasecmp( command, "joingame" ) )
	{
		if ( m_Proxy->GetServer()->IsDemoFile() )
		{
			PrintfToClient( "Proxy is replaying demo.\n" );
		}
		else if ( m_Proxy->IsPublicGame() )
		{
			char commandLine[64];
#ifdef _WIN32
			_snprintf( commandLine, sizeof( commandLine ), "connect %s\n", m_World->GetGameServerAddress()->ToString() );
#else
			snprintf( commandLine, sizeof( commandLine ), "connect %s\n", m_World->GetGameServerAddress()->ToString() );
#endif
			clientChannel.reliableStream.WriteByte( svc_stufftext );
			clientChannel.reliableStream.WriteString( commandLine );
		}
		else
		{
			PrintfToClient( "Joining game is not allowed.\n" );
		}
	}
	else if ( !strcasecmp( command, "status" ) )
	{
		PrintfToClient( "--- HLTV Status ---\n" );
		if ( m_Proxy->GetServer()->IsDemoFile() )
		{
			PrintfToClient( "Replay demo file %s\n", m_Proxy->GetServer()->GetDemoFileName() );
		}
		else if ( m_Proxy->IsPublicGame() )
		{
			PrintfToClient( "Game Server: %s\n", m_World->GetGameServerAddress()->ToString() );
		}
		else
		{
			PrintfToClient( "Private game server\n" );
		}

		int proxies, slots, spectators;
		m_Proxy->GetStatistics( proxies, slots, spectators );
		PrintfToClient( "Global HLTV stats: spectators %i, slots %i, proxies %i\n", spectators, slots, proxies );
	}
	else if ( !strcasecmp( command, "ignoremsg" ) )
	{
		if ( cmdLine.CountToken() == 2 )
		{
			chatEnabled = atoi( cmdLine.GetToken( 1 ) ) != 1;
			if ( chatEnabled )
				PrintfToClient( "Spectator chat enabled.\n" );
			else
				PrintfToClient( "Spectator chat disabled.\n" );
		}
	}
	else
	{
		if ( type < TYPE_COMMENTATOR )
			m_System->DPrintf( "Unkown client command: \"%s\"\n", command );

		m_System->DPrintf( "Unkown director command: \"%s\"\n", command );
		return false;
	}

	return true;
}

void ProxyClient::UpdateUserInfo( char* userinfostring )
{
	BaseClient::UpdateUserInfo( userinfostring );
	if ( type < TYPE_CLIENT || type > TYPE_DEMO )
	{
		m_System->DPrintf( "WARNING! Client::UpdateUserInfo: invalid client ype %i\n", type );
		type = TYPE_CLIENT;
	}
	else if ( type != TYPE_CLIENT )
	{
		return;
	}

	if ( clientChannel.GetRate() > m_Proxy->GetMaxRate() )
		clientChannel.SetRate( m_Proxy->GetMaxRate() );

	if ( type == TYPE_CLIENT && clientChannel.GetUpdateRate() > m_Proxy->GetMaxUpdateRate() )
		clientChannel.SetUpdateRate( m_Proxy->GetMaxUpdateRate() );
}

void ProxyClient::SendDatagram()
{
	frame_t* frame;
	double time;
	if ( m_Proxy->GetDelay() > 0.0f )
	{
		double worldTime = m_Proxy->GetSpectatorTime();
		double proxyTime = m_Proxy->GetProxyTime();
		frame = m_World->GetFrameByTime( worldTime );
		if ( !frame )
			return;

		time = proxyTime - ( worldTime - frame->time );
		if ( clientChannel.GetIdleTime() > 2.0f )
		{
			clientChannel.unreliableStream.WriteByte( svc_centerprint );
			if ( frame->seqnr <= 1 )
				clientChannel.unreliableStream.WriteString( "Buffering game..." );
			else
				clientChannel.unreliableStream.WriteString( "Game pending..." );
		}
	}
	else
	{
		frame = m_World->GetLastFrame();
		if ( !frame )
			return;

		time = frame->time;
		if ( clientChannel.GetIdleTime() > 2.0f )
		{
			clientChannel.unreliableStream.WriteByte( svc_centerprint );
			clientChannel.unreliableStream.WriteString( "Game pending..." );
		}
	}

	WriteDatagram( time, frame );
}

void ProxyClient::ParseVoiceData( NetPacket* packet )
{
	int length = packet->data.ReadShort();
	BitBuffer voiceMsg( length + 8 );
	if ( type == TYPE_COMMENTATOR )
	{
		if ( length > 4096 )
		{
			Disconnect( "Invalid voice message.\n" );
			return;
		}

		voiceMsg.WriteByte( svc_voicedata );
		voiceMsg.WriteByte( m_World->GetSlotNumber() );
		voiceMsg.WriteShort( length );
		voiceMsg.WriteBuf( packet->data.currentByte, length );
		m_Proxy->Broadcast( voiceMsg.data, voiceMsg.CurrentSize(),
				GROUP_PROXY | GROUP_DEMO | GROUP_UNUSED | GROUP_VOICE, false );
	}

	packet->data.SkipBytes( length );
}
