#ifndef IDIRECTOR_H
#define IDIRECTOR_H

#include "common/ISystemModule.h"

class IWorld;
class IProxy;
class BitBuffer;
class DirectorCmd;
class IObjectContainer;

class IDirector : public virtual ISystemModule
{
public:
	virtual ~IDirector() override
	{
	}

	virtual void NewGame( IWorld* world, IProxy* proxy ) = 0;
	virtual char* GetModName() = 0;

	virtual void WriteCommands( BitBuffer* stream, float startTime, float endTime ) = 0;
	virtual int AddCommand( DirectorCmd* command ) = 0;
	virtual bool RemoveCommand( int index ) = 0;
	virtual DirectorCmd* GetLastCommand() = 0;
	virtual IObjectContainer* GetCommands() = 0;
};

#endif
