#ifndef IDEMOPLAYER_H
#define IDEMOPLAYER_H

class IBaseSystem;
class ISystemModule;
class IWorld;
class IProxy;
class IObjectContainer;
class BitBuffer;
class DirectorCmd;
struct ref_params_s;

class IDemoPlayer
{
public:
	virtual ~IDemoPlayer() = default;

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

	virtual void NewGame( IWorld* world, IProxy* proxy ) = 0;
	virtual char* GetModName() = 0;
	virtual void WriteCommands( BitBuffer* stream, float startTime, float endTime ) = 0;
	virtual int AddCommand( DirectorCmd* command ) = 0;
	virtual bool RemoveCommand( int index ) = 0;
	virtual DirectorCmd* GetLastCommand() = 0;
	virtual IObjectContainer* GetCommands() = 0;

	virtual void SetWorldTime( double time, bool relative ) = 0;
	virtual void SetTimeScale( float scale ) = 0;
	virtual void SetPaused( bool paused ) = 0;
	virtual void SetEditMode( bool enabled ) = 0;
	virtual void SetMasterMode( bool enabled ) = 0;

	virtual bool IsPaused() = 0;
	virtual bool IsLoading() = 0;
	virtual bool IsActive() = 0;
	virtual bool IsEditMode() = 0;
	virtual bool IsMasterMode() = 0;

	virtual void RemoveFrames( double startTime, double endTime ) = 0;
	virtual void ExecuteDirectorCmd( DirectorCmd* command ) = 0;

	virtual double GetWorldTime() = 0;
	virtual double GetStartTime() = 0;
	virtual double GetEndTime() = 0;
	virtual float GetTimeScale() = 0;
	virtual IWorld* GetWorld() = 0;
	virtual char* GetFileName() = 0;

	virtual bool SaveGame( char* filename ) = 0;
	virtual bool LoadGame( char* filename ) = 0;
	virtual void Stop() = 0;
	virtual void ForceHLTV( bool enabled ) = 0;

	virtual void GetDemoViewInfo( ref_params_s* parameters, float* view, int* viewmodel ) = 0;
	virtual int ReadDemoMessage( unsigned char* buffer, int size ) = 0;
	virtual void ReadNetchanState( int* incomingSequence, int* incomingAcknowledged, int* incomingReliableAcknowledged,
		int* incomingReliableSequence, int* outgoingSequence, int* reliableSequence, int* lastReliableSequence ) = 0;
};

#endif
