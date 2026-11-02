#pragma once

#include "const.h"
#include "mathlib.h"
#include "filesystem.h"
#include "custom.h"

void		HPAK_Init();
void		HPAK_FlushHostQueue();
void		HPAK_CreatePak( const char* pakname, resource_t* resource, void* data, FileHandle_t source );
void		HPAK_AddLump( qboolean queue, const char* pakname, resource_t* resource, void* data, FileHandle_t source );
void		HPAK_RemoveLump( const char* pakname, resource_t* resource );
qboolean	HPAK_GetDataPointer( const char* pakname, resource_t* resource, byte** buffer, int* size );
qboolean	HPAK_ResourceForHash( const char* pakname, byte* hash, resource_t* resource );
qboolean	HPAK_ResourceForIndex( const char* pakname, int index, resource_t* resource );
void		HPAK_CheckSize( const char* pakname );
void		HPAK_CheckIntegrity( const char* pakname );
void		HPAK_ValidatePak( const char* fullpakname );

void		HPAK_List_f();
void		HPAK_Remove_f();
void		HPAK_Validate_f();
void		HPAK_Extract_f();
