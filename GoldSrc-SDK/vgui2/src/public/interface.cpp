#include "interface.h"

#include <stddef.h>
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

// Linux doesn't have this function so this emulates its functionality
//
//
void* GetModuleHandle( const char* name )
{
#ifdef _WIN32
	return name ? (void*)::GetModuleHandleA( name ) : NULL;
#else
	void* handle;

	if ( name == NULL )
	{
		// hmm, how can this be handled under linux....
		// is it even needed?
		return NULL;
	}

	handle = dlopen( name, RTLD_NOW );

	if ( handle == NULL )
	{
		//printf("Error:%s\n",dlerror());
		// couldn't open this file
		return NULL;
	}

	// read "man dlopen" for details
	// in short dlopen() inc a ref count
	// so dec the ref count by performing the close
	dlclose( handle );
	return handle;
#endif
}

InterfaceReg* InterfaceReg::s_pInterfaceRegs = NULL;

// CreateInterface.
extern "C" void* CreateInterface( const char* pName, int* pReturnCode )
{
	return InterfaceReg::Create( pName, pReturnCode );
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : pModuleName - module name
//			*pName - proc name
//-----------------------------------------------------------------------------
// hlds_run wants to use this function 
void* Sys_GetProcAddress( void* pModuleHandle, const char* pName )
{
#ifdef _WIN32
	return (void*)::GetProcAddress( (HMODULE)pModuleHandle, pName );
#else
	return dlsym( pModuleHandle, pName );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : pModuleName - module name
//			*pName - proc name
//-----------------------------------------------------------------------------
//static hlds_run wants to use this function 
static void* Sys_GetProcAddress( const char* pModuleName, const char* pName )
{
	return Sys_GetProcAddress( GetModuleHandle( pModuleName ), pName );
}

//-----------------------------------------------------------------------------
// Purpose: Loads a DLL/component from disk and returns a handle to it
// Input  : *pModuleName - filename of the component
// Output : opaque handle to the module (hides system dependency)
//-----------------------------------------------------------------------------
CSysModule* Sys_LoadModule( const char* pModuleName )
{
#ifdef _WIN32
	return (CSysModule*)::LoadLibraryA( pModuleName );
#else
	void* hDLL = NULL;
	char szAbsoluteModuleName[1024];

	szAbsoluteModuleName[0] = 0;

	if ( pModuleName[0] != '/' )
	{
		char szCwd[1024];
		char szAbsoluteModuleName[1024];

		getcwd( szCwd, sizeof( szCwd ) );

		if ( szCwd[strlen( szCwd ) - 1] == '/' )
		{
			szCwd[strlen( szCwd ) - 1] = 0;
		}

		snprintf( szAbsoluteModuleName, sizeof( szAbsoluteModuleName ), "%s/%s", szCwd, pModuleName );
		hDLL = dlopen( szAbsoluteModuleName, RTLD_NOW );
	}
	else
	{
		snprintf( szAbsoluteModuleName, sizeof( szAbsoluteModuleName ), "%s", pModuleName );
		hDLL = dlopen( pModuleName, RTLD_NOW );
	}

	if ( !hDLL )
	{
		char str[512];

		printf( "Error:%s\n", dlerror() );
		snprintf( str, sizeof( str ), "%s.so", szAbsoluteModuleName );
		hDLL = dlopen( str, RTLD_NOW );
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
	void* hDLL;

	if ( !pModule )
	{
		return;
	}

	hDLL = pModule;
#ifdef _WIN32
	::FreeLibrary( (HMODULE)hDLL );
#else
	dlclose( hDLL );
#endif
}

CreateInterfaceFn Sys_GetFactory( CSysModule* pModule )
{
	void* hDLL;

	if ( !pModule )
	{
		return NULL;
	}

	hDLL = pModule;
	return (CreateInterfaceFn)Sys_GetProcAddress( hDLL, CREATEINTERFACE_PROCNAME );
}

CreateInterfaceFn Sys_GetFactoryThis()
{
	return CreateInterface;
}

CreateInterfaceFn Sys_GetFactory( const char* pModuleName )
{
	return (CreateInterfaceFn)Sys_GetProcAddress( pModuleName, CREATEINTERFACE_PROCNAME );
}
