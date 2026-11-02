#ifndef IENGINEWRAPPER_H
#define IENGINEWRAPPER_H

class IBaseSystem;
class ISystemModule;
struct client_data_s;
struct event_args_s;

class IEngineWrapper
{
public:
	virtual ~IEngineWrapper() = default;

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

	virtual bool GetViewOrigin( float* origin ) = 0;
	virtual bool GetViewAngles( float* angles ) = 0;
	virtual int GetTraceEntity() = 0;

	virtual float GetCvarFloat( char* name ) = 0;
	virtual char* GetCvarString( char* name ) = 0;
	virtual void SetCvar( char* name, char* value ) = 0;
	virtual void Cbuf_AddText( char* text ) = 0;

	virtual void DemoUpdateClientData( client_data_s* data ) = 0;
	virtual void CL_QueueEvent( int flags, int index, float delay, event_args_s* event ) = 0;
	virtual void HudWeaponAnim( int sequence, int body ) = 0;
	virtual void CL_DemoPlaySound( int channel, char* sample, float attenuation, float volume,
		int flags, int pitch ) = 0;
	virtual void ClientDLL_ReadDemoBuffer( int size, unsigned char* buffer ) = 0;

	virtual bool ValidStuffText( const char* text ) = 0;
	virtual void Cbuf_AddFilteredText( char* text ) = 0;
};

#endif
