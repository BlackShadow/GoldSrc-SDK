#include "FakeClient.h"

#include "common/IBaseSystem.h"
#include "HLTV/common/INetwork.h"
#include "HLTV/common/IWorld.h"
#include "HLTV/common/IServer.h"
#include "HLTV/common/INetSocket.h"
#include "HLTV/common/NetPacket.h"

#include <stdio.h>

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
FakeClient::FakeClient()
{
}

FakeClient::~FakeClient()
{
}

void FakeClient::RunFrame( double time )
{
	BaseSystemModule::RunFrame( time );

	NetPacket* packet;
	while ( ( packet = m_Socket->ReceivePacket() ) != NULL )
	{
		m_System->DPrintf( "FakeClient: WARNING! Packet from %s with invalid sequence number.\n",
				packet->address.ToString() );
		m_Socket->FreePacket( packet );
	}
}

void FakeClient::SetRate( int rate )
{
	m_Server->SetRate( rate );
}

void FakeClient::Connect( NetAddress* adr )
{
	m_Server->Connect( m_World, adr, m_Socket );
}

void FakeClient::Retry()
{
	m_Server->Retry();
}

void FakeClient::Say( char* text )
{
	if ( !m_Server->IsConnected() )
		return;

	char string[1024];
	snprintf( string, sizeof( string ), "say \"%s\"", text );
	m_Server->SendStringCommand( string );
}

void FakeClient::Disconnect()
{
	m_Server->Disconnect();
}

void FakeClient::ShutDown()
{
	if ( m_State == 4 )
		return;

	if ( m_World )
		m_World->RemoveListener( this );

	BaseSystemModule::ShutDown();
	m_System->Printf( "FakeClient module shutdown.\n" );
}

void FakeClient::ReceiveSignal( ISystemModule* module, unsigned int signal, void* )
{
	if ( module->GetSerial() == m_World->GetSerial() && signal == 8 )
		ShutDown();
}

char* FakeClient::GetType()
{
	return "fakeclient000";
}

char* FakeClient::GetStatusLine()
{
	return "I am a fake client.\n";
}

bool FakeClient::Init( IBaseSystem* system, int serial, char* name )
{
	BaseSystemModule::Init( system, serial, name );
	sprintf( m_Name, "fakeclient%i", serial );

	m_Network = dynamic_cast<INetwork*>( m_System->GetModule( "network001", "core", NULL ) );
	if ( !m_Network )
	{
		m_System->Errorf( "FakeClient::Init: couldn't load network module.\n" );
		return false;
	}

	m_Socket = m_Network->CreateSocket( serial + 2048, false, false );
	if ( !m_Socket )
	{
		m_System->Errorf( "FakeClient::Init: Could not create port %i.\n", serial + 2048 );
		return false;
	}

	char temp[256];
	sprintf( temp, "fakeserver%i", serial );
	m_Server = dynamic_cast<IServer*>( m_System->GetModule( "server001", "core", temp ) );
	if ( !m_Server )
	{
		m_System->Errorf( "FakeClient::Init: couldn't load server module.\n" );
		return false;
	}

	m_Server->RegisterListener( this );
	m_Server->SetUserInfo( "hspecs", "0" );
	m_Server->SetUserInfo( "hslots", "0" );
	m_Server->SetDelayReconnect( false );
	m_Server->SetPlayerName( m_Name );
	m_Server->SetAutoRetry( false );

	sprintf( temp, "fakeworld%i", serial );
	m_World = dynamic_cast<IWorld*>( m_System->GetModule( "world001", "core", temp ) );
	if ( !m_World )
	{
		m_System->Errorf( "FakeClient::Init: couldn't load world module.\n" );
		return false;
	}

	m_World->RegisterListener( this );
	m_State = 3;
	m_System->Printf( "Fake client module initialized (%i).\n", serial );
	return true;
}
