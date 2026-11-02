#include "common.h"

#include <cstdio>

#define MAX_MODULE_PATH 260

static char moduleServerFileName[MAX_MODULE_PATH];
static char moduleClientFileName[MAX_MODULE_PATH];

static char* ModuleFileName( char* dest, const char* moduleName )
{
	char gameName[MAX_MODULE_PATH];

	COM_FileBase( com_gamedir, gameName );
	snprintf( dest, MAX_MODULE_PATH - 1, "%s/%s", gameName, moduleName );
	dest[MAX_MODULE_PATH - 1] = 0;
	return dest;
}

char* SV_GetFileNameModuleS()
{
	return ModuleFileName( moduleServerFileName, "ModuleS_i386.so" );
}

char* SV_GetFileNameModuleC()
{
	return ModuleFileName( moduleClientFileName, "ModuleC.dll" );
}
