#pragma once

#include "common/BaseSystemModule.h"
#include "HLTV/common/INetwork.h"
#include "HLTV/common/NetAddress.h"

class NetSocket;

class Network : public BaseSystemModule, public INetwork
{
	friend class NetSocket;

public:
	Network();
	virtual ~Network() override;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void RunFrame( double time ) override;
	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) override
	{
		BaseSystemModule::ReceiveSignal( module, signal, data );
	}

	virtual void ExecuteCommand( int commandID, char* commandLine ) override;

	virtual void RegisterListener( ISystemModule* module ) override
	{
		BaseSystemModule::RegisterListener( module );
	}

	virtual void RemoveListener( ISystemModule* module ) override
	{
		BaseSystemModule::RemoveListener( module );
	}

	virtual IBaseSystem* GetSystem() override
	{
		return BaseSystemModule::GetSystem();
	}

	virtual int GetSerial() override
	{
		return BaseSystemModule::GetSerial();
	}

	virtual char* GetStatusLine() override;
	virtual char* GetType() override;

	virtual char* GetName() override
	{
		return BaseSystemModule::GetName();
	}

	virtual int GetState() override
	{
		return BaseSystemModule::GetState();
	}

	virtual int GetVersion() override
	{
		return BaseSystemModule::GetVersion();
	}

	virtual void ShutDown() override;

	virtual INetSocket* CreateSocket( int port, bool reuse, bool loopback ) override;
	virtual bool RemoveSocket( INetSocket* netsocket ) override;
	virtual NetAddress* GetLocalAddress() override;
	virtual bool ResolveAddress( char* string, NetAddress* address ) override;

	virtual void GetFlowStats( float* totalIn, float* totalOut ) override;
	virtual int GetLastErrorCode() override;
	virtual char* GetErrorText( int code ) override;

	int ReceiveData();
	int SendData();
	NetSocket* GetSocket( int port );
	void UpdateStats();
	void CMD_FakeLoss( char* cmdLine );

	enum LocalCommandIDs
	{
		CMD_ID_FAKELOSS = 1
	};

private:
	float		m_FakeLoss = 0.0f;
	NetAddress	m_LocalAddress;
	bool		m_IsMultihomed = false;
	bool		m_NoDNS = false;

	double		m_LastStatsUpdateTime = 0.0;
	ObjectList	m_Sockets;
};
