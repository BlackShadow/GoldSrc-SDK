#ifndef ENGINE_LAUNCHER_API_H
#define ENGINE_LAUNCHER_API_H

#include "tier1/interface.h"

static const char kEngineLauncherVersion[] = "VENGINE_LAUNCHER_API_VERSION002";

class IEngineAPI : public IBaseInterface
{
public:
	virtual int Run( void* instance, char* baseDirectory, const char* commandLine, char* postRestartCommandLine,
		CreateInterfaceFn launcherFactory, CreateInterfaceFn fileSystemFactory ) = 0;
};

enum
{
	kEngineRunNone				= 0,
	kEngineRunRestart			= 1,
	kEngineRunVideoModeFailed	= 2
};

#endif
