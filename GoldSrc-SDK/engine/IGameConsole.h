#pragma once

#include "interface.h"

#ifdef _MSC_VER
class __declspec( novtable ) IGameConsole : public IBaseInterface
#else
class IGameConsole : public IBaseInterface
#endif
{
public:
	virtual void Activate();
	virtual void Initialize();
	virtual void Hide();
	virtual void Clear();
	virtual bool IsConsoleVisible();

	virtual void Printf( const char* format, ... );
	virtual void DPrintf( const char* format, ... );

	virtual void SetParent( int parent );
};
