#include "HLTV/Core/Network.h"
#include "HLTV/Core/NetSocket.h"
#include "common/IBaseSystem.h"
#include "common/TokenLine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

Network::Network()
{
	m_State = 0;
}

Network::~Network()
{
}

bool Network::Init( IBaseSystem* system, int serial, char* name )
{
	char hostname[512];
	char* ip;
	char* portText;

	BaseSystemModule::Init( system, serial, name );

	if ( name != NULL )
	{
		strncpy( m_Name, name, sizeof( m_Name ) - 1 );
	}
	else
	{
		strcpy( m_Name, "network001" );
	}

	m_Name[sizeof( m_Name ) - 1] = '\0';
	m_FakeLoss = 0.0f;
	m_IsMultihomed = false;
	m_NoDNS = false;
	m_Sockets.Init();
	memset( &m_LocalAddress, 0, sizeof( m_LocalAddress ) );

	m_System->RegisterCommand( "fakeloss", this, CMD_ID_FAKELOSS );

#ifdef _WIN32
	WSADATA wsaData;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
	{
		m_System->Printf( "Network Error WSAStartup:%s\n", GetErrorText( GetLastErrorCode() ) );
		return false;
	}
#endif

	if ( m_System->CheckParam( "-nodns" ) != NULL )
	{
		m_NoDNS = true;
	}

	ip = m_System->CheckParam( "-ip" );
	if ( ResolveAddress( ip, &m_LocalAddress ) )
	{
		m_IsMultihomed = true;
		m_System->Printf( "Network uses %s as host IP.\n", m_LocalAddress.ToBaseString() );
	}
	else
	{
		gethostname( hostname, sizeof( hostname ) );
		ResolveAddress( hostname, &m_LocalAddress );
	}

	portText = m_System->CheckParam( "-port" );
	if ( portText == NULL )
	{
		portText = "27020";
	}
#ifdef _WIN32
	m_LocalAddress.m_Port = htons( (unsigned short)atoi( portText ) );
#else
	m_LocalAddress.m_Port = htons( (unsigned short)strtol( portText, NULL, 10 ) );
#endif

	m_LastStatsUpdateTime = 0.0;
	m_State = 3;
	m_System->Printf( "Network initialized.\n" );
	return true;
}

void Network::RunFrame( double time )
{
	BaseSystemModule::RunFrame( time );
	SendData();
	ReceiveData();
	UpdateStats();
}

void Network::ExecuteCommand( int commandID, char* commandLine )
{
	if ( commandID == CMD_ID_FAKELOSS )
	{
		CMD_FakeLoss( commandLine );
	}
	else
	{
		m_System->Printf( "ERROR! Network::ExecuteCommand: unknown command ID %i.\n", commandID );
	}
}

char* Network::GetStatusLine()
{
	static char string[256];
	float in;
	float out;
	char* address;
	int sockets;

	GetFlowStats( &in, &out );
	address = m_LocalAddress.ToBaseString();
	sockets = m_Sockets.CountElements();
#ifdef _WIN32
	_snprintf( string, sizeof( string ), "Local IP %s, Sockets %i, In %.2f, Out %.2f.\n", address, sockets, in, out );
#else
	snprintf( string, sizeof( string ), "Local IP %s, Sockets %i, In %.2f, Out %.2f.\n", address, sockets, in, out );
#endif
	return string;
}

char* Network::GetType()
{
	return "network001";
}

void Network::ShutDown()
{
	NetSocket* sock;

	if ( m_State == 4 )
	{
		return;
	}

	sock = (NetSocket*)m_Sockets.RemoveTail();
	while ( sock != NULL )
	{
		sock->Close();
		delete sock;
		sock = (NetSocket*)m_Sockets.RemoveTail();
	}

#ifdef _WIN32
	if ( WSACleanup() == SOCKET_ERROR )
	{
		m_System->Printf( "Network Error WSACleanup:%s\n", GetErrorText( GetLastErrorCode() ) );
	}
#endif

	m_System->Printf( "Network shut down.\n" );
	BaseSystemModule::ShutDown();
}

INetSocket* Network::CreateSocket( int port, bool reuse, bool loopback )
{
	NetSocket* socket;

	socket = GetSocket( port );
	if ( socket != NULL )
	{
		return socket;
	}

	socket = new NetSocket();
	if ( socket == NULL )
	{
		m_System->Printf( "ERROR! Network::CreateSocket: out of memory.\n" );
		return NULL;
	}

	if ( !socket->Create( this, port, reuse, loopback ) )
	{
		m_System->Printf( "ERROR! Network::CreateSocket: %s.\n", GetErrorText( GetLastErrorCode() ) );
		delete socket;
		return NULL;
	}

	m_Sockets.Add( socket );
	return socket;
}

bool Network::RemoveSocket( INetSocket* netsocket )
{
	return m_Sockets.Remove( netsocket );
}

NetAddress* Network::GetLocalAddress()
{
	return &m_LocalAddress;
}

bool Network::ResolveAddress( char* string, NetAddress* address )
{
	sockaddr sadr;
	char copy[128];
	char* cursor;
	unsigned int resolved;
	struct hostent* host;

	address->Clear();

	if ( string == NULL || string[0] == '\0' )
	{
		return false;
	}

	memset( sadr.sa_data, 0, sizeof( sadr.sa_data ) );
	sadr.sa_family = AF_INET;
	strncpy( copy, string, 127 );
	copy[127] = '\0';

	for ( cursor = copy; *cursor != '\0'; cursor++ )
	{
		if ( *cursor == ':' )
		{
			*cursor = '\0';
#ifdef _WIN32
			*(unsigned short*)sadr.sa_data = htons( (unsigned short)atoi( cursor + 1 ) );
#else
			*(unsigned short*)sadr.sa_data = htons( (unsigned short)strtol( cursor + 1, NULL, 10 ) );
#endif
			break;
		}
	}

	if ( (unsigned char)( copy[0] - '0' ) <= 9 && strchr( copy, '.' ) != NULL )
	{
		resolved = inet_addr( copy );
		if ( resolved == INADDR_NONE )
		{
			return false;
		}
	}
	else
	{
		if ( m_NoDNS )
		{
			return false;
		}

		host = gethostbyname( copy );
		if ( host == NULL )
		{
			return false;
		}

		resolved = *(unsigned int*)host->h_addr_list[0];
	}

	*(unsigned int*)&sadr.sa_data[2] = resolved;
	return address->FromSockadr( &sadr );
}

void Network::GetFlowStats( float* totalIn, float* totalOut )
{
	NetSocket* sock;
	float in;
	float out;

	*totalIn = 0.0f;
	*totalOut = 0.0f;
	sock = (NetSocket*)m_Sockets.GetFirst();
	while ( sock != NULL )
	{
		sock->GetFlowStats( &in, &out );
		*totalIn = *totalIn + in;
		*totalOut = *totalOut + out;
		sock = (NetSocket*)m_Sockets.GetNext();
	}
}

int Network::GetLastErrorCode()
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

char* Network::GetErrorText( int code )
{
#ifdef _WIN32
	switch ( code )
	{
	case WSAEINTR:
		return "WSAEINTR";
	case WSAEBADF:
		return "WSAEBADF";
	case WSAEWOULDBLOCK:
		return "WSAEWOULDBLOCK";
	case WSAEACCES:
		return "WSAEACCES";
	case WSAEFAULT:
		return "WSAEFAULT";
	case WSAEINVAL:
		return "WSAEINVAL";
	case WSAEMFILE:
		return "WSAEMFILE";
	case WSAENAMETOOLONG:
		return "WSAENAMETOOLONG";
	case WSAELOOP:
		return "WSAELOOP";
	case WSAENOTSOCK:
		return "WSAENOTSOCK";
	case WSAEDESTADDRREQ:
		return "WSAEDESTADDRREQ";
	case WSAEMSGSIZE:
		return "WSAEMSGSIZE";
	case WSAEPROTOTYPE:
		return "WSAEPROTOTYPE";
	case WSAENOPROTOOPT:
		return "WSAENOPROTOOPT";
	case WSAEPROTONOSUPPORT:
		return "WSAEPROTONOSUPPORT";
	case WSAESOCKTNOSUPPORT:
		return "WSAESOCKTNOSUPPORT";
	case WSAEOPNOTSUPP:
		return "WSAEOPNOTSUPP";
	case WSAEPFNOSUPPORT:
		return "WSAEPFNOSUPPORT";
	case WSAEAFNOSUPPORT:
		return "WSAEAFNOSUPPORT";
	case WSAEADDRINUSE:
		return "WSAEADDRINUSE";
	case WSAEADDRNOTAVAIL:
		return "WSAEADDRNOTAVAIL";
	case WSAENETDOWN:
		return "WSAENETDOWN";
	case WSAENETUNREACH:
		return "WSAENETUNREACH";
	case WSAENETRESET:
		return "WSAENETRESET";
	case WSAECONNABORTED:
		return "WSWSAECONNABORTEDAEINTR";
	case WSAECONNRESET:
		return "WSAECONNRESET";
	case WSAENOBUFS:
		return "WSAENOBUFS";
	case WSAEISCONN:
		return "WSAEISCONN";
	case WSAENOTCONN:
		return "WSAENOTCONN";
	case WSAESHUTDOWN:
		return "WSAESHUTDOWN";
	case WSAETOOMANYREFS:
		return "WSAETOOMANYREFS";
	case WSAETIMEDOUT:
		return "WSAETIMEDOUT";
	case WSAECONNREFUSED:
		return "WSAECONNREFUSED";
	case WSAEHOSTDOWN:
		return "WSAEHOSTDOWN";
	case WSAEALREADY:
		return "WSAEALREADY";
	case WSAEINPROGRESS:
		return "WSAEINPROGRESS";
	case WSASYSNOTREADY:
		return "WSASYSNOTREADY";
	case WSAVERNOTSUPPORTED:
		return "WSAVERNOTSUPPORTED";
	case WSANOTINITIALISED:
		return "WSANOTINITIALISED";
	case WSAEDISCON:
		return "WSAEDISCON";
	case WSAHOST_NOT_FOUND:
		return "WSAHOST_NOT_FOUND";
	case WSATRY_AGAIN:
		return "WSATRY_AGAIN";
	case WSANO_RECOVERY:
		return "WSANO_RECOVERY";
	case WSANO_DATA:
		return "WSANO_DATA";
	}
#else
	switch ( code )
	{
	case EINTR:
		return "WSAEINTR";
	case EBADF:
		return "WSAEBADF";
	case EWOULDBLOCK:
		return "WSAEWOULDBLOCK";
	case EACCES:
		return "WSAEACCES";
	case EFAULT:
		return "WSAEFAULT";
	case EINVAL:
		return "WSAEINVAL";
	case EMFILE:
		return "WSAEMFILE";
	case ENAMETOOLONG:
		return "WSAENAMETOOLONG";
	case ELOOP:
		return "WSAELOOP";
	case ENOTSOCK:
		return "WSAENOTSOCK";
	case EDESTADDRREQ:
		return "WSAEDESTADDRREQ";
	case EMSGSIZE:
		return "WSAEMSGSIZE";
	case EPROTOTYPE:
		return "WSAEPROTOTYPE";
	case ENOPROTOOPT:
		return "WSAENOPROTOOPT";
	case EPROTONOSUPPORT:
		return "WSAEPROTONOSUPPORT";
	case ESOCKTNOSUPPORT:
		return "WSAESOCKTNOSUPPORT";
	case EOPNOTSUPP:
		return "WSAEOPNOTSUPP";
	case EPFNOSUPPORT:
		return "WSAEPFNOSUPPORT";
	case EAFNOSUPPORT:
		return "WSAEAFNOSUPPORT";
	case EADDRINUSE:
		return "WSAEADDRINUSE";
	case EADDRNOTAVAIL:
		return "WSAEADDRNOTAVAIL";
	case ENETDOWN:
		return "WSAENETDOWN";
	case ENETUNREACH:
		return "WSAENETUNREACH";
	case ENETRESET:
		return "WSAENETRESET";
	case ECONNABORTED:
		return "WSWSAECONNABORTEDAEINTR";
	case ECONNRESET:
		return "WSAECONNRESET";
	case ENOBUFS:
		return "WSAENOBUFS";
	case EISCONN:
		return "WSAEISCONN";
	case ENOTCONN:
		return "WSAENOTCONN";
	case ESHUTDOWN:
		return "WSAESHUTDOWN";
	case ETOOMANYREFS:
		return "WSAETOOMANYREFS";
	case ETIMEDOUT:
		return "WSAETIMEDOUT";
	case ECONNREFUSED:
		return "WSAECONNREFUSED";
	case EHOSTDOWN:
		return "WSAEHOSTDOWN";
	case EALREADY:
		return "WSAEALREADY";
	case EINPROGRESS:
		return "WSAEINPROGRESS";
	}
#endif
	return "UNKNOWN ERROR";
}

int Network::ReceiveData()
{
	NetSocket* sock;
	int count;

	count = 0;
	sock = (NetSocket*)m_Sockets.GetFirst();
	while ( sock != NULL )
	{
		count += sock->DispatchIncoming();
		sock = (NetSocket*)m_Sockets.GetNext();
	}

	return count;
}

int Network::SendData()
{
	NetSocket* sock;
	int count;

	count = 0;
	sock = (NetSocket*)m_Sockets.GetFirst();
	while ( sock != NULL )
	{
		count += sock->DrainChannels();
		sock = (NetSocket*)m_Sockets.GetNext();
	}

	return count;
}

NetSocket* Network::GetSocket( int port )
{
	NetSocket* sock;

	sock = (NetSocket*)m_Sockets.GetFirst();
	while ( sock != NULL )
	{
		if ( sock->m_Port == port || port == 0 )
		{
			return sock;
		}

		sock = (NetSocket*)m_Sockets.GetNext();
	}

	return NULL;
}

void Network::UpdateStats()
{
	NetSocket* sock;

	if ( m_LastStatsUpdateTime + 0.5f <= m_SystemTime )
	{
		sock = (NetSocket*)m_Sockets.GetFirst();
		while ( sock != NULL )
		{
			sock->UpdateStats( m_SystemTime );
			sock = (NetSocket*)m_Sockets.GetNext();
		}

		m_LastStatsUpdateTime = m_SystemTime;
	}
}

void Network::CMD_FakeLoss( char* cmdLine )
{
	TokenLine params( cmdLine );
	char* value;

	if ( params.CountToken() == 2 )
	{
		value = params.GetToken( 1 );
		m_FakeLoss = (float)atof( value );
	}
	else
	{
		m_System->Printf( "Syntax: fakeloss <float>\n" );
		m_System->Printf( "Current fakeloss is %.2f\n", m_FakeLoss );
	}
}

INetwork* CreateNetwork()
{
	return new Network();
}

static InterfaceReg __g_CreateINetwork_reg( (InstantiateInterfaceFn)CreateNetwork, "network001" );
