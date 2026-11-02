#include "Master.h"

#include "Proxy.h"
#include "common/IBaseSystem.h"
#include "common/TokenLine.h"
#include "HLTV/common/INetwork.h"
#include "HLTV/common/INetSocket.h"
#include "HLTV/common/IWorld.h"
#include "HLTV/common/NetAddress.h"
#include "HLTV/common/common.h"

#include "external/steamworks/isteamgameserver.h"
#include "external/steamworks/steam_gameserver.h"

#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	unsigned int	iAppID;
	const char*		pGameDir;
} GameToAppIDMapItem_t;

GameToAppIDMapItem_t g_GameToAppIDMap[] =
{
	{ 10, "cstrike" },
	{ 20, "tfc" },
	{ 30, "dod" },
	{ 40, "dmc" },
	{ 50, "gearbox" },
	{ 60, "ricochet" },
	{ 70, "valve" },
	{ 80, "czero" },
	{ 100, "czeror" },
	{ 130, "bshift" },
	{ 150, "cstrike_beta" }
};

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
Master::Master()
{
	m_bSteamInitialized = false;
	m_flMasterUpdateTime = 0.0;
}

Master::~Master()
{
}

int GetGameAppID( const char* gameDir )
{
	for ( unsigned int i = 0; i < sizeof( g_GameToAppIDMap ) / sizeof( g_GameToAppIDMap[0] ); i++ )
	{
		if ( !strcasecmp( g_GameToAppIDMap[i].pGameDir, gameDir ) )
			return g_GameToAppIDMap[i].iAppID;
	}

	return 70;
}

bool Master::Init( IBaseSystem* system, int serial, char* name )
{
	BaseSystemModule::Init( system, serial, name );
	m_MasterSocket = m_Proxy->m_Socket;

	m_System->RegisterCommand( "heartbeat", this, CMD_ID_HEARTBEAT );
	m_System->RegisterCommand( "nomaster", this, CMD_ID_NOMASTER );
	m_System->RegisterCommand( "listmaster", this, CMD_ID_LISTMASTER );

	m_NoMaster = false;
	m_bMasterLoaded = false;
	m_State = MODULE_RUNNING;

	if ( !m_MasterSocket->GetNetwork()->GetLocalAddress() )
	{
		m_System->Printf( "Master module failed to initialize (no net).\n" );
		return false;
	}

	m_bSteamInitialized = false;
	m_System->Printf( "Master module initialized.\n" );
	return true;
}

void Master::RunFrame( double time )
{
	BaseSystemModule::RunFrame( time );

	static double s_flLastRunCallbacks = 0.0;
	if ( time - s_flLastRunCallbacks > 0.1f )
	{
		SteamGameServer_RunCallbacks();
		s_flLastRunCallbacks = time;
	}

	if ( m_NoMaster )
		return;

	if ( m_Proxy->IsActive() )
	{
		if ( !m_bSteamInitialized && m_Proxy->GetWorld() )
		{
			int appID = GetGameAppID( m_Proxy->GetWorld()->GetGameDir() );
			if ( appID > 0 )
			{
				FILE* file = fopen( "steam_appid.txt", "w+" );
				if ( file )
				{
					fprintf( file, "%d\n", appID );
					fclose( file );
				}
			}

			NetAddress* localAddress = m_MasterSocket->GetNetwork()->GetLocalAddress();
			uint32 ip;
			memcpy( &ip, localAddress->m_IP, sizeof( ip ) );
			if ( SteamInternal_GameServer_Init( htonl( ip ), 0, 0, STEAMGAMESERVER_QUERY_PORT_SHARED,
					eServerModeNoAuthentication, "1.1.2.6" ) )
			{
				SteamGameServer()->SetProduct( "hltv" );
				SteamGameServer()->SetGameDescription( "hltv" );
				SteamGameServer()->SetModDir( m_Proxy->GetWorld()->GetGameDir() );
				SteamGameServer()->SetDedicatedServer( true );
				SteamGameServer()->SetSpectatorPort( htons( localAddress->m_Port ) );
				SteamGameServer()->LogOnAnonymous();
				SteamGameServer()->SetAdvertiseServerActive( true );
			}
			else
			{
				m_System->Printf( "Master module failed to initialize. (init failed)\n" );
			}

			m_bSteamInitialized = SteamGameServer() != NULL;
		}

		if ( !m_bSteamInitialized )
			return;

		if ( m_System->GetTime() > m_flMasterUpdateTime )
		{
			m_flMasterUpdateTime = m_System->GetTime() + 5.0;

			char mapName[260];
			char hostName[260];
			COM_FileBase( m_Proxy->GetWorld()->GetLevelName(), mapName );
			strncpy( hostName, m_Proxy->GetWorld()->GetHostName(), sizeof( hostName ) );

			int proxies;
			int slots;
			int spectators;
			m_Proxy->GetStatistics( proxies, slots, spectators );

			SteamGameServer()->SetMaxPlayerCount( slots < 127 ? slots : 127 );
			SteamGameServer()->SetServerName( hostName );
			SteamGameServer()->SetMapName( mapName );
			SteamGameServer()->SetPasswordProtected( m_Proxy->IsPasswordProtected() );
			SteamGameServer()->SetModDir( m_Proxy->GetWorld()->GetGameDir() );
		}
	}

	if ( m_bSteamInitialized )
	{
		char outBuffer[4096];
		uint32 ip;
		uint16 port;
		int length = SteamGameServer()->GetNextOutgoingPacket( outBuffer, sizeof( outBuffer ), &ip, &port );
		while ( length > 0 )
		{
			ip = htonl( ip );
			NetAddress address;
			memcpy( address.m_IP, &ip, sizeof( address.m_IP ) );
			address.m_Port = htons( port );
			m_MasterSocket->SendPacket( &address, outBuffer, length );
			length = SteamGameServer()->GetNextOutgoingPacket( outBuffer, sizeof( outBuffer ), &ip, &port );
		}
	}
}

void Master::ShutDown()
{
	if ( m_State == MODULE_DISCONNECTED )
		return;

	if ( SteamGameServer() )
		SteamGameServer()->LogOff();

	BaseSystemModule::ShutDown();
	SteamGameServer_Shutdown();
	m_System->Printf( "Master module shutdown.\n" );
}

void Master::CMD_Heartbeat( char* )
{
}

void Master::CMD_NoMaster( char* cmdLine )
{
	if ( m_State == MODULE_DISCONNECTED || !SteamGameServer() )
		return;

	TokenLine params( cmdLine );
	if ( params.CountToken() != 2 )
	{
		m_System->Printf( "Syntax: nomaster <0|1>\n" );
		m_System->Printf( "Master server notification is %s.\n", m_NoMaster ? "disabled" : "enabled" );
		return;
	}

	bool oldNoMaster = m_NoMaster;
	m_NoMaster = atoi( params.GetToken( 1 ) ) != 0;
	if ( oldNoMaster != m_NoMaster )
		SteamGameServer()->SetAdvertiseServerActive( m_NoMaster );
}

void Master::CMD_ListMaster( char* )
{
	if ( m_State != MODULE_DISCONNECTED )
		m_System->Printf( "No longer used\n" );
}

void Master::ExecuteCommand( int commandID, char* commandLine )
{
	switch ( commandID )
	{
	case CMD_ID_HEARTBEAT:
		CMD_Heartbeat( commandLine );
		break;
	case CMD_ID_NOMASTER:
		CMD_NoMaster( commandLine );
		break;
	case CMD_ID_LISTMASTER:
		CMD_ListMaster( commandLine );
		break;
	default:
		m_System->Printf( "ERROR! Master::ExecuteCommand: unknown command ID %i.\n", commandID );
		break;
	}
}

char* Master::GetType()
{
	return "master000";
}

void Master::SendShutdown()
{
	if ( m_State != MODULE_DISCONNECTED && SteamGameServer() )
		SteamGameServer()->SetAdvertiseServerActive( false );
}

char* Master::GetStatusLine()
{
	static char string[256];
#ifdef _WIN32
	_snprintf( string, sizeof( string ), "Master servers: 0\n" );
#else
	snprintf( string, sizeof( string ), "Master servers: 0\n" );
#endif
	return string;
}
