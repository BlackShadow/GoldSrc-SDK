#ifndef DIRECTORNULL_H
#define DIRECTORNULL_H

#include "HLTV/common/IDirector.h"
#include "HLTV/common/IWorld.h"
#include "HLTV/common/common.h"
#include "common/BaseSystemModule.h"
#include "common/hltv.h"

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4250 )
#endif

class DirectorNull : public IDirector, public BaseSystemModule
{
public:
	virtual void NewGame( IWorld* world, IProxy* ) override
	{
		static unsigned char cmd[] = { 1, DRC_CMD_START };
		world->AddSignonData( svc_director, cmd, sizeof( cmd ) );
	}

	virtual int AddCommand( DirectorCmd* ) override
	{
		return 0;
	}

	virtual void WriteCommands( BitBuffer*, float, float ) override
	{
	}

	virtual char* GetModName() override
	{
		return "NULL";
	}

	virtual bool RemoveCommand( int ) override
	{
		return false;
	}

	virtual DirectorCmd* GetLastCommand() override
	{
		return NULL;
	}

	virtual IObjectContainer* GetCommands() override
	{
		return NULL;
	}
};

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif
