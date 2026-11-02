#include "interface.h"

#if !defined( _WIN32 )
#include <dlfcn.h>
#include <unistd.h>
#else
#include <Windows.h>
#endif

#include <cstdio>
#include <cstring>

#ifdef GetModuleHandle
#undef GetModuleHandle
#endif

// ------------------------------------------------------------------------------------ //
// InterfaceReg.
// ------------------------------------------------------------------------------------ //
InterfaceReg* InterfaceReg::s_pInterfaceRegs;

// ------------------------------------------------------------------------------------ //
// CreateInterface.
// ------------------------------------------------------------------------------------ //
extern "C" void* CreateInterface( const char* pName, int* pReturnCode )
{
	return InterfaceReg::Create( pName, pReturnCode );
}

//-----------------------------------------------------------------------------
// Purpose: returns the instance of this module
//-----------------------------------------------------------------------------
CreateInterfaceFn Sys_GetFactoryThis()
{
	return CreateInterface;
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : module - windows HMODULE from Sys_LoadModule() 
//			*pName - proc name
// Output : factory for this module
//-----------------------------------------------------------------------------
CreateInterfaceFn Sys_GetFactory( CSysModule* pModule )
{
	if ( !pModule )
		return NULL;

#if !defined( _WIN32 )
	return (CreateInterfaceFn)dlsym( pModule, "CreateInterface" );
#else
	return (CreateInterfaceFn)GetProcAddress( (HMODULE)pModule, "CreateInterface" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Loads a DLL/component from disk and returns a handle to it
// Input  : *pModuleName - filename of the component
// Output : opaque handle to the module (hides system dependency)
//-----------------------------------------------------------------------------
CSysModule* Sys_LoadModule( const char* pModuleName )
{
#if !defined( _WIN32 )
	char szAbsoluteModuleName[1024];
	char szCwd[1024];
	char szPath[1024];
	char str[512];
	void* hDLL;

	szAbsoluteModuleName[0] = '\0';
	if ( pModuleName[0] == '/' )
	{
		snprintf( szAbsoluteModuleName, sizeof( szAbsoluteModuleName ), "%s", pModuleName );
		hDLL = dlopen( pModuleName, RTLD_NOW );
		if ( hDLL )
			return (CSysModule*)hDLL;
	}
	else
	{
		getcwd( szCwd, sizeof( szCwd ) );
		if ( szCwd[strlen( szCwd ) - 1] == '/' )
			szCwd[strlen( szCwd ) - 1] = '\0';

		snprintf( szPath, sizeof( szPath ), "%s/%s", szCwd, pModuleName );
		hDLL = dlopen( szPath, RTLD_NOW );
		if ( hDLL )
			return (CSysModule*)hDLL;
	}

	printf( "Error:%s\n", dlerror() );
	snprintf( str, sizeof( str ), "%s.so", szAbsoluteModuleName );
	return (CSysModule*)dlopen( str, RTLD_NOW );
#else
	HMODULE hDLL = LoadLibraryA( pModuleName );
	if ( !hDLL )
	{
		char str[512];

		_snprintf( str, sizeof( str ), "%s.dll", pModuleName );
		str[sizeof( str ) - 1] = '\0';
		hDLL = LoadLibraryA( str );
	}

	return (CSysModule*)hDLL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Unloads a DLL/component from
// Input  : *pModuleName - filename of the component
// Output : opaque handle to the module (hides system dependency)
//-----------------------------------------------------------------------------
void Sys_UnloadModule( CSysModule* pModule )
{
	if ( !pModule )
		return;

#if !defined( _WIN32 )
	dlclose( pModule );
#else
	FreeLibrary( (HMODULE)pModule );
#endif
}

// Linux doesn't have this function so this emulates its functionality
//
//
void* GetModuleHandle( const char* name )
{
	if ( !name )
		return NULL;

#if !defined( _WIN32 )
	void* handle = dlopen( name, RTLD_NOW );
	if ( !handle )
		return NULL;

	// read "man dlopen" for details
	// in short dlopen() inc a ref count
	// so dec the ref count by performing the close
	dlclose( handle );
	return handle;
#else
	return GetModuleHandleA( name );
#endif
}

// Input  : pModuleName - module name
//			*pName - proc name
//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
//-----------------------------------------------------------------------------
// hlds_run wants to use this function 
void* Sys_GetProcAddress( void* pModuleHandle, const char* pName )
{
#if !defined( _WIN32 )
	return dlsym( pModuleHandle, pName );
#else
	return (void*)GetProcAddress( (HMODULE)pModuleHandle, pName );
#endif
}

CreateInterfaceFn Sys_GetFactory( const char* pModuleName )
{
#if !defined( _WIN32 )
	void* handle = NULL;
	if ( pModuleName )
	{
		handle = dlopen( pModuleName, RTLD_NOW );
		if ( handle )
			dlclose( handle );
	}

	return (CreateInterfaceFn)dlsym( handle, CREATEINTERFACE_PROCNAME );
#else
	HMODULE module = GetModuleHandleA( pModuleName );
	if ( !module )
		return NULL;

	return (CreateInterfaceFn)GetProcAddress( module, CREATEINTERFACE_PROCNAME );
#endif
}
