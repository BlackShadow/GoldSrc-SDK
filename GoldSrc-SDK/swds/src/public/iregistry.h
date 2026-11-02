#pragma once

#ifdef _WIN32
#include "../../../HL25-Engine/engine/registry.h"
#else
//-----------------------------------------------------------------------------
// Purpose: reads and writes the engine's persistent user settings
//-----------------------------------------------------------------------------
class IRegistry
{
public:
	virtual void Init() = 0;
	virtual void Shutdown() = 0;

	virtual int ReadInt( const char* key, int defaultValue ) = 0;
	virtual void WriteInt( const char* key, int value ) = 0;

	virtual const char* ReadString( const char* key, const char* defaultValue ) = 0;
	virtual void WriteString( const char* key, const char* value ) = 0;
};

extern IRegistry* registry;
#endif
