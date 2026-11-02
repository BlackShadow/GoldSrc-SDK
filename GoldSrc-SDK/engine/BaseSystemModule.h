#pragma once

#include "ISystemModule.h"
#include "ObjectList.h"

class IBaseSystem;

#if defined( _WIN32 )
#pragma pack( push, 8 )
#else
#pragma pack( push, 4 )
#endif
class BaseSystemModule : public virtual ISystemModule
{
public:
	BaseSystemModule();
	virtual ~BaseSystemModule();

	virtual bool Init( IBaseSystem* system, int serial, char* name );
	virtual void RunFrame( double time );
	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data );
	virtual void ExecuteCommand( int commandID, char* commandLine );
	virtual void RegisterListener( ISystemModule* module );
	virtual void RemoveListener( ISystemModule* module );

	virtual IBaseSystem* GetSystem();
	virtual int GetSerial();
	virtual char* GetStatusLine();
	virtual char* GetType();
	virtual char* GetName();
	virtual int GetState();
	virtual int GetVersion();

	virtual void ShutDown();
	virtual char* COM_GetBaseDir() { return ""; }

protected:
	enum : unsigned int
	{
		MODULE_UNDEFINED = 0,
		MODULE_INITIALIZING = 1,
		MODULE_RUNNING = 3,
		MODULE_DISCONNECTED = 4
	};

	void FireSignal( unsigned int signal, void* data );

	IBaseSystem* m_System;
	ObjectList m_Listener;
	char m_Name[255];
	unsigned int m_State;
	unsigned int m_Serial;
	double m_SystemTime;
};
#pragma pack( pop )
