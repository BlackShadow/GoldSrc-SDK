#ifndef DEMO_PLAYER_H
#define DEMO_PLAYER_H

#include "IDemoPlayer.h"
#include "BaseSystemModule.h"
#include "IDirector.h"
#include "ObjectDictionary.h"
#include "Bitbuffer.h"

class DirectorCmd;
class IEngineWrapper;
class IProxy;
class IServer;
class IWorld;

struct ref_params_s;
typedef struct ref_params_s ref_params_t;

class DemoPlayer : public IDemoPlayer, public BaseSystemModule, public IDirector
{
public:
	DemoPlayer();
	virtual ~DemoPlayer() override;

	virtual void RegisterListener( ISystemModule* module ) override
	{
		BaseSystemModule::RegisterListener( module );
	}

	virtual void RemoveListener( ISystemModule* module ) override
	{
		BaseSystemModule::RemoveListener( module );
	}

	virtual IBaseSystem* GetSystem() override
	{
		return BaseSystemModule::GetSystem();
	}

	virtual int GetSerial() override
	{
		return BaseSystemModule::GetSerial();
	}

	virtual char* GetName() override
	{
		return BaseSystemModule::GetName();
	}

	virtual int GetState() override
	{
		return BaseSystemModule::GetState();
	}

	virtual int GetVersion() override
	{
		return BaseSystemModule::GetVersion();
	}

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void RunFrame( double time ) override;
	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) override;
	virtual void ExecuteCommand( int commandID, char* commandLine ) override;
	virtual char* GetStatusLine() override;
	virtual char* GetType() override;
	virtual void ShutDown() override;

	virtual void SetWorldTime( double time, bool relative ) override;
	virtual void SetTimeScale( float scale ) override;
	virtual void SetPaused( bool state ) override;
	virtual void SetEditMode( bool state ) override;
	virtual void SetMasterMode( bool state ) override;

	virtual bool IsPaused() override;
	virtual bool IsLoading() override;
	virtual bool IsActive() override;
	virtual bool IsEditMode() override;
	virtual bool IsMasterMode() override;

	virtual void RemoveFrames( double startTime, double endTime ) override;
	virtual void ExecuteDirectorCmd( DirectorCmd* cmd ) override;

	virtual double GetWorldTime() override;
	virtual double GetStartTime() override;
	virtual double GetEndTime() override;
	virtual float GetTimeScale() override;
	virtual IWorld* GetWorld() override;
	virtual char* GetFileName() override;
	virtual bool SaveGame( char* filename ) override;
	virtual bool LoadGame( char* filename ) override;
	virtual void Stop() override;
	virtual void ForceHLTV( bool state ) override;

	virtual void GetDemoViewInfo( ref_params_t* rp, float* view, int* viewmodel ) override;
	virtual int ReadDemoMessage( unsigned char* buffer, int size ) override;
	virtual void ReadNetchanState( int* incoming_sequence, int* incoming_acknowledged,
			int* incoming_reliable_acknowledged, int* incoming_reliable_sequence, int* outgoing_sequence,
			int* reliable_sequence, int* last_reliable_sequence ) override;

	virtual void NewGame( IWorld* world, IProxy* proxy ) override;
	virtual int AddCommand( DirectorCmd* cmd ) override;
	virtual bool RemoveCommand( int index ) override;
	virtual IObjectContainer* GetCommands() override;
	virtual void WriteCommands( BitBuffer* stream, float startTime, float endTime ) override;
	virtual char* GetModName() override;
	virtual DirectorCmd* GetLastCommand() override;

	IDirector* GetDirector();

protected:
	enum PlayerState
	{
		DEMO_INACTIVE	= 0,
		DEMO_LOADING	= 1,
		DEMO_NEWGAME	= 2,
		DEMO_SPAWNING	= 3,
		DEMO_PLAYING	= 4
	};

	enum LocalCommandIDs
	{
		CMD_ID_JUMP			= 1,
		CMD_ID_FORCEHLTV	= 2,
		CMD_ID_PAUSE		= 3,
		CMD_ID_SPEED		= 4,
		CMD_ID_START		= 5,
		CMD_ID_SAVE			= 6,
	};

	double GetPlayerTime();

	void CMD_Jump( char* cmdLine );
	void CMD_ForceHLTV( char* cmdLine );
	void CMD_Pause( char* cmdLine );
	void CMD_Speed( char* cmdLine );
	void CMD_Start( char* cmdLine );
	void CMD_Save( char* cmdLine );

	void WriteSpawn( BitBuffer* stream );
	void WriteCameraPath( DirectorCmd* cmd, BitBuffer* stream );
	void ReindexCommands();
	void RunClocks();
	char* FormatTime( float time );
	void WriteDatagram( BitBuffer* stream );
	void ExecuteDemoFileCommands( BitBuffer* stream );

	IEngineWrapper*		m_Engine;
	IWorld*				m_World;
	IServer*			m_Server;

	ObjectDictionary	m_Commands;
	DirectorCmd*		m_LastCmd;
	unsigned int		m_CommandCounter;

#ifdef _WIN32
	static const int DemoFileNameCapacity = 260;
#else
	static const int DemoFileNameCapacity = 4096;
#endif
	char				m_DemoFileName[DemoFileNameCapacity];
	int					m_PlayerState;
	BitBuffer			m_DemoStream;

	bool				m_EditorMode;
	bool				m_MasterMode;
	bool				m_ForceHLTV;
	bool				m_IsSaving;

	float				m_TimeScale;
	double				m_WorldTime;
	double				m_PlayerTime;
	double				m_StartTime;
	double				m_LastFrameTime;
	bool				m_IsPaused;
	double				m_LastClockUpdateTime;

	unsigned int		m_LastFrameSeqNr;
	unsigned int		m_DeltaFrameSeqNr;
	unsigned int		m_Outgoing_sequence;
};

#endif
