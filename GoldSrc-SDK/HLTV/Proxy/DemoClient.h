#ifndef DEMOCLIENT_H
#define DEMOCLIENT_H

#include "HLTV/common/IClient.h"
#include "HLTV/common/DemoFile.h"
#include "HLTV/common/InfoString.h"
#include "HLTV/common/NetChannel.h"
#include "common/BaseSystemModule.h"

class IProxy;
class IWorld;

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4250 )
#endif

class DemoClient : public IClient, public BaseSystemModule
{
public:
	DemoClient();
	virtual ~DemoClient() override;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void RunFrame( double time ) override;
	virtual char* GetStatusLine() override;
	virtual char* GetType() override;
	virtual void ShutDown() override;

	virtual bool Connect( INetSocket* socket, NetAddress* adr, char* userinfo ) override;
	virtual void Send( unsigned char* data, int length, bool isReliable ) override;
	virtual void Disconnect( char* reason ) override;
	virtual void Reconnect() override;
	virtual void SetWorld( IWorld* world ) override;
	virtual int GetClientType() override;
	virtual char* GetClientName() override;
	virtual InfoString* GetUserInfo() override;
	virtual NetAddress* GetAddress() override;
	virtual bool IsActive() override;
	virtual bool IsHearingVoices() override;
	virtual bool HasChatEnabled() override;

	void SetProxy( IProxy* proxy );
	void SetFileName( char* fileName );
	DemoFile* GetDemoFile();

protected:
	void FinishDemo();
	void WriteDatagram( double time, frame_t* frame );
	void SendDatagram();

	IProxy*			m_Proxy;
	IWorld*			m_World;
	bool			m_IsActive;
	NetChannel		m_DemoChannel;
	DemoFile		m_DemoFile;
#ifdef _WIN32
	char			m_BaseFileName[260];
#else
	char			m_BaseFileName[4096];
#endif
	InfoString		m_DemoInfo;
	unsigned int	m_LastFrameSeqNr;
	unsigned int	m_ClientDelta;
};

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif
