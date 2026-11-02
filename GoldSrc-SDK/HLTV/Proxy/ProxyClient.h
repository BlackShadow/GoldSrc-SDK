#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H

#include "HLTV/common/BaseClient.h"

class IProxy;

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4250 )
#endif

class ProxyClient : public BaseClient
{
public:
	ProxyClient( IProxy* proxy );
	virtual ~ProxyClient() override;

	virtual bool Init( IBaseSystem* system, int serial, char* moduleName ) override;
	virtual void ShutDown() override;
	virtual bool HasChatEnabled() override;

	virtual void DownloadFile( char* fileName ) override;
	virtual void ReplySpawn( int spawncount, int crcMap ) override;
	virtual bool ProcessStringCmd( char* string ) override;
	virtual void UpdateUserInfo( char* userinfostring ) override;
	virtual void SendDatagram() override;
	virtual void ParseHLTV( NetPacket* packet ) override;
	virtual void ParseVoiceData( NetPacket* packet ) override;

protected:
	IProxy*	m_Proxy;
	float	lastChatTime;
	float	lastCheerTime;
	float	nextDecalTime;
	bool	chatEnabled;
};

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif
