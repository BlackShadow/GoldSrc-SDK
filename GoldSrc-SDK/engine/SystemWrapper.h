#pragma once

#include "interface.h"
#include "ObjectList.h"
#include "ISystemModule.h"
#include "IBaseSystem.h"
#include "BaseSystemModule.h"

class IFileSystem;
class IVGuiModule;
struct client_data_s;
struct event_args_s;

class IEngineWrapper
#if defined( _WIN32 )
	: public ISystemModule
#endif
{
public:
	virtual ~IEngineWrapper() = default;

#if !defined( _WIN32 )
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
#endif

	virtual bool GetViewOrigin( float* origin ) = 0;
	virtual bool GetViewAngles( float* angles ) = 0;
	virtual int GetTraceEntity() = 0;

	virtual float GetCvarFloat( char* name ) = 0;
	virtual char* GetCvarString( char* name ) = 0;
	virtual void SetCvar( char* name, char* value ) = 0;

	virtual void Cbuf_AddText( char* text ) = 0;

	virtual void DemoUpdateClientData( client_data_s* data ) = 0;
	virtual void CL_QueueEvent( int flags, int index, float delay, event_args_s* args ) = 0;
	virtual void HudWeaponAnim( int animation, int body ) = 0;
	virtual void CL_DemoPlaySound( int channel, char* sample, float attenuation, float volume, int flags,
		int pitch ) = 0;
	virtual void ClientDLL_ReadDemoBuffer( int size, unsigned char* buffer ) = 0;

	virtual bool ValidStuffText( const char* message ) = 0;
	virtual void Cbuf_AddFilteredText( char* text ) = 0;
};

class EngineWrapper : public IEngineWrapper, public BaseSystemModule
{
public:
	EngineWrapper();
	~EngineWrapper() override;

	bool Init( IBaseSystem* system, int serial, char* name ) override { return BaseSystemModule::Init( system, serial, name ); }
	void RunFrame( double time ) override { BaseSystemModule::RunFrame( time ); }
	void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) override { BaseSystemModule::ReceiveSignal( module, signal, data ); }
	void ExecuteCommand( int commandID, char* commandLine ) override { BaseSystemModule::ExecuteCommand( commandID, commandLine ); }
	void RegisterListener( ISystemModule* module ) override { BaseSystemModule::RegisterListener( module ); }
	void RemoveListener( ISystemModule* module ) override { BaseSystemModule::RemoveListener( module ); }
	IBaseSystem* GetSystem() override { return BaseSystemModule::GetSystem(); }
	int GetSerial() override { return BaseSystemModule::GetSerial(); }
	char* GetStatusLine() override;
	char* GetType() override;
	char* GetName() override { return BaseSystemModule::GetName(); }
	int GetState() override { return BaseSystemModule::GetState(); }
	int GetVersion() override { return BaseSystemModule::GetVersion(); }
	void ShutDown() override { BaseSystemModule::ShutDown(); }

	bool GetViewOrigin( float* origin ) override;
	bool GetViewAngles( float* angles ) override;
	int GetTraceEntity() override;
	float GetCvarFloat( char* name ) override;
	char* GetCvarString( char* name ) override;
	void SetCvar( char* name, char* value ) override;
	void Cbuf_AddText( char* text ) override;
	void DemoUpdateClientData( client_data_s* data ) override;
	void CL_QueueEvent( int flags, int index, float delay, event_args_s* args ) override;
	void HudWeaponAnim( int animation, int body ) override;
	void CL_DemoPlaySound( int channel, char* sample, float attenuation, float volume, int flags, int pitch ) override;
	void ClientDLL_ReadDemoBuffer( int size, unsigned char* buffer ) override;
	bool ValidStuffText( const char* message ) override;
	void Cbuf_AddFilteredText( char* text ) override;
};

class SystemWrapper : public IBaseSystem, public BaseSystemModule
{
public:
	struct command_t
	{
		char name[32];
		int commandID;
		ISystemModule* module;
	};

#if defined( _WIN32 )
	enum { MAX_LIBRARY_PATH = 260 };
#else
	enum { MAX_LIBRARY_PATH = 4096 };
#endif

	struct library_t
	{
		char name[MAX_LIBRARY_PATH];
		CSysModule* handle;
		CreateInterfaceFn createInterfaceFn;
	};

	SystemWrapper();
	~SystemWrapper() override;

	bool Init( IBaseSystem* system, int serial, char* name ) override;
	void RunFrame( double time ) override;
	void ExecuteCommand( int commandID, char* commandLine ) override;
	char* GetStatusLine() override;
	char* GetType() override;
	void ShutDown() override;

	void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) override { BaseSystemModule::ReceiveSignal( module, signal, data ); }
	void RegisterListener( ISystemModule* module ) override { BaseSystemModule::RegisterListener( module ); }
	void RemoveListener( ISystemModule* module ) override { BaseSystemModule::RemoveListener( module ); }
	IBaseSystem* GetSystem() override { return BaseSystemModule::GetSystem(); }
	int GetSerial() override { return BaseSystemModule::GetSerial(); }
	char* GetName() override { return BaseSystemModule::GetName(); }
	int GetState() override { return BaseSystemModule::GetState(); }
	int GetVersion() override { return BaseSystemModule::GetVersion(); }

	double GetTime() override;
	unsigned int GetTick() override;
	void SetFPS( float ) override {}

	void Printf( char* format, ... ) override;
	void DPrintf( char* format, ... ) override;
	void RedirectOutput( char* buffer, int maxSize ) override;

	IFileSystem* GetFileSystem() override;
	unsigned char* LoadFile( const char* name, int* length ) override;
	void FreeFile( unsigned char* buffer ) override;

	void SetTitle( char* title ) override;
	void SetStatusLine( char* status ) override;

	void ShowConsole( bool ) override {}
	void LogConsole( char* filename ) override;

	bool InitVGUI( IVGuiModule* module ) override;
#if defined( _WIN32 )
	void* GetPanel() override;
#else
	void* GetPanel();
#endif

	bool RegisterCommand( char* name, ISystemModule* module, int commandID ) override;
	void GetCommandMatches( char* string, ObjectList* matches ) override;
	void ExecuteString( char* commands ) override;
	void ExecuteFile( char* filename ) override;
	void Errorf( char* format, ... ) override;
	char* CheckParam( char* param ) override;

	bool AddModule( ISystemModule* module, char* name ) override;
	ISystemModule* GetModule( char* interfaceName, char* library, char* instanceName ) override;
	bool RemoveModule( ISystemModule* module ) override;

	void Stop() override;
	char* COM_GetBaseDir() override;

	ISystemModule* FindModule( char* type, char* name );
	library_t* GetLibrary( char* name );
	bool DispatchCommand( char* command );

private:
	enum
	{
		COMMAND_MODULES = 1,
		COMMAND_LOADMODULE = 2,
		COMMAND_UNLOADMODULE = 3
	};

	void RunExecuteString( char* commands );
	void ExecuteSingleCommand( char* text );

	void CMD_LoadModule( char* commandLine );
	void CMD_UnloadModule( char* commandLine );
	void CMD_Modules( char* commandLine );

	friend void SystemWrapper_Init();
	friend void SystemWrapperCommandForwarder();

	ObjectList m_Modules;
	ObjectList m_Libraries;
	ObjectList m_Commands;
	unsigned int m_SerialCounter;
	unsigned int m_Tick;
	double m_LastTime;
	IEngineWrapper* m_EngineWrapper;
};

extern SystemWrapper gSystemWrapper;

void SystemWrapper_Init();
void SystemWrapper_ShutDown();
void SystemWrapper_RunFrame( double time );
void SystemWrapper_ExecuteString( char* command );
int SystemWrapper_LoadModule( char* interfaceName, char* library, char* instanceName );
void SystemWrapperCommandForwarder();

int COM_BuildNumber();
void COM_RemoveEvilChars( char* string );
