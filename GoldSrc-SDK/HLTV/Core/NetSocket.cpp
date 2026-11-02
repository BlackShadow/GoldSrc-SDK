#include "HLTV/Core/NetSocket.h"
#include "HLTV/Core/Network.h"
#include "HLTV/common/Bitbuffer.h"
#include "HLTV/common/NetAddress.h"
#include "HLTV/common/NetPacket.h"
#include "HLTV/common/INetChannel.h"
#include "HLTV/common/random.h"
#include "common/ObjectList.h"
#include "common/port.h"
#include "common/IBaseSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define SOCKET_WOULD_BLOCK				WSAEWOULDBLOCK
#define SOCKET_CONNECTION_RESET			WSAECONNRESET
#define SOCKET_CONNECTION_REFUSED		WSAECONNREFUSED
#define SOCKET_ADDRESS_UNAVAILABLE		WSAEADDRNOTAVAIL
#define SOCKET_MESSAGE_TOO_LARGE		WSAEMSGSIZE
#define SOCKET_FAMILY_UNSUPPORTED		WSAEAFNOSUPPORT
#else
#define SOCKET_WOULD_BLOCK				EWOULDBLOCK
#define SOCKET_CONNECTION_RESET			ECONNRESET
#define SOCKET_CONNECTION_REFUSED		ECONNREFUSED
#define SOCKET_ADDRESS_UNAVAILABLE		EADDRNOTAVAIL
#define SOCKET_MESSAGE_TOO_LARGE		EMSGSIZE
#define SOCKET_FAMILY_UNSUPPORTED		EAFNOSUPPORT
#endif

#ifndef MIN_ROUTEABLE_PACKET
#define MIN_ROUTEABLE_PACKET	16
#endif

#ifndef MAX_ROUTEABLE_PACKET
#define MAX_ROUTEABLE_PACKET	1400
#endif

#ifdef _WIN32
#define SOCKET_IP_MULTICAST_TTL			3
#define SOCKET_IP_ADD_MEMBERSHIP		5
#define SOCKET_IP_DROP_MEMBERSHIP		6
#else
#define SOCKET_IP_MULTICAST_TTL			IP_MULTICAST_TTL
#define SOCKET_IP_ADD_MEMBERSHIP		IP_ADD_MEMBERSHIP
#define SOCKET_IP_DROP_MEMBERSHIP		IP_DROP_MEMBERSHIP
#endif

NetSocket::NetSocket()
{
}

NetSocket::~NetSocket()
{
}

bool NetSocket::AddChannel( INetChannel* channel )
{
	return m_Channels.AddTail( channel );
}

bool NetSocket::RemoveChannel( INetChannel* channel )
{
	return m_Channels.Remove( channel );
}

INetwork* NetSocket::GetNetwork()
{
	return m_Network;
}

int NetSocket::GetPort()
{
	return m_Port;
}

void NetSocket::GetFlowStats( float* avgInKBSec, float* avgOutKBSec )
{
	*avgInKBSec = m_AvgBytesIn * ( 1.0f / 1024.0f );
	*avgOutKBSec = m_AvgBytesOut * ( 1.0f / 1024.0f );
}

NetPacket* NetSocket::ReceivePacket()
{
	return (NetPacket*)m_IncomingPackets.RemoveTail();
}

void NetSocket::FreePacket( NetPacket* packet )
{
	if ( packet != NULL )
	{
		packet->data.Free();
		delete packet;
	}
}

void NetSocket::AddPacket( NetPacket* packet )
{
	NetPacket* newpacket;

	if ( !packet->connectionless )
	{
		m_System->DPrintf( "WARNING! NetSocket::AddPacket: only connectionless packets accepted.\n" );
		return;
	}

	newpacket = new NetPacket;
	newpacket->time = packet->time;
	newpacket->address.FromNetAddress( &packet->address );
	newpacket->connectionless = true;
	newpacket->seqnr = (unsigned int)-1;
	newpacket->data.Resize( packet->data.maxSize );
	newpacket->data.WriteBuf( packet->data.data, packet->data.maxSize );
	newpacket->data.Reset();
	m_IncomingPackets.AddHead( newpacket );
}

int NetSocket::DrainChannels()
{
	int count;
	INetChannel* channel;

	channel = (INetChannel*)m_Channels.GetFirst();

	if ( channel == NULL )
	{
		return 0;
	}

	count = 0;

	do
	{
		while ( !channel->IsReadyToSend() || !channel->KeepAlive() || !channel->IsConnected() )
		{
			channel = (INetChannel*)m_Channels.GetNext();

			if ( channel == NULL )
			{
				return count;
			}
		}

		count++;
		channel->TransmitOutgoing();
		channel = (INetChannel*)m_Channels.GetNext();
	}
	while ( channel != NULL );

	return count;
}

int NetSocket::DispatchIncoming()
{
	int count;
	int length;
	NetAddress from;
	INetChannel* channel;
	NetPacket* packet;

	count = 0;

	while ( ( length = ReceivePacketIntern( &from ) ) != 0 )
	{
		if ( length == -1 )
		{
			continue;
		}

		for ( channel = (INetChannel*)m_Channels.GetFirst(); channel != NULL;
				channel = (INetChannel*)m_Channels.GetNext() )
		{
			if ( from.Equal( channel->GetTargetAddress() ) )
			{
				channel->ProcessIncoming( m_Buffer, length );
				break;
			}
		}

		if ( channel == NULL && *(int*)m_Buffer == -1 )
		{
			length -= 4;
			packet = new NetPacket;
			packet->time = m_System->GetTime();
			packet->address.FromNetAddress( &from );
			packet->connectionless = true;
			packet->seqnr = (unsigned int)-1;
			packet->data.Resize( length );
			packet->data.WriteBuf( &m_Buffer[4], length );
			packet->data.Reset();
			m_IncomingPackets.AddHead( packet );
		}

		count++;
	}

	return count;
}

int NetSocket::GetLong( unsigned char* pData, int size )
{
	static int packetSequenceFlags[5] = { -1, -1, -1, -1, -1 };
	SPLITPACKET* pHeader;
	int packetNumber;
	int packetCount;
	int sequenceNumber;
	int offset;
	int payload;
	int splitSize;

	pHeader = (SPLITPACKET*)pData;
	packetNumber = pHeader->packetID >> 4;
	packetCount = pHeader->packetID & 0x0F;
	sequenceNumber = pHeader->sequenceNumber;

	if ( packetNumber > 4 || packetCount > 5 )
	{
		m_System->Printf( "Malformed packet number (%i)\n", packetNumber );
		return 0;
	}

	if ( netSplitPacket.currentSequence == -1 || sequenceNumber != netSplitPacket.currentSequence )
	{
		netSplitPacket.currentSequence = sequenceNumber;
		netSplitPacket.splitCount = packetCount;
	}

	splitSize = MAX_ROUTEABLE_PACKET - (int)sizeof( SPLITPACKET );
	payload = size - (int)sizeof( SPLITPACKET );
	offset = splitSize * packetNumber;

	if ( sequenceNumber == packetSequenceFlags[packetNumber] )
	{
		m_System->DPrintf( "NetSocket::GetLong: Ignoring duplicated split packet %i of %i ( %i bytes )\n",
				packetNumber + 1, packetCount, payload );
	}
	else
	{
		if ( packetNumber == packetCount - 1 )
		{
			netSplitPacket.totalSize = payload + offset;
		}

		netSplitPacket.splitCount--;
		packetSequenceFlags[packetNumber] = sequenceNumber;
	}

	memcpy( netSplitPacket.buffer + offset, pData + sizeof( SPLITPACKET ), payload );

	if ( netSplitPacket.splitCount > 0 )
	{
		return 0;
	}

	netSplitPacket.currentSequence = -1;

	if ( (unsigned int)netSplitPacket.totalSize > sizeof( m_Buffer ) )
	{
		m_System->DPrintf( "WARNING! NetSocket::GetLong: Split packet too large! %d bytes\n",
				netSplitPacket.totalSize );
		return -1;
	}

	memcpy( pData, netSplitPacket.buffer, netSplitPacket.totalSize );
	return netSplitPacket.totalSize;
}

int NetSocket::ReceivePacketIntern( NetAddress* fromHost )
{
	sockaddr from;
	int fromlen;
	int ret;
	int err;

	fromlen = (int)sizeof( from );
#ifdef _WIN32
	ret = recvfrom( m_Socket, (char*)m_Buffer, sizeof( m_Buffer ), 0, &from, &fromlen );
#else
	ret = recvfrom( m_Socket, m_Buffer, sizeof( m_Buffer ), 0, &from, (socklen_t*)&fromlen );
#endif

	if ( ret == -1 )
	{
		err = m_Network->GetLastErrorCode();

		if ( err != SOCKET_CONNECTION_RESET && err != SOCKET_WOULD_BLOCK && err != SOCKET_CONNECTION_REFUSED )
		{
			if ( err == SOCKET_MESSAGE_TOO_LARGE )
			{
				m_System->DPrintf( "WARNING! NetSocket::ReceivePacket: Ignoring oversized network message\n" );
			}
			else
			{
				m_System->DPrintf( "WARNING! NetSocket::ReceivePacket: %s %d %i\n",
						m_Network->GetErrorText( err ), err, errno );
				perror( "err" );
			}
		}

		return 0;
	}

	if ( ret == 0 || m_Network->m_FakeLoss > RandomFloat( 0.0f, 1.0f ) )
	{
		return 0;
	}

	if ( ret <= 3 )
	{
		m_System->DPrintf( "WARNING! NetSocket::ReceivePacket: Ignoring undersized network message\n" );
		return 0;
	}

	if ( ret == (int)sizeof( m_Buffer ) )
	{
		m_System->DPrintf( "WARNING! NetSocket::ReceivePacket: Oversize packet from %s\n", fromHost->ToString() );
		return 0;
	}

	if ( *(int*)m_Buffer == -2 )
	{
		if ( ret <= 8 )
		{
			m_System->Printf( "Invalid split packet length %i\n", ret );
			return 0;
		}

		ret = GetLong( m_Buffer, ret );
	}

	fromHost->FromSockadr( &from );
	m_BytesIn += ret + 28;
	return ret;
}

void NetSocket::UpdateStats( double time )
{
	float timeDiff;
	int bytesIn;
	int bytesOut;

	timeDiff = (float)( time - m_LastUpdateTime );

	if ( timeDiff <= 0.0f )
	{
		return;
	}

	bytesIn = m_BytesIn;
	m_BytesIn = 0;
	m_AvgBytesIn = ( (float)bytesIn / timeDiff ) * 0.3333333f + m_AvgBytesIn * 0.6666667f;

	bytesOut = m_BytesOut;
	m_BytesOut = 0;
	m_AvgBytesOut = ( (float)bytesOut / timeDiff ) * 0.3333333f + m_AvgBytesOut * 0.6666667f;

	m_LastUpdateTime = time;
}

int NetSocket::SendLong( const char* pData, int len, int flags, const sockaddr* to, int tolen )
{
	unsigned char packet[MAX_ROUTEABLE_PACKET];
	SPLITPACKET* header;
	const char* pPacket;
	int sequence;
	int splitSize;
	int packetCount;
	int packetNumber;
	int totalSent;
	int size;
	int ret;

	sequence = (int)( (unsigned int)netSplitSequenceNumber + 1u );

	if ( sequence < 0 )
	{
		sequence = 1;
	}

	netSplitSequenceNumber = sequence;

	header = (SPLITPACKET*)packet;
	header->netID = -2;
	header->sequenceNumber = sequence;

	if ( len <= 0 )
	{
		return 0;
	}

	splitSize = MAX_ROUTEABLE_PACKET - (int)sizeof( SPLITPACKET );
	packetCount = ( len + splitSize - 1 ) / splitSize;
	totalSent = 0;
	packetNumber = 0;
	pPacket = pData;

	while ( len > 0 )
	{
		size = std::min( len, splitSize );
		header->packetID = (unsigned char)( ( packetNumber << 4 ) + packetCount );
		memcpy( packet + sizeof( SPLITPACKET ), pPacket, size );

		ret = sendto( m_Socket, (const char*)packet, size + (int)sizeof( SPLITPACKET ), flags, to, tolen );

		if ( ret < 0 )
		{
			return ret;
		}

		if ( size <= ret )
		{
			totalSent += size;
		}

		len -= size;
		pPacket += splitSize;
		packetNumber++;
	}

	return totalSent;
}

int NetSocket::SendShort( const char* pData, int len, int flags, const sockaddr* to, int tolen )
{
	char packet[MIN_ROUTEABLE_PACKET];

	if ( len > MIN_ROUTEABLE_PACKET )
	{
		m_System->DPrintf( "WARNING! NetSocket::SendShort: length > MIN_ROUTEABLE_PACKET.\n" );
		return 0;
	}

	memcpy( packet, pData, len );
	memset( packet + len, 0, MIN_ROUTEABLE_PACKET - len );
	return sendto( m_Socket, packet, MIN_ROUTEABLE_PACKET, flags, to, tolen );
}

bool NetSocket::SendPacket( NetAddress* to, const void* data, int length )
{
	sockaddr addr;
	int ret;
	int err;

	if ( m_Network->m_FakeLoss != 0.0f && m_Network->m_FakeLoss > RandomFloat( 0.0f, 1.0f ) )
	{
		return true;
	}

	if ( length <= 0 || to == NULL || data == NULL )
	{
		return false;
	}

	to->ToSockadr( &addr );

	if ( length > MAX_ROUTEABLE_PACKET )
	{
		ret = SendLong( (const char*)data, length, 0, &addr, (int)sizeof( addr ) );
	}
	else if ( length >= MIN_ROUTEABLE_PACKET )
	{
		ret = sendto( m_Socket, (const char*)data, length, 0, &addr, (int)sizeof( addr ) );
	}
	else
	{
		ret = SendShort( (const char*)data, length, 0, &addr, (int)sizeof( addr ) );
	}

	if ( ret == -1 )
	{
		err = m_Network->GetLastErrorCode();

		if ( err == SOCKET_WOULD_BLOCK || err == SOCKET_ADDRESS_UNAVAILABLE
				|| err == SOCKET_CONNECTION_RESET || err == SOCKET_CONNECTION_REFUSED )
		{
			return true;
		}

		m_System->DPrintf( "WARNING! NetSocket::SendPacket: %s\n", m_Network->GetErrorText( err ) );
		return false;
	}

	m_BytesOut += ret + 28;
	return true;
}

bool NetSocket::SendPacket( NetPacket* packet )
{
	int length;
	unsigned char* data;

	length = packet->data.CurrentSize();
	data = packet->data.data;
	return SendPacket( &packet->address, data, length );
}

void NetSocket::OutOfBandPrintf( NetAddress* to, const char* format, ... )
{
	char string[4037];
	va_list argptr;

	*(int*)string = -1;

	va_start( argptr, format );
	vsnprintf( string + 4, sizeof( string ) - 4, format, argptr );
	va_end( argptr );

	SendPacket( to, string, (int)strlen( string ) + 1 );
}

void NetSocket::Flush()
{
	NetPacket* in;
	sockaddr from;
	int fromlen;

	while ( ( in = (NetPacket*)m_IncomingPackets.RemoveTail() ) != NULL )
	{
		in->data.Free();
		delete in;
	}

	if ( m_Socket != 0 )
	{
		fromlen = (int)sizeof( from );

#ifdef _WIN32
		while ( recvfrom( m_Socket, (char*)m_Buffer, sizeof( m_Buffer ), 0, &from, &fromlen ) > 0 )
#else
		while ( recvfrom( m_Socket, m_Buffer, sizeof( m_Buffer ), 0, &from, (socklen_t*)&fromlen ) > 0 )
#endif
		{
		}
	}
}

void NetSocket::Close()
{
	INetChannel* channel;

	while ( ( channel = (INetChannel*)m_Channels.RemoveHead() ) != NULL )
	{
		channel->Close();
	}

	Flush();
	m_Network->RemoveSocket( this );
#ifdef _WIN32
	shutdown( m_Socket, SD_BOTH );
	closesocket( m_Socket );
#else
	shutdown( m_Socket, SHUT_RDWR );
	close( m_Socket );
#endif
}

bool NetSocket::JoinGroup( NetAddress* group )
{
	ip_mreq mreq;
	int err;

	mreq.imr_multiaddr.s_addr = *(unsigned int*)group->m_IP;
	mreq.imr_interface.s_addr = INADDR_ANY;

	err = setsockopt( m_Socket, IPPROTO_IP, SOCKET_IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof( mreq ) );

	if ( err == -1 )
	{
		err = m_Network->GetLastErrorCode();

		if ( err != SOCKET_FAMILY_UNSUPPORTED )
		{
			m_System->DPrintf( "WARNING! NetSocket::JoinGroup: IP_ADD_MEMBERSHIP: %s\n",
					m_Network->GetErrorText( err ) );
		}

		return false;
	}

	return true;
}

bool NetSocket::LeaveGroup( NetAddress* group )
{
	ip_mreq mreq;
	int err;

	mreq.imr_multiaddr.s_addr = *(unsigned int*)group->m_IP;
	mreq.imr_interface.s_addr = INADDR_ANY;

	err = setsockopt( m_Socket, IPPROTO_IP, SOCKET_IP_DROP_MEMBERSHIP, (char*)&mreq, sizeof( mreq ) );

	if ( err == -1 )
	{
		err = m_Network->GetLastErrorCode();

		if ( err != SOCKET_FAMILY_UNSUPPORTED )
		{
			m_System->DPrintf( "WARNING! NetSocket::LeaveGroup: IP_DROP_MEMBERSHIP: %s\n",
					m_Network->GetErrorText( err ) );
		}

		return false;
	}

	return true;
}

bool NetSocket::Create( Network* network, int port, bool reuse, bool loopback )
{
#ifdef _WIN32
	u_long i;
#else
	unsigned int i;
#endif
	char reuseAddress;
	const char* ttlparam;
	sockaddr_in address;

	(void)loopback;

	m_Network = network;
	m_System = network->m_System;
	i = 1;
	reuseAddress = 1;

	m_Channels.Init();
	memset( m_Buffer, 0, sizeof( m_Buffer ) );
	memset( &netSplitPacket, 0, sizeof( netSplitPacket ) );

	m_Socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

#ifdef _WIN32
	if ( m_Socket == INVALID_SOCKET
			|| ioctlsocket( m_Socket, FIONBIO, &i ) == SOCKET_ERROR
#else
	if ( m_Socket == -1
			|| ioctl( m_Socket, FIONBIO, &i ) == -1
#endif
			|| setsockopt( m_Socket, SOL_SOCKET, SO_BROADCAST, (const char*)&i, sizeof( i ) ) == -1
			|| ( reuse && setsockopt( m_Socket, SOL_SOCKET, SO_REUSEADDR,
					&reuseAddress, sizeof( reuseAddress ) ) == -1 ) )
	{
		return false;
	}

	if ( m_Network->m_IsMultihomed )
	{
		m_Network->GetLocalAddress()->ToSockadr( (sockaddr*)&address );
	}
	else
	{
		address.sin_addr.s_addr = INADDR_ANY;
	}

	address.sin_family = AF_INET;
	m_Port = port;
	address.sin_port = htons( (unsigned short)port );

	if ( bind( m_Socket, (const sockaddr*)&address, sizeof( address ) ) == -1 )
	{
#ifdef _WIN32
		closesocket( m_Socket );
#else
		close( m_Socket );
#endif
		return false;
	}

	ttlparam = m_System->CheckParam( "-multicastttl" );

	if ( ttlparam != NULL )
	{
		i = (unsigned int)atoi( ttlparam );
	}
	else
	{
		i = 32;
	}

	if ( setsockopt( m_Socket, IPPROTO_IP, SOCKET_IP_MULTICAST_TTL, (const char*)&i, sizeof( i ) ) == -1 )
	{
		return false;
	}

	m_BytesOut = 0;
	m_BytesIn = 0;
	m_LastUpdateTime = 0.0;
	m_AvgBytesOut = 0.0f;
	m_AvgBytesIn = 0.0f;
	return true;
}
