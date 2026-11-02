#ifndef NETPACKET_H
#define NETPACKET_H

#include "NetAddress.h"
#include "Bitbuffer.h"

class NetPacket
{
public:
	NetAddress		address;
	BitBuffer		data;

	double			time;
	bool			connectionless;
	unsigned int	seqnr;
	bool			hasReliableData;
};

#endif
