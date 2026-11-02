#pragma once

#include "interface.h"

typedef int HKeySymbol;

class IKeyValues : public IBaseInterface
{
public:
	virtual void RegisterSizeofKeyValues( int size );
	virtual void* AllocKeyValuesMemory( int size );
	virtual void FreeKeyValuesMemory( void* memory );

	virtual HKeySymbol GetSymbolForString( const char* name );
	virtual const char* GetStringForSymbol( HKeySymbol symbol );

	virtual void GetLocalizedFromANSI( const char* ansi, wchar_t* localized, int bufferSize );
	virtual void GetANSIFromLocalized( const wchar_t* localized, char* ansi, int bufferSize );

	virtual void AddKeyValuesToMemoryLeakList( void* memory, HKeySymbol name );
	virtual void RemoveKeyValuesFromMemoryLeakList( void* memory );
};
