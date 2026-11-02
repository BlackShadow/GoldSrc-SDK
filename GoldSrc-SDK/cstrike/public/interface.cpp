//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Main class factory methods
//=============================================================================

#include "interface.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef GetModuleHandle
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

// ------------------------------------------------------------------------------------ //
// InterfaceReg.
// ------------------------------------------------------------------------------------ //
InterfaceReg* InterfaceReg::s_pInterfaceRegs = NULL;

extern "C" void* CreateInterface( const char* name, int* returnCode )
{
	return InterfaceReg::Create( name, returnCode );
}

void* GetModuleHandle( const char* name )
{
#ifdef _WIN32
	return name ? (void*)::GetModuleHandleA( name ) : NULL;
#else
	if ( !name )
		return NULL;
	void* handle = dlopen( name, RTLD_NOW );
	if ( handle )
		dlclose( handle );
	return handle;
#endif
}

void* Sys_GetProcAddress( void* module, const char* name )
{
#ifdef _WIN32
	return (void*)::GetProcAddress( (HMODULE)module, name );
#else
	return dlsym( module, name );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Loads a DLL/component from disk and returns a handle to it
// Input  : *pModuleName - filename of the component
// Output : opaque handle to the module (hides system dependency)
//-----------------------------------------------------------------------------
CSysModule* Sys_LoadModule( const char* name )
{
#ifdef _WIN32
	HMODULE module = ::LoadLibraryA( name );
	if ( !module )
	{
		char pathName[512];
		_snprintf( pathName, sizeof( pathName ), "%s.dll", name );
		pathName[sizeof( pathName ) - 1] = 0;
		module = ::LoadLibraryA( pathName );
	}
	return (CSysModule*)module;
#else
	void* module;
	char absoluteName[1024];
	absoluteName[0] = 0;
	if ( name[0] == '/' )
	{
		snprintf( absoluteName, sizeof( absoluteName ), "%s", name );
		module = dlopen( name, RTLD_NOW );
	}
	else
	{
		char directory[1024];
		char relativeName[1024];
		getcwd( directory, sizeof( directory ) );
		if ( directory[strlen( directory ) - 1] == '/' )
			directory[strlen( directory ) - 1] = 0;
		snprintf( relativeName, sizeof( relativeName ), "%s/%s", directory, name );
		module = dlopen( relativeName, RTLD_NOW );
	}
	if ( !module )
	{
		char retryName[512];
		printf( "Error:%s\n", dlerror() );
		snprintf( retryName, sizeof( retryName ), "%s.so", absoluteName );
		module = dlopen( retryName, RTLD_NOW );
	}
	return (CSysModule*)module;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Unloads a DLL/component from
// Input  : *pModuleName - filename of the component
// Output : opaque handle to the module (hides system dependency)
//-----------------------------------------------------------------------------
void Sys_UnloadModule( CSysModule* module )
{
	if ( !module )
		return;
#ifdef _WIN32
	::FreeLibrary( (HMODULE)module );
#else
	dlclose( module );
#endif
}

CreateInterfaceFn Sys_GetFactory( CSysModule* module )
{
	return module ? (CreateInterfaceFn)Sys_GetProcAddress( module, CREATEINTERFACE_PROCNAME ) : NULL;
}

CreateInterfaceFn Sys_GetFactory( const char* name )
{
	return (CreateInterfaceFn)Sys_GetProcAddress( GetModuleHandle( name ), CREATEINTERFACE_PROCNAME );
}

CreateInterfaceFn Sys_GetFactoryThis()
{
	return CreateInterface;
}
