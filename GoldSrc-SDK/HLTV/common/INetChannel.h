#ifndef INETCHANNEL_H
#define INETCHANNEL_H

class IBaseSystem;
class INetSocket;
class NetAddress;

class INetChannel
{
public:
	virtual ~INetChannel()
	{
	}

	virtual bool Create( IBaseSystem* system, INetSocket* netsocket, NetAddress* adr ) = 0;
	virtual NetAddress* GetTargetAddress() = 0;
	virtual void Close() = 0;
	virtual void Clear() = 0;
	virtual void Reset() = 0;

	virtual bool IsConnected() = 0;
	virtual bool IsReadyToSend() = 0;
	virtual bool IsCrashed() = 0;
	virtual bool IsTimedOut() = 0;
	virtual bool IsFakeChannel() = 0;
	virtual bool KeepAlive() = 0;

	virtual void SetRate( int newRate ) = 0;
	virtual void SetUpdateRate( int newupdaterate ) = 0;
	virtual void SetTimeOut( float time ) = 0;
	virtual void SetKeepAlive( bool flag ) = 0;

	virtual float GetIdleTime() = 0;
	virtual int GetRate() = 0;
	virtual int GetUpdateRate() = 0;
	virtual float GetLoss() = 0;

	virtual void TransmitOutgoing() = 0;
	virtual void ProcessIncoming( unsigned char* data, int size ) = 0;
	virtual void OutOfBandPrintf( const char* format, ... ) = 0;
	virtual void FakeAcknowledgement() = 0;
};

#endif
