#pragma once

#include "quakedef.h"

#include <cstddef>

#define ZONE_ID 0x1d4a11
#define ZONE_FREE_TAG 0
#define ZONE_ALLOCATED_TAG 1
#define ZONE_ALIGNMENT 8
#define MINFRAGMENT 64

#define HUNK_SENTINEL 0x1df001ed
#define HUNK_ALIGNMENT 16
#define HUNK_NAME_SIZE 64

#define CACHE_NAME_SIZE 64

typedef struct memblock_s
{
	int size; // including the header and possibly tiny fragments
	int tag;  // a tag of 0 is a free block
	int id; // should be ZONEID
	struct memblock_s* next;
	struct memblock_s* prev;
	int pad; // pad to 64 bit boundary
} memblock_t;

typedef struct memzone_s
{
	int size;			  // total bytes malloced, including header
	memblock_t blocklist; // start / end cap for linked list
	memblock_t* rover;
} memzone_t;

typedef struct hunk_s
{
	int sentinel;
	int size; // including sizeof(hunk_t), -1 = not allocated
	char name[HUNK_NAME_SIZE];
} hunk_t;

typedef struct cache_system_s
{
	int size; // including this header
	cache_user_t* user;
	char name[CACHE_NAME_SIZE];
	struct cache_system_s* prev;
	struct cache_system_s* next;
	struct cache_system_s* lru_prev; // for LRU flushing
	struct cache_system_s* lru_next;
} cache_system_t;

void				Memory_Init( void* buffer, int size );

extern memzone_t* mainzone;

void				Z_ClearZone( memzone_t* zone, int size );
void*				Z_Malloc( int size ); // returns 0 filled memory
void*				Z_TagMalloc( int size, int tag );
void				Z_Free( void* pointer );

extern byte* hunk_base;
extern int hunk_size;
extern int hunk_low_used;
extern int hunk_high_used;
extern qboolean hunk_tempactive;
extern int hunk_tempmark;

void*				Hunk_Alloc( int size ); // returns 0 filled memory
void*				Hunk_AllocName( int size, const char* name );
void*				Hunk_HighAllocName( int size, const char* name );
int					Hunk_LowMark();
void				Hunk_FreeToLowMark( int mark );
int					Hunk_HighMark();
void				Hunk_FreeToHighMark( int mark );
void*				Hunk_TempAlloc( int size );
void				Hunk_Check();

extern cache_system_t cache_head;

void				Cache_Init();
void				Cache_Flush();
void				Cache_Force_Flush();
void				Cache_Report();
void*				Cache_Check( cache_user_t* user );
// returns the cached data, and moves to the head of the LRU list
// if present, otherwise returns NULL

void*				Cache_Alloc( cache_user_t* user, int size, const char* name );
// Returns NULL if all purgable data was tossed and there still
// wasn't enough room.
void				Cache_Free( cache_user_t* user );
cache_system_t*		Cache_TryAlloc( int size, qboolean nobottom );
void				Cache_MakeLRU( cache_system_t* cache );
void				Cache_UnlinkLRU( cache_system_t* cache );
void				Cache_Move( cache_system_t* cache );
void				Cache_FreeLow( int newLowMark );
void				Cache_FreeHigh( int newHighMark );
