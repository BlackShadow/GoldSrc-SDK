#pragma once

#include "const.h"
#include "mathlib.h"
#include "../common/ref_params.h"

class IBaseSystem;
class ISystemModule;
class IWorld;
class IProxy;
class BitBuffer;
class DirectorCmd;
class IObjectContainer;

class IDemoPlayer
{
public:
	virtual ~IDemoPlayer();

	virtual bool Init( IBaseSystem* system, int serial, char* name );
	virtual void RunFrame( double time );
	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data );
	virtual void ExecuteCommand( int command, char* parameters );
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

	virtual void NewGame( IWorld* world, IProxy* proxy );
	virtual char* GetModName();

	virtual void WriteCommands( BitBuffer* buffer, float startTime, float endTime );
	virtual int AddCommand( DirectorCmd* command );
	virtual bool RemoveCommand( int index );
	virtual DirectorCmd* GetLastCommand();
	virtual IObjectContainer* GetCommands();

	virtual void SetWorldTime( double time, bool relative );
	virtual void SetTimeScale( float scale );
	virtual void SetPaused( bool paused );
	virtual void SetEditMode( bool editing );
	virtual void SetMasterMode( bool master );
	virtual bool IsPaused();
	virtual bool IsLoading();
	virtual bool IsActive();
	virtual bool IsEditMode();
	virtual bool IsMasterMode();
	virtual void RemoveFrames( double startTime, double endTime );
	virtual void ExecuteDirectorCmd( DirectorCmd* command );
	virtual double GetWorldTime();
	virtual double GetStartTime();
	virtual double GetEndTime();
	virtual float GetTimeScale();
	virtual IWorld* GetWorld();
	virtual char* GetFileName();

	virtual bool SaveGame( char* filename );
	virtual bool LoadGame( char* filename );
	virtual void Stop();
	virtual void ForceHLTV( bool force );

	virtual void GetDemoViewInfo( ref_params_t* parameters, float* view, int* viewmodel );
	virtual int ReadDemoMessage( byte* buffer, int size );
	virtual void ReadNetchanState( int* incomingSequence, int* incomingAcknowledged,
		int* incomingReliableAcknowledged, int* incomingReliableSequence, int* outgoingSequence,
		int* reliableSequence, int* lastReliableSequence );
};
