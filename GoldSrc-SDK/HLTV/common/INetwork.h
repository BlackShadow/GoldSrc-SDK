#ifndef INETWORK_H
#define INETWORK_H

#include "common/ISystemModule.h"

class INetChannel;
class INetSocket;
class NetAddress;

class INetwork
{
public:
	virtual ~INetwork()
	{
	}

	virtual bool Init( IBaseSystem* system, int serial, char* name ) = 0;
	virtual void RunFrame( double time ) = 0;
	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) = 0;
	virtual void ExecuteCommand( int commandID, char* commandLine ) = 0;

	virtual void RegisterListener( ISystemModule* module ) = 0;
	virtual void RemoveListener( ISystemModule* module ) = 0;

	virtual IBaseSystem* GetSystem() = 0;
	virtual int GetSerial() = 0;
	virtual char* GetStatusLine() = 0;
	virtual char* GetType() = 0;
	virtual char* GetName() = 0;
	virtual int GetState() = 0;
	virtual int GetVersion() = 0;
	virtual void ShutDown() = 0;

	virtual INetSocket* CreateSocket( int port, bool reuse, bool loopback ) = 0;
	virtual bool RemoveSocket( INetSocket* netsocket ) = 0;

	virtual NetAddress* GetLocalAddress() = 0;
	virtual bool ResolveAddress( char* string, NetAddress* address ) = 0;
	virtual void GetFlowStats( float* totalIn, float* totalOut ) = 0;

	virtual int GetLastErrorCode() = 0;
	virtual char* GetErrorText( int code ) = 0;
};

#endif
