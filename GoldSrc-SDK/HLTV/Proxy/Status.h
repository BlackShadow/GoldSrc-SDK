#ifndef STATUS_H
#define STATUS_H

#include "common/BaseSystemModule.h"
#include "common/ObjectDictionary.h"
#include "HLTV/common/NetAddress.h"

class BitBuffer;
class IProxy;

class Status : public BaseSystemModule
{
public:
	Status();
	virtual ~Status() override;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void ExecuteCommand( int commandID, char* commandLine ) override;
	virtual void RunFrame( double time ) override;
	virtual char* GetStatusLine() override;
	virtual char* GetType() override;
	virtual void ShutDown() override;

	void GetGlobalStats( int& proxies, int& slots, int& spectators );
	void GetLocalStats( int& proxies, int& slots, int& spectators );
	void SetProxy( IProxy* proxy );
	float GetBestRelayProxy( NetAddress* addr );
	void ParseStatusMsg( BitBuffer* stream );
	void ParseStatusReport( NetAddress* from, BitBuffer* stream );
	void Reset();
	int GetMaxSpectatorNumber();

protected:
	enum LocalCommandIDs
	{
		CMD_ID_PROXIES = 1
	};

	void CMD_Proxies( char* cmdLine );
	void RemoveProxy( NetAddress* addr );
	void ClearProxyList();
	void ReplyStatusReport();
	void SendStatusUpdate();

	typedef struct proxyInfo_s
	{
		NetAddress	address;
		int			slots;
		int			proxies;
		int			spectators;
		int			isPrivate;
		float		time;
	} proxyInfo_t;

	IProxy*				m_Proxy;
	float				m_NextUpdateTime;
	ObjectDictionary	m_Proxies;
	int					m_NumberOfProxies;
	int					m_NumberOfSlots;
	int					m_NumberOfSpectators;
	int					m_MaxNumberOfSpectators;
};

#endif
