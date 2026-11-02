#ifndef FAKECLIENT_H
#define FAKECLIENT_H

#include "common/BaseSystemModule.h"

class INetwork;
class IWorld;
class IServer;
class INetSocket;
class NetAddress;

class FakeClient : public BaseSystemModule
{
public:
	FakeClient();
	virtual ~FakeClient() override;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void RunFrame( double time ) override;
	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) override;
	virtual char* GetStatusLine() override;
	virtual char* GetType() override;
	virtual void ShutDown() override;

	void SetRate( int rate );
	void Connect( NetAddress* adr );
	void Retry();
	void Say( char* text );
	void Disconnect();

protected:
	INetwork*	m_Network;
	IWorld*		m_World;
	IServer*	m_Server;
	INetSocket*	m_Socket;
};

#endif
