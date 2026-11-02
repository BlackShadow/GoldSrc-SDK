#include "HLTV/common/NetAddress.h"
#include "HLTV/common/Bitbuffer.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

unsigned short _ntohs( unsigned short netshort )
{
	return (unsigned short)( ( netshort << 8 ) | ( netshort >> 8 ) );
}

unsigned short _htons( unsigned short hostshort )
{
	return (unsigned short)( ( hostshort << 8 ) | ( hostshort >> 8 ) );
}

NetAddress::NetAddress()
{
#ifdef _WIN32
	memset( this, 0, sizeof( *this ) );
#else
	Clear();
#endif
}

NetAddress::~NetAddress()
{
}

void NetAddress::SetPort_( short port )
{
	m_Port = _htons( (unsigned short)port );
}

void NetAddress::ToSockadr( sockaddr* s )
{
	sockaddr_in* addr = (sockaddr_in*)s;

	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = *(unsigned int*)m_IP;
	addr->sin_port = m_Port;
}

bool NetAddress::FromSockadr( sockaddr* s )
{
	sockaddr_in* addr = (sockaddr_in*)s;

	if ( addr->sin_family != AF_INET )
		return false;

	*(unsigned int*)m_IP = addr->sin_addr.s_addr;
	m_Port = addr->sin_port;
	return true;
}

bool NetAddress::Equal( NetAddress* a )
{
	if ( memcmp( a->m_IP, m_IP, sizeof( m_IP ) ) != 0 )
		return false;

	return m_Port == a->m_Port;
}

bool NetAddress::EqualBase( NetAddress* a )
{
	return memcmp( a->m_IP, m_IP, sizeof( m_IP ) ) == 0;
}

void NetAddress::ToStream( BitBuffer* stream )
{
	stream->WriteByte( m_IP[0] );
	stream->WriteByte( m_IP[1] );
	stream->WriteByte( m_IP[2] );
	stream->WriteByte( m_IP[3] );
	stream->WriteShort( m_Port );
}

void NetAddress::FromStream( BitBuffer* stream )
{
	m_IP[0] = (unsigned char)stream->ReadByte();
	m_IP[1] = (unsigned char)stream->ReadByte();
	m_IP[2] = (unsigned char)stream->ReadByte();
	m_IP[3] = (unsigned char)stream->ReadByte();
	m_Port = (unsigned short)stream->ReadShort();
}

char* NetAddress::ToString()
{
#ifdef _WIN32
	_snprintf( m_String, sizeof( m_String ), "%i.%i.%i.%i:%i", m_IP[0], m_IP[1], m_IP[2], m_IP[3], _ntohs( m_Port ) );
#else
	snprintf( m_String, sizeof( m_String ), "%i.%i.%i.%i:%i", m_IP[0], m_IP[1], m_IP[2], m_IP[3], _ntohs( m_Port ) );
#endif
	return m_String;
}

char* NetAddress::ToBaseString()
{
#ifdef _WIN32
	_snprintf( m_String, sizeof( m_String ), "%i.%i.%i.%i", m_IP[0], m_IP[1], m_IP[2], m_IP[3] );
#else
	snprintf( m_String, sizeof( m_String ), "%i.%i.%i.%i", m_IP[0], m_IP[1], m_IP[2], m_IP[3] );
#endif
	return m_String;
}

void NetAddress::FromNetAddress( NetAddress* adr )
{
	if ( !adr )
	{
		memset( m_IP, 0, sizeof( m_IP ) );
		m_Port = 0;
		return;
	}

	memcpy( m_IP, adr->m_IP, sizeof( m_IP ) );
	m_Port = adr->m_Port;
}

netadr_t* NetAddress::ToOldNetAdr()
{
	static netadr_t adr;

	adr.type = NA_IP;
	adr.port = m_Port;
	*(unsigned int*)adr.ip = *(unsigned int*)m_IP;
	return &adr;
}

bool NetAddress::FromOldNetAdr( netadr_t* adr )
{
	if ( adr->type != NA_IP )
		return false;

	memcpy( m_IP, adr->ip, sizeof( m_IP ) );
	m_Port = adr->port;
	return true;
}

bool NetAddress::IsValid()
{
	if ( !m_Port )
		return false;

	return *(unsigned int*)m_IP != 0;
}

void NetAddress::Clear()
{
	memset( m_IP, 0, sizeof( m_IP ) );
	m_Port = 0;
	memset( m_String, 0, sizeof( m_String ) );
}

bool NetAddress::IsSubAdress( NetAddress* adr )
{
	for ( int i = 0; i < 4; i++ )
	{
		if ( m_IP[i] && m_IP[i] != adr->m_IP[i] )
			return false;
	}

	return *(unsigned int*)m_IP != 0;
}
