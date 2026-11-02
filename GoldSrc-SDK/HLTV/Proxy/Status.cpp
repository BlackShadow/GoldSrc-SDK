#include "Status.h"

#include "common/IBaseSystem.h"
#include "HLTV/common/Bitbuffer.h"
#include "HLTV/common/DirectorCmd.h"
#include "HLTV/common/IProxy.h"
#include "HLTV/common/IServer.h"
#include "HLTV/common/common.h"
#include "HLTV/common/mem.h"
#include "common/hltv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATUS_PRIVATE	0x8000

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
Status::Status()
{
}

Status::~Status()
{
}

bool Status::Init( IBaseSystem* system, int serial, char* name )
{
	BaseSystemModule::Init( system, serial, name );
#ifdef _WIN32
	_snprintf( m_Name, sizeof( m_Name ), "status" );
#else
	snprintf( m_Name, sizeof( m_Name ), "status" );
#endif
	m_Proxies.Init();
	m_System->RegisterCommand( "proxies", this, CMD_ID_PROXIES );
	Reset();
	m_State = MODULE_RUNNING;
	m_System->DPrintf( "Status module initialized.\n" );
	return true;
}

void Status::RunFrame( double time )
{
	BaseSystemModule::RunFrame( time );

	if ( m_MaxNumberOfSpectators < m_NumberOfSpectators )
		m_MaxNumberOfSpectators = m_NumberOfSpectators;

	if ( m_Proxy->IsMaster() && m_Proxy->IsActive() && m_SystemTime > m_NextUpdateTime )
	{
		GetLocalStats( m_NumberOfProxies, m_NumberOfSlots, m_NumberOfSpectators );
		SendStatusUpdate();

		IServer* server = m_Proxy->GetServer();
		server->SetUserInfo( "hspecs", COM_VarArgs( "%u", m_NumberOfSpectators ) );
		server->SetUserInfo( "hslots", COM_VarArgs( "%u", m_NumberOfSlots ) );
		m_NextUpdateTime = (float)( m_SystemTime + 16.0 );
	}
}

void Status::ExecuteCommand( int commandID, char* commandLine )
{
	switch ( commandID )
	{
	case CMD_ID_PROXIES:
		CMD_Proxies( commandLine );
		break;
	default:
		m_System->Printf( "ERROR! Status::ExecuteCommand: unknown command ID %i.\n", commandID );
		break;
	}
}

void Status::ShutDown()
{
	if ( m_State == MODULE_DISCONNECTED )
		return;

	m_Proxies.Clear( true );
	m_System->Printf( "Status module shutdown.\n" );
	BaseSystemModule::ShutDown();
}

void Status::GetGlobalStats( int& proxies, int& slots, int& spectators )
{
	proxies = m_NumberOfProxies;
	slots = m_NumberOfSlots;
	spectators = m_NumberOfSpectators;
}

void Status::SetProxy( IProxy* proxy )
{
	m_Proxy = proxy;
}

void Status::SendStatusUpdate()
{
	BitBuffer buffer( 128 );
	buffer.WriteByte( svc_hltv );
	buffer.WriteByte( HLTV_STATUS );
	buffer.WriteWord( m_NumberOfProxies );
	buffer.WriteLong( m_NumberOfSlots );
	buffer.WriteLong( m_NumberOfSpectators );
	m_Proxy->Broadcast( buffer.data, buffer.CurrentSize(), GROUP_PROXY, true );

	buffer.FastClear();
	DirectorCmd cmd;
	cmd.SetStatusData( m_NumberOfSlots, m_NumberOfSpectators, m_NumberOfProxies );
	cmd.WriteToStream( &buffer );
	m_Proxy->Broadcast( buffer.data, buffer.CurrentSize(), GROUP_CLIENT | GROUP_DEMO | GROUP_UNUSED, false );
}

char* Status::GetType()
{
	return "status000";
}

char* Status::GetStatusLine()
{
	static char string[256];
#ifdef _WIN32
	_snprintf( string, sizeof( string ), "Global Status: Proxies %i, Slots %i, Spectators %i (max %i)\n",
#else
	snprintf( string, sizeof( string ), "Global Status: Proxies %i, Slots %i, Spectators %i (max %i)\n",
#endif
			m_NumberOfProxies, m_NumberOfSlots, m_NumberOfSpectators, m_MaxNumberOfSpectators );
	return string;
}

void Status::Reset()
{
	m_Proxies.Clear( true );
	m_NextUpdateTime = 0.0f;
	m_NumberOfProxies = 0;
	m_NumberOfSlots = 0;
	m_NumberOfSpectators = 0;
	m_MaxNumberOfSpectators = 0;
}

void Status::ParseStatusMsg( BitBuffer* stream )
{
	if ( m_Proxy->IsMaster() )
	{
		m_System->Printf( "WARNING! Status::ParseStatusMsg: unexpected HLTV_STATUS message as Master.\n" );
		stream->SkipBytes( 10 );
		return;
	}

	m_NumberOfProxies = stream->ReadWord();
	m_NumberOfSlots = stream->ReadLong();
	m_NumberOfSpectators = stream->ReadLong();
	ReplyStatusReport();
	SendStatusUpdate();
}

void Status::ReplyStatusReport()
{
	BitBuffer data( 128 );
	int proxies = 0;
	int slots = 0;
	int spectators = 0;
	GetLocalStats( proxies, slots, spectators );

	if ( m_Proxy->IsPasswordProtected() )
	{
		proxies |= STATUS_PRIVATE;
	}
	else if ( m_Proxy->GetDispatchMode() == 0 )
	{
		slots = m_Proxy->GetMaxClients();
	}
	else if ( m_Proxy->IsStressed() )
	{
		proxies |= STATUS_PRIVATE;
	}

	data.WriteByte( HLTV_STATUS );
	data.WriteWord( proxies );
	data.WriteLong( slots );
	data.WriteLong( spectators );
	m_Proxy->GetServer()->SendHLTVCommand( &data );
}

int Status::GetMaxSpectatorNumber()
{
	return m_MaxNumberOfSpectators;
}

void Status::ParseStatusReport( NetAddress* from, BitBuffer* stream )
{
	int proxies = stream->ReadWord();
	int slots = stream->ReadLong();
	int spectators = stream->ReadLong();
	bool isPrivate = ( proxies & STATUS_PRIVATE ) != 0;
	proxies &= ~STATUS_PRIVATE;

	proxyInfo_t* proxy = (proxyInfo_t*)m_Proxies.GetFirst();
	while ( proxy && !from->Equal( &proxy->address ) )
		proxy = (proxyInfo_t*)m_Proxies.GetNext();

	if ( !proxy )
	{
		proxy = (proxyInfo_t*)Mem_ZeroMalloc( sizeof( proxyInfo_t ) );
		if ( !proxy )
		{
			m_System->Printf( "WARNING! Status::ParseStatusReport: not enough memory to increase proxy list.\n" );
			return;
		}

		proxy->address.FromNetAddress( from );
		m_Proxies.Add( proxy );
	}

	proxy->proxies = proxies;
	proxy->spectators = spectators;
	proxy->slots = slots;
	proxy->time = (float)m_SystemTime;
	proxy->isPrivate = isPrivate;

	float ratio = slots > 0 ? (float)spectators / (float)slots : 1.0f;
	m_Proxies.ChangeKey( proxy, isPrivate ? 1.0f : ratio );
}

void Status::GetLocalStats( int& proxies, int& slots, int& spectators )
{
	slots = 0;
	spectators = 0;
	proxies = 0;

	for ( proxyInfo_t* proxy = (proxyInfo_t*)m_Proxies.GetFirst(); proxy;
			proxy = (proxyInfo_t*)m_Proxies.GetNext() )
	{
		if ( m_SystemTime <= proxy->time + 64.0f )
		{
			proxies += proxy->proxies;
			slots += proxy->slots;
			spectators += proxy->spectators;
		}
		else
		{
			m_Proxies.Remove( proxy );
			free( proxy );
		}
	}

	int mySpecs;
	int myProxies;
	m_Proxy->CountLocalClients( mySpecs, myProxies );
	spectators += mySpecs;
	proxies++;
	slots += m_Proxy->GetMaxClients();
}

void Status::CMD_Proxies( char* )
{
	int count = 0;
	for ( proxyInfo_t* proxy = (proxyInfo_t*)m_Proxies.GetFirst(); proxy;
			proxy = (proxyInfo_t*)m_Proxies.GetNext() )
	{
		m_System->Printf( "IP %s, Clients %i, MaxClients %i%s\n", proxy->address.ToString(),
				proxy->spectators, proxy->slots, proxy->isPrivate ? " (excluded)" : "" );
		count++;
	}

	m_System->Printf( "--- Total %i relay proxies ---\n", count );
}

float Status::GetBestRelayProxy( NetAddress* addr )
{
	for ( proxyInfo_t* proxy = (proxyInfo_t*)m_Proxies.GetFirst(); proxy;
			proxy = (proxyInfo_t*)m_Proxies.GetNext() )
	{
		if ( proxy->slots > 0 && proxy->slots >= proxy->spectators && !proxy->isPrivate )
		{
			proxy->spectators++;
			float ratio = (float)proxy->spectators / (float)proxy->slots;
			m_Proxies.ChangeKey( proxy, ratio );
			addr->FromNetAddress( &proxy->address );
			return ratio;
		}

		m_Proxies.ChangeKey( proxy, 1.0f );
	}

	addr->Clear();
	return -1.0f;
}
