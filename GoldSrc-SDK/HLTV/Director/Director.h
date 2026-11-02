#ifndef DIRECTOR_H
#define DIRECTOR_H

#include "HLTV/common/IDirector.h"
#include "common/BaseSystemModule.h"
#include "common/ObjectDictionary.h"
#include "HLTV/common/types.h"

#define MAX_DIRECTOR_HISTORY	200
#define MAX_DIRECTOR_PLAYERS	32

class IBSPModel;
struct frame_s;

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4250 )
#endif

class Director : public IDirector, public BaseSystemModule
{
public:
	Director();
	virtual ~Director() override;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void RunFrame( double time ) override;
	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) override;
	virtual void ExecuteCommand( int commandID, char* commandLine ) override;
	virtual char* GetStatusLine() override;
	virtual char* GetType() override;
	virtual void ShutDown() override;

	virtual void NewGame( IWorld* world, IProxy* proxy ) override;
	virtual int AddCommand( DirectorCmd* command ) override;
	virtual void WriteCommands( BitBuffer* stream, float startTime, float endTime ) override;
	virtual char* GetModName() override;
	virtual bool RemoveCommand( int index ) override;
	virtual DirectorCmd* GetLastCommand() override;
	virtual IObjectContainer* GetCommands() override;

protected:
	enum LocalCommandIDs
	{
		CMD_ID_SLOWMOTION = 1
	};

	typedef struct playerData_s
	{
		vec3_t	origin;
		vec3_t	angles;
		int		active;
		int		target;
		float	rank;
	} playerData_t;

	typedef struct worldEvent_s
	{
		int				entity1;
		int				entity2;
		unsigned int	flags;
	} worldEvent_t;

	typedef struct worldHistory_s
	{
		float			time;
		unsigned int	seqNr;
		worldEvent_t	event;
		playerData_t	players[MAX_DIRECTOR_PLAYERS];
	} worldHistory_t;

	worldHistory_t* FindBestEvent();
	void CMD_SlowMotion( char* commandLine );
	void ExecuteDirectorCommands();
	void RandomizeCommand( DirectorCmd* command );
	int GetClosestPlayer( frame_s* frame, int entityIndex );
	void AddEvent( int entity1, int entity2, unsigned int flags );
	void SmoothRank( int playerNum, float rank );
	void AnalysePlayer( int playerNum );
	void AnalyseFrame( frame_s* frame );
	void ClearDirectorCommands();
	float AddBestMODCut();
	float AddBestGenericCut();
	void WriteSignonData();
	void WriteProxyStatus( BitBuffer* stream );

	ObjectDictionary	m_Commands;
	worldHistory_t*		history;
	float				maxRank;
	float				gaussFilter[MAX_DIRECTOR_HISTORY];
	int					historyLength;
	unsigned int		nextCutSeqnr;
	unsigned int		currentSeqnr;
	float				nextCutTime;
	float				currentTime;
	float				m_LastExecTime;
	float				m_slowMotion;
	worldEvent_t		frameEvent;
	DirectorCmd*		lastCut;
	IBSPModel*			worldModel;
	bool				active;
	IWorld*				m_World;
	IProxy*				m_Proxy;
};

#ifdef _MSC_VER
#pragma warning( pop )
#endif

float	WeightedAngle( float* vec1, float* vec2 );

#endif
