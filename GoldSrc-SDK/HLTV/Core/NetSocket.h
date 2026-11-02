#pragma once

#include "common/ObjectList.h"
#include "HLTV/common/INetSocket.h"

#include <stddef.h>
#ifdef _WIN32
#include <winsock2.h>
#endif

class INetChannel;
class INetwork;
class Network;
class IBaseSystem;
class NetAddress;
class NetPacket;
struct sockaddr;

class NetSocket : public INetSocket
{
public:
#pragma pack( push, 1 )
	struct SPLITPACKET
	{
		int				netID;
		int				sequenceNumber;
		unsigned char	packetID;
	};
#pragma pack( pop )

	struct LONGPACKET
	{
		int		currentSequence;
		int		splitCount;
		int		totalSize;
		char	buffer[4010];
	};

	NetSocket();
	virtual ~NetSocket() override;

	virtual NetPacket* ReceivePacket() override;
	virtual void FreePacket( NetPacket* packet ) override;
	virtual bool SendPacket( NetPacket* packet ) override;
	virtual bool SendPacket( NetAddress* to, const void* data, int length ) override;
	virtual void AddPacket( NetPacket* packet ) override;

	virtual bool AddChannel( INetChannel* channel ) override;
	virtual bool RemoveChannel( INetChannel* channel ) override;
	virtual INetwork* GetNetwork() override;
	virtual void OutOfBandPrintf( NetAddress* to, const char* format, ... ) override;
	virtual void Flush() override;
	virtual void GetFlowStats( float* avgInKBSec, float* avgOutKBSec ) override;
	virtual bool LeaveGroup( NetAddress* group ) override;
	virtual bool JoinGroup( NetAddress* group ) override;
	virtual void Close() override;
	virtual int GetPort() override;

	int DrainChannels();
	int DispatchIncoming();
	int GetLong( unsigned char* pData, int size );
	int ReceivePacketIntern( NetAddress* fromHost );
	void UpdateStats( double time );
	int SendLong( const char* pData, int len, int flags, const sockaddr* to, int tolen );
	int SendShort( const char* pData, int len, int flags, const sockaddr* to, int tolen );
	bool Create( Network* network, int port, bool reuse, bool loopback );

	int				m_Port = 0;
	unsigned char	m_Buffer[4010]{};
	ObjectList		m_IncomingPackets;
	ObjectList		m_Channels;
#ifdef _WIN32
	SOCKET			m_Socket = INVALID_SOCKET;
#else
	int				m_Socket = -1;
#endif
	Network*		m_Network = NULL;
	IBaseSystem*	m_System = NULL;

	LONGPACKET		netSplitPacket{};
	int				netSplitSequenceNumber = 0;
	int				netSplitFlags[5]{};

	int				m_BytesIn = 0;
	int				m_BytesOut = 0;
	double			m_LastUpdateTime = 0.0;
	float			m_AvgBytesIn = 0.0f;
	float			m_AvgBytesOut = 0.0f;
};
