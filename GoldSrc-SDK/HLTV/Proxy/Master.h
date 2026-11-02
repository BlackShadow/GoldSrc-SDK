#ifndef MASTER_H
#define MASTER_H

#include "common/BaseSystemModule.h"

class INetSocket;
class Proxy;

class Master : public BaseSystemModule
{
	friend class Proxy;

public:
	Master();
	virtual ~Master() override;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void ExecuteCommand( int commandID, char* commandLine ) override;
	virtual void RunFrame( double time ) override;
	virtual char* GetStatusLine() override;
	virtual char* GetType() override;
	virtual void ShutDown() override;

protected:
	void InitializeSteam();

	enum LocalCommandIDs
	{
		CMD_ID_HEARTBEAT		= 1,
		CMD_ID_NOMASTER		= 2,
		CMD_ID_LISTMASTER	= 3
	};

	void CMD_Heartbeat( char* cmdLine );
	void CMD_NoMaster( char* cmdLine );
	void CMD_ListMaster( char* cmdLine );
	void SendShutdown();

	bool		m_NoMaster;
	bool		m_bMasterLoaded;
	INetSocket*	m_MasterSocket;
	Proxy*		m_Proxy;
	double		m_flMasterUpdateTime;
	bool		m_bSteamInitialized;
};

int	GetGameAppID( const char* gameDir );

#endif // MASTER_H
