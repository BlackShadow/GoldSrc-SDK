#include "basefilesystem.h"

#include <cstdio>
#include <cstring>

#ifndef _WIN32
#define FS_MODULE_PATH_CAPACITY	((size_t)1024)
#define FS_MODULE_SUFFIX_LIMIT	((size_t)512)
#endif // LINUX

CSysModule* Sys_LoadModule( const char* pModuleName )
{
#ifdef _WIN32
	return (CSysModule*)LoadLibraryA( pModuleName );
#else
	char szAbsoluteModuleName[FS_MODULE_PATH_CAPACITY];
	char szCwd[FS_MODULE_PATH_CAPACITY];
	char szModuleName[FS_MODULE_PATH_CAPACITY];
	CSysModule* module;

	szAbsoluteModuleName[0] = 0;
	if ( *pModuleName == '/' )
	{
		snprintf( szAbsoluteModuleName, sizeof( szAbsoluteModuleName ), "%s", pModuleName );
		module = (CSysModule*)dlopen( pModuleName, RTLD_NOW );
		if ( module != NULL )
		{
			return module;
		}
	}
	else
	{
		unsigned int last;

		getcwd( szCwd, sizeof( szCwd ) );
		last = (unsigned int)strlen( szCwd ) - 1u;
		if ( szCwd[last] == '/' )
		{
			szCwd[last] = 0;
		}

		snprintf( szModuleName, sizeof( szModuleName ), "%s/%s", szCwd, pModuleName );
		module = (CSysModule*)dlopen( szModuleName, RTLD_NOW );
		if ( module != NULL )
		{
			return module;
		}
	}

	printf( "Error:%s\n", dlerror() );
	snprintf( szModuleName, FS_MODULE_SUFFIX_LIMIT, "%s.so", szAbsoluteModuleName );
	return (CSysModule*)dlopen( szModuleName, RTLD_NOW );
#endif
}

void Sys_UnloadModule( CSysModule* pModule )
{
	if ( pModule != NULL )
	{
#ifdef _WIN32
		FreeLibrary( (HMODULE)pModule );
#else
		dlclose( pModule );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to a function, given a module
// Input  : pModuleName - module name
//			*pName - proc name
//-----------------------------------------------------------------------------
// hlds_run wants to use this function 
void* Sys_GetProcAddress( void* pModule, const char* pName )
{
#ifdef _WIN32
	return (void*)GetProcAddress( (HMODULE)pModule, pName );
#else
	return dlsym( pModule, pName );
#endif
}

CreateInterfaceFn Sys_GetFactory( CSysModule* pModule )
{
	if ( pModule != NULL )
	{
		return (CreateInterfaceFn)Sys_GetProcAddress( pModule, "CreateInterface" );
	}

	return NULL;
}

CreateInterfaceFn Sys_GetFactoryThis()
{
#ifdef _WIN32
	return CreateInterface;
#else
	return NULL;
#endif // LINUX
}

CreateInterfaceFn Sys_GetFactory( const char* pModuleName )
{
#ifdef _WIN32
	return Sys_GetFactory( (CSysModule*)GetModuleHandleA( pModuleName ) );
#else
	void* opened;

	if ( pModuleName != NULL )
	{
		opened = dlopen( pModuleName, RTLD_NOW );
		if ( opened != NULL )
		{
			dlclose( opened );
		}
		else
		{
			opened = NULL;
		}
	}
	else
	{
		opened = NULL;
	}

	return (CreateInterfaceFn)dlsym( opened, "CreateInterface" );
#endif
}

InterfaceReg* InterfaceReg::s_pInterfaceRegs = NULL;
