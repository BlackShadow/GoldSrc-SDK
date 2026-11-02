#ifndef BASESYSTEMMODULE_H
#define BASESYSTEMMODULE_H

#include "common/ISystemModule.h"
#include "common/ObjectList.h"

#include <stddef.h>

class BaseSystemModule : public virtual ISystemModule
{
public:
	BaseSystemModule();
	virtual ~BaseSystemModule() override;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void RunFrame( double time ) override;

	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) override;
	virtual void ExecuteCommand( int commandID, char* commandLine ) override;
	virtual void RegisterListener( ISystemModule* module ) override;
	virtual void RemoveListener( ISystemModule* module ) override;

	virtual IBaseSystem* GetSystem() override;
	virtual int GetSerial() override;
	virtual char* GetStatusLine() override;
	virtual char* GetType() override;
	virtual char* GetName() override;
	virtual int GetState() override;
	virtual int GetVersion() override;

	virtual void ShutDown() override;
	virtual char* COM_GetBaseDir()
	{
		return "";
	}

protected:
	enum ModuleState
	{
		MODULE_UNDEFINED = 0,
		MODULE_INITIALIZING = 1,
		MODULE_RUNNING = 3,
		MODULE_DISCONNECTED = 4
	};

	void FireSignal( unsigned int signal, void* data );

	IBaseSystem*	m_System;
	ObjectList		m_Listener;
	char			m_Name[255];
	unsigned int	m_State;
	unsigned int	m_Serial;
	double			m_SystemTime;
};

#endif
