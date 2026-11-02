#ifndef ISERVER_H
#define ISERVER_H

class IBaseSystem;
class ISystemModule;
class IWorld;
class IProxy;
class IDirector;
class INetSocket;
class NetAddress;
class InfoString;
class BitBuffer;

class IServer
{
public:
	virtual ~IServer() = default;

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

	virtual bool Connect( IWorld* world, NetAddress* address, INetSocket* socket ) = 0;
	virtual bool LoadDemo( IWorld* world, char* filename, bool forceHLTV, bool continuous ) = 0;
	virtual void Reconnect() = 0;
	virtual void Disconnect() = 0;
	virtual void Retry() = 0;
	virtual void StopRetry() = 0;

	virtual void SendStringCommand( char* command ) = 0;
	virtual void SendHLTVCommand( BitBuffer* command ) = 0;

	virtual bool IsConnected() = 0;
	virtual bool IsDemoFile() = 0;
	virtual bool IsGameServer() = 0;
	virtual bool IsRelayProxy() = 0;
	virtual bool IsVoiceBlocking() = 0;

	virtual void SetProxy( IProxy* proxy ) = 0;
	virtual void SetDirector( IDirector* director ) = 0;
	virtual void SetPlayerName( char* name ) = 0;
	virtual void SetDelayReconnect( bool enabled ) = 0;
	virtual void SetAutoRetry( bool enabled ) = 0;
	virtual void SetVoiceBlocking( bool enabled ) = 0;
	virtual void SetRate( int rate ) = 0;
	virtual void SetUpdateRate( int rate ) = 0;
	virtual void SetUserInfo( char* key, char* value ) = 0;
	virtual bool SetProtocol( int protocol ) = 0;
	virtual void SetGameDirectory( const char* gameDirectory, const char* defaultDirectory ) = 0;

	virtual int GetRate() = 0;
	virtual int GetUpdateRate() = 0;
	virtual InfoString* GetServerInfoString() = 0;
	virtual char* GetPlayerName() = 0;
	virtual float GetTime() = 0;
	virtual IWorld* GetWorld() = 0;
	virtual char* GetDemoFileName() = 0;
	virtual NetAddress* GetAddress() = 0;
	virtual char* GetHostName() = 0;
	virtual bool GetAutoRetry() = 0;
	virtual float GetPacketLoss() = 0;
	virtual int GetProtocol() = 0;
};

#endif
