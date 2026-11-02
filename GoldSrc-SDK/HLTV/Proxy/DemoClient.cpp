#include "DemoClient.h"

#include "HLTV/common/IProxy.h"
#include "HLTV/common/IWorld.h"
#include "HLTV/common/NetPacket.h"
#include "HLTV/common/common.h"
#include "common/IBaseSystem.h"
#include "common/hltv.h"

#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
DemoClient::DemoClient()
{
}

DemoClient::~DemoClient()
{
}

bool DemoClient::Init( IBaseSystem* system, int serial, char* name )
{
	BaseSystemModule::Init( system, serial, name );
	m_Proxy = NULL;
	m_IsActive = false;
	m_LastFrameSeqNr = 0;
	m_ClientDelta = 0;
	memset( m_BaseFileName, 0, sizeof( m_BaseFileName ) );
	m_DemoInfo.SetMaxSize( 256 );
	m_State = 3;
	m_System->Printf( "Demo client initialized.\n" );
	return true;
}

char* DemoClient::GetStatusLine()
{
	static char string[256];
	float demoTime = m_DemoFile.GetDemoTime();

#ifdef _WIN32
	_snprintf( string, sizeof( string ), "Recording to %s (%.1f seconds)\n", m_DemoFile.GetFileName(), demoTime );
#else
	snprintf( string, sizeof( string ), "Recording to %s (%.1f seconds)\n", m_DemoFile.GetFileName(), demoTime );
#endif
	return string;
}

char* DemoClient::GetType()
{
	return "democlient000";
}

bool DemoClient::Connect( INetSocket*, NetAddress*, char* )
{
	if ( IsActive() )
	{
		if ( m_DemoFile.IsRecording() )
			m_System->Printf( "Already recording to %s.\n", m_DemoFile.GetFileName() );
		else
			m_System->Printf( "Already recording initialized for %s.\n", m_BaseFileName );
		return false;
	}

	m_DemoChannel.Create( m_System, NULL, NULL );
	m_DemoChannel.SetUpdateRate( 40 );
	m_DemoChannel.SetRate( 20000 );
	m_LastFrameSeqNr = 0;
	m_ClientDelta = 0;
	m_DemoChannel.SetKeepAlive( false );
	m_DemoFile.Init( m_World, NULL, &m_DemoChannel );
	m_IsActive = true;
	m_System->Printf( "Recording initialized.\n" );
	return true;
}

void DemoClient::Send( unsigned char* data, int length, bool isReliable )
{
	if ( isReliable )
		m_DemoChannel.reliableStream.WriteBuf( data, length );
	else
		m_DemoChannel.unreliableStream.WriteBuf( data, length );
}

void DemoClient::Disconnect( char* reason )
{
	if ( !IsActive() )
		return;

	if ( reason != NULL )
	{
		m_DemoChannel.reliableStream.WriteByte( svc_print );
		m_DemoChannel.reliableStream.WriteString( reason );
	}

	FinishDemo();
	m_IsActive = false;
}

void DemoClient::Reconnect()
{
	FinishDemo();
}

void DemoClient::SetWorld( IWorld* world )
{
	m_World = world;
}

int DemoClient::GetClientType()
{
	return TYPE_DEMO;
}

char* DemoClient::GetClientName()
{
	return m_Name;
}

InfoString* DemoClient::GetUserInfo()
{
	return &m_DemoInfo;
}

NetAddress* DemoClient::GetAddress()
{
	static NetAddress fakeaddress;

	fakeaddress.Clear();
	return &fakeaddress;
}

bool DemoClient::IsActive()
{
	return m_IsActive;
}

bool DemoClient::IsHearingVoices()
{
	return true;
}

bool DemoClient::HasChatEnabled()
{
	return false;
}

void DemoClient::SetProxy( IProxy* proxy )
{
	m_Proxy = proxy;
}

void DemoClient::SetFileName( char* fileName )
{
	strncpy( m_BaseFileName, fileName, sizeof( m_BaseFileName ) );
	m_BaseFileName[sizeof( m_BaseFileName ) - 1] = 0;
}

DemoFile* DemoClient::GetDemoFile()
{
	return &m_DemoFile;
}

void DemoClient::FinishDemo()
{
	if ( !IsActive() )
		return;

	m_DemoChannel.reliableStream.WriteByte( svc_print );
	m_DemoChannel.reliableStream.WriteString( "HLTV Demo finished.\n" );
	m_DemoFile.WriteDemoMessage( &m_DemoChannel.unreliableStream, &m_DemoChannel.reliableStream );
	m_DemoFile.CloseFile();
	m_DemoChannel.Clear();
	m_LastFrameSeqNr = 0;
	m_ClientDelta = 0;
}

void DemoClient::ShutDown()
{
	if ( m_State == 4 )
		return;

	FinishDemo();
	m_IsActive = false;
	BaseSystemModule::ShutDown();
	m_System->Printf( "Demo module shutdown.\n" );
}

void DemoClient::WriteDatagram( double time, frame_t* frame )
{
	unsigned int deltaSeqNr = m_LastFrameSeqNr;

	if ( m_LastFrameSeqNr != 0 )
	{
		if ( m_LastFrameSeqNr == frame->seqnr )
			return;

		if ( m_LastFrameSeqNr > frame->seqnr )
			m_LastFrameSeqNr = frame->seqnr - 1;
	}
	else
	{
		char fileName[260];
		char mapname[260];

		COM_FileBase( m_World->GetLevelName(), mapname );
#ifdef _WIN32
		_snprintf( fileName, sizeof( fileName ), "%s-%s-%s.dem", m_BaseFileName, COM_TimeString(), mapname );
#else
		snprintf( fileName, sizeof( fileName ), "%s-%s-%s.dem", m_BaseFileName, COM_TimeString(), mapname );
#endif
		m_DemoFile.StartRecording( fileName );
		m_Proxy->WriteSignonData( TYPE_DEMO, &m_DemoChannel.reliableStream );
	}

	m_DemoChannel.unreliableStream.WriteByte( svc_time );
	m_DemoChannel.unreliableStream.WriteFloat( (float)time );
	m_World->WriteFrame( frame, m_LastFrameSeqNr, &m_DemoChannel.reliableStream,
			&m_DemoChannel.unreliableStream, deltaSeqNr, m_ClientDelta, true );
	m_LastFrameSeqNr = frame->seqnr;
	m_ClientDelta = (unsigned char)m_DemoChannel.outgoing_sequence;

	if ( m_DemoChannel.reliableStream.sizeError )
	{
		m_System->Printf( "WARNING! DemoClient::WriteDatagram: reliable data overflow.\n" );
		Disconnect( "reliable data overflow" );
		return;
	}

	if ( m_DemoChannel.unreliableStream.sizeError )
		m_DemoChannel.unreliableStream.Clear();

	m_DemoFile.WriteDemoMessage( &m_DemoChannel.unreliableStream, &m_DemoChannel.reliableStream );
	m_DemoChannel.TransmitOutgoing();

	client_data_t cdata;
	memset( &cdata, 0, sizeof( cdata ) );
	m_DemoFile.WriteUpdateClientData( &cdata );
}

void DemoClient::SendDatagram()
{
	if ( m_Proxy->GetDelay() <= 0 )
	{
		frame_t* frame = m_World->GetLastFrame();
		if ( frame != NULL )
			WriteDatagram( frame->time, frame );
	}
	else
	{
		double worldTime = m_Proxy->GetSpectatorTime();
		double proxyTime = m_Proxy->GetProxyTime();
		frame_t* frame = m_World->GetFrameByTime( worldTime );

		if ( frame != NULL && frame->seqnr > 1 )
			WriteDatagram( proxyTime - ( worldTime - frame->time ), frame );
	}
}

void DemoClient::RunFrame( double time )
{
	BaseSystemModule::RunFrame( time );

	NetPacket* packet;
	while ( ( packet = m_DemoChannel.GetPacket() ) != NULL )
	{
		m_System->DPrintf( "WARNING! Incoming data in demo game channel.Ignored.\n" );
		m_DemoChannel.FreePacket( packet );
	}

	if ( IsActive() && m_World->IsActive() && m_DemoChannel.IsReadyToSend() )
		SendDatagram();
}
