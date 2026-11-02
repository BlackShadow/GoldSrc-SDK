#ifndef NETADDRESS_H
#define NETADDRESS_H

#include "common/netadr.h"

class BitBuffer;
struct sockaddr;

class NetAddress
{
public:
	NetAddress();
	virtual ~NetAddress();

	void SetPort_( short port );
	void ToSockadr( sockaddr* s );
	bool FromSockadr( sockaddr* s );
	bool Equal( NetAddress* a );
	bool EqualBase( NetAddress* a );

	void ToStream( BitBuffer* stream );
	void FromStream( BitBuffer* stream );
	char* ToString();
	char* ToBaseString();
	void FromNetAddress( NetAddress* adr );
	netadr_t* ToOldNetAdr();
	bool FromOldNetAdr( netadr_t* adr );

	bool IsValid();
	void Clear();
	bool IsSubAdress( NetAddress* adr );

	unsigned char	m_IP[4];
	unsigned short	m_Port;
	char			m_String[32];
};

#endif // NETADDRESS_H
