#pragma once

#include "netadr.h"

struct sockaddr;

class INetAPI
{
public:
	virtual void NetAdrToSockAddr( netadr_t* address, struct sockaddr* socketAddress ) = 0;
	virtual void SockAddrToNetAdr( struct sockaddr* socketAddress, netadr_t* address ) = 0;
	virtual char* AdrToString( netadr_t* address ) = 0;
	virtual bool StringToAdr( const char* string, netadr_t* address ) = 0;
	virtual void GetSocketAddress( int socket, netadr_t* address ) = 0;
	virtual bool CompareAdr( netadr_t* address, netadr_t* other ) = 0;
	virtual void GetLocalIP( netadr_t* address ) = 0;
};

extern INetAPI* net;
