#ifndef INETSOCKET_H
#define INETSOCKET_H

class NetPacket;
class NetAddress;
class INetChannel;
class INetwork;

class INetSocket
{
public:
	virtual ~INetSocket()
	{
	}

	virtual NetPacket* ReceivePacket() = 0;
	virtual void FreePacket( NetPacket* packet ) = 0;
	virtual bool SendPacket( NetPacket* packet ) = 0;
	virtual bool SendPacket( NetAddress* to, const void* data, int length ) = 0;
	virtual void AddPacket( NetPacket* packet ) = 0;

	virtual bool AddChannel( INetChannel* channel ) = 0;
	virtual bool RemoveChannel( INetChannel* channel ) = 0;
	virtual INetwork* GetNetwork() = 0;

	virtual void OutOfBandPrintf( NetAddress* to, const char* format, ... ) = 0;
	virtual void Flush() = 0;
	virtual void GetFlowStats( float* avgInKBSec, float* avgOutKBSec ) = 0;

	virtual bool LeaveGroup( NetAddress* group ) = 0;
	virtual bool JoinGroup( NetAddress* group ) = 0;

	virtual void Close() = 0;
	virtual int GetPort() = 0;
};

#endif
