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
//						ZONE MEMORY ALLOCATION
//
// There is never any space between memblocks, and there will never be two
// contiguous free memblocks.
//
// The rover can be left pointing at a non-empty block
//
// The zone calls are pretty much only used for small strings and structures,
// all big things are allocated on the hunk.
//=============================================================================	   

#include "zone.h"
#include "common.h"
#include "cmd.h"
#include "client.h"
#include "cvar.h"
#include "conprint.h"
#include "sys.h"

#include <cstdio>
#include <cstdlib>

#define DYNAMIC_SIZE 0x200000

cvar_t mem_dbgfile = { "mem_dbgfile", ".\\mem.txt" };

void Sys_SplitPath( const char* path, char* drive, char* dir, char* fname, char* ext );

/*
==============================================================================

						ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

memzone_t* mainzone;

void* Z_TagMalloc( int size, int tag )
{
	int extra;
	memblock_t *start, *rover, *newmem, *base;

	if ( tag == ZONE_FREE_TAG )
		Sys_Error( "Z_TagMalloc: tried to use a 0 tag" );

	//
	// scan through the block list looking for the first free block
	// of sufficient size
	//
	size += sizeof( memblock_t ); // account for size of block header
	size += 4;                    // space for memory trash tester
	size = ( size + 7 ) & ~7;     // align to 8-byte boundary

	base = rover = mainzone->rover;
	start = base->prev;

	do
	{
		if ( rover == start ) // scaned all the way around the list
			return NULL;
		if ( rover->tag != ZONE_FREE_TAG )
			base = rover = rover->next;
		else
			rover = rover->next;
	} while ( base->tag != ZONE_FREE_TAG || base->size < size );

	//
	// found a block big enough
	//
	extra = base->size - size;
	if ( extra > MINFRAGMENT )
	{
		// there will be a free fragment after the allocated block
		newmem = (memblock_t*)( (byte*)base + size );
		newmem->size = extra;
		newmem->tag = ZONE_FREE_TAG;
		newmem->prev = base;
		newmem->id = ZONE_ID;
		newmem->next = base->next;
		newmem->next->prev = newmem;
		base->next = newmem;
		base->size = size;
	}

	base->tag = tag; // no longer a free block

	mainzone->rover = base->next; // next allocation will start looking here

	base->id = ZONE_ID;

	// marker for memory trash testing
	*(int*)( (byte*)base + base->size - 4 ) = ZONE_ID;

	return (void*)( base + 1 );
}

/*
========================
Z_Malloc
========================
*/
void* Z_Malloc( int size )
{
	void* buf;

	buf = Z_TagMalloc( size, ZONE_ALLOCATED_TAG );
	if ( !buf )
		Sys_Error( "Z_Malloc: failed on allocation of %i bytes", size );
	Q_memset( buf, 0, size );

	return buf;
}

/*
========================
Z_Free
========================
*/
void Z_Free( void* ptr )
{
	memblock_t *block, *other;

	if ( !ptr )
		Sys_Error( "Z_Free: NULL pointer" );

	block = (memblock_t*)ptr - 1;
	if ( block->id != ZONE_ID )
		Sys_Error( "Z_Free: freed a pointer without ZONEID" );
	if ( block->tag == ZONE_FREE_TAG )
		Sys_Error( "Z_Free: freed a freed pointer" );

	other = block->prev;
	block->tag = ZONE_FREE_TAG; // mark as free

	if ( other->tag == ZONE_FREE_TAG )
	{
		// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if ( block == mainzone->rover )
			mainzone->rover = other;
		block = other;
	}

	other = block->next;
	if ( other->tag == ZONE_FREE_TAG )
	{
		// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if ( other == mainzone->rover )
			mainzone->rover = block;
	}
}

byte* hunk_base;
int hunk_size;

int hunk_low_used;
int hunk_high_used;

qboolean hunk_tempactive;
int hunk_tempmark;

/*
===================
Hunk_AllocName
===================
*/
void* Hunk_AllocName( int size, const char* name )
{
	hunk_t* h;

	if ( size < 0 )
		Sys_Error( "Hunk_Alloc: bad size: %i", size );

	size = sizeof( hunk_t ) + ( ( size + 15 ) & ~15 );

	if ( hunk_size - hunk_high_used - hunk_low_used < size )
		Sys_Error( "Hunk_Alloc: failed on %i bytes", size );

	h = (hunk_t*)( hunk_base + hunk_low_used );
	hunk_low_used += size;

	Cache_FreeLow( hunk_low_used );

	Q_memset( h, 0, size );

	h->size = size;
	h->sentinel = HUNK_SENTINEL;
	Q_strncpy( h->name, name, sizeof( h->name ) );
	h->name[sizeof( h->name ) - 1] = 0;

	return (void*)( h + 1 );
}

/*
===================
Hunk_Alloc
===================
*/
void* Hunk_Alloc( int size )
{
	return Hunk_AllocName( size, "unknown" );
}

/*
==============
Hunk_Check

Run consistancy and sentinal trahing checks
==============
*/
void Hunk_Check()
{
	hunk_t* h;

	for ( h = (hunk_t*)hunk_base; (byte*)h != hunk_base + hunk_low_used; )
	{
		if ( h->sentinel != HUNK_SENTINEL )
			Sys_Error( "Hunk_Check: trahsed sentinal" );
		if ( h->size < 16 || h->size + (byte*)h - hunk_base > hunk_size )
			Sys_Error( "Hunk_Check: bad size" );
		h = (hunk_t*)( (byte*)h + h->size );
	}
}

int Hunk_LowMark()
{
	return hunk_low_used;
}

void Hunk_FreeToLowMark( int mark )
{
	if ( mark < 0 || mark > hunk_low_used )
		Sys_Error( "Hunk_FreeToLowMark: bad mark %i", mark );

	hunk_low_used = mark;
}

void Hunk_FreeToHighMark( int mark )
{
	if ( hunk_tempactive )
	{
		hunk_tempactive = FALSE;
		Hunk_FreeToHighMark( hunk_tempmark );
	}
	if ( mark < 0 || mark > hunk_high_used )
		Sys_Error( "Hunk_FreeToHighMark: bad mark %i", mark );

	hunk_high_used = mark;
}

int Hunk_HighMark()
{
	if ( hunk_tempactive )
	{
		hunk_tempactive = FALSE;
		Hunk_FreeToHighMark( hunk_tempmark );
	}

	return hunk_high_used;
}

/*
===================
Hunk_HighAllocName
===================
*/
void* Hunk_HighAllocName( int size, const char* name )
{
	hunk_t* h;

	if ( size < 0 )
		Sys_Error( "Hunk_HighAllocName: bad size: %i", size );

	if ( hunk_tempactive )
	{
		hunk_tempactive = FALSE;
		Hunk_FreeToHighMark( hunk_tempmark );
	}

	size = sizeof( hunk_t ) + ( ( size + 15 ) & ~15 );

	if ( hunk_size - hunk_high_used - hunk_low_used < size )
	{
		Con_Printf( "Hunk_HighAlloc: failed on %i bytes\n", size );
		return NULL;
	}

	hunk_high_used += size;
	Cache_FreeHigh( hunk_high_used );

	h = (hunk_t*)( hunk_base + hunk_size - hunk_high_used );

	Q_memset( h, 0, size );
	h->size = size;
	h->sentinel = HUNK_SENTINEL;
	Q_strncpy( h->name, name, sizeof( h->name ) );
	h->name[sizeof( h->name ) - 1] = 0;

	return (void*)( h + 1 );
}

/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
void* Hunk_TempAlloc( int size )
{
	void* buf;

	size = ( size + 15 ) & ~15;

	hunk_tempmark = Hunk_HighMark();

	buf = Hunk_HighAllocName( size, "temp" );

	hunk_tempactive = TRUE;

	return buf;
}

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/
cache_system_t cache_head;

void Cache_UnlinkLRU( cache_system_t* cs )
{
	if ( !cs->lru_next || !cs->lru_prev )
		Sys_Error( "Cache_UnlinkLRU: NULL link" );

	cs->lru_next->lru_prev = cs->lru_prev;
	cs->lru_prev->lru_next = cs->lru_next;

	cs->lru_prev = cs->lru_next = NULL;
}

void Cache_MakeLRU( cache_system_t* cs )
{
	if ( cs->lru_next || cs->lru_prev )
		Sys_Error( "Cache_MakeLRU: active link" );

	cache_head.lru_next->lru_prev = cs;
	cs->lru_next = cache_head.lru_next;
	cs->lru_prev = &cache_head;
	cache_head.lru_next = cs;
}

/*
==============
Cache_Check
==============
*/
void* Cache_Check( cache_user_t* c )
{
	cache_system_t* cs;

	if ( !c->data )
		return NULL;

	cs = (cache_system_t*)c->data - 1;

	// move to head of LRU
	Cache_UnlinkLRU( cs );
	Cache_MakeLRU( cs );

	return c->data;
}

/*
==============
Cache_Free

Frees the memory and removes it from the LRU list
==============
*/
void Cache_Free( cache_user_t* c )
{
	cache_system_t* cs;

	if ( !c->data )
		Sys_Error( "Cache_Free: not allocated" );

	cs = (cache_system_t*)c->data - 1;

	cs->prev->next = cs->next;
	cs->next->prev = cs->prev;
	cs->prev = cs->next = NULL;

	c->data = NULL;

	Cache_UnlinkLRU( cs );
}

/*
============
Cache_TryAlloc

Looks for a free block of memory between the high and low hunk marks
Size should already include the header and padding
============
*/
cache_system_t* Cache_TryAlloc( int size, qboolean nobottom )
{
	cache_system_t *cs, *newmem;

	// is the cache completely empty?
	if ( !nobottom && cache_head.prev == &cache_head )
	{
		if ( hunk_size - hunk_low_used - hunk_high_used < size )
			Sys_Error( "Cache_TryAlloc: %i is greater then free hunk", size );

		newmem = (cache_system_t*)( hunk_base + hunk_low_used );
		Q_memset( newmem, 0, sizeof( *newmem ) );
		newmem->size = size;

		cache_head.next = cache_head.prev = newmem;
		newmem->next = newmem->prev = &cache_head;

		Cache_MakeLRU( newmem );
		return newmem;
	}

	// search from the bottom up for space
	newmem = (cache_system_t*)( hunk_base + hunk_low_used );
	cs = cache_head.next;

	for ( ;; )
	{
		if ( ( !nobottom || cs != cache_head.next ) && (byte*)cs - (byte*)newmem >= size )
			break; // found space

		// continue looking
		newmem = (cache_system_t*)( (byte*)cs + cs->size );
		cs = cs->next;

		if ( cs == &cache_head )
		{
			// try to allocate one at the very end
			if ( hunk_base + hunk_size - hunk_high_used - (byte*)newmem < size )
				return NULL; // couldn't allocate
			break;
		}
	}

	Q_memset( newmem, 0, sizeof( *newmem ) );
	newmem->size = size;

	newmem->next = cs;
	newmem->prev = cs->prev;
	cs->prev->next = newmem;
	cs->prev = newmem;

	Cache_MakeLRU( newmem );

	return newmem;
}

/*
==============
Cache_Alloc
==============
*/
void* Cache_Alloc( cache_user_t* c, int size, const char* name )
{
	cache_system_t* cs;

	if ( c->data )
		Sys_Error( "Cache_Alloc: already allocated" );

	if ( size <= 0 )
		Sys_Error( "Cache_Alloc: size %i", size );

	size = ( size + sizeof( cache_system_t ) + 15 ) & ~15;

	// find memory for it
	for ( ;; )
	{
		cs = Cache_TryAlloc( size, FALSE );
		if ( cs )
			break;

		// free the least recently used entry
		// assume if it's a locked entry that we've run out.
		if ( cache_head.lru_prev == &cache_head )
			Sys_Error( "Cache_Alloc: out of memory" );
		// not enough memory at all
		Cache_Free( cache_head.lru_prev->user );
	}

	Q_strncpy( cs->name, name, sizeof( cs->name ) - 1 );
	cs->name[sizeof( cs->name ) - 1] = 0;
	c->data = cs + 1;
	cs->user = c;

	return Cache_Check( c );
}

/*
===========
Cache_Move
===========
*/
void Cache_Move( cache_system_t* c )
{
	cache_system_t* newmem;

	// we are clearing up space at the bottom, so only allocate it late
	newmem = Cache_TryAlloc( c->size, TRUE );
	if ( newmem )
	{
//		Con_Printf ("cache_move ok\n");
		Q_memcpy( newmem + 1, c + 1, c->size - sizeof( cache_system_t ) );
		newmem->user = c->user;
		Q_memcpy( newmem->name, c->name, sizeof( newmem->name ) );
		Cache_Free( c->user );
		newmem->user->data = (void*)( newmem + 1 );
	}
	else
	{
//		Con_Printf ("cache_move failed\n");
		Cache_Free( c->user ); // tough luck...
	}
}

/*
============
Cache_FreeLow

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeLow( int new_low_hunk )
{
	cache_system_t* c;

	for ( ;; )
	{
		c = cache_head.next;
		if ( c == &cache_head )
			return; // nothing in cache at all
		if ( (byte*)c >= hunk_base + new_low_hunk )
			return; // there is space to grow the hunk
		Cache_Move( c ); // reclaim the space
	}
}

/*
============
Cache_FreeHigh

Throw things out until the hunk can be expanded to the given point
============
*/
void Cache_FreeHigh( int new_high_hunk )
{
	cache_system_t *c, *prev;

	prev = NULL;
	for ( ;; )
	{
		c = cache_head.prev;
		if ( c == &cache_head )
			return; // nothing in cache at all
		if ( (byte*)c + c->size <= hunk_base + hunk_size - new_high_hunk )
			return; // there is space to grow the hunk
		if ( c == prev )
		{
			Cache_Free( c->user ); // didn't move out of the way
		}
		else
		{
			Cache_Move( c ); // try to move it
			prev = c;
		}
	}
}

void Cache_Force_Flush()
{
	while ( cache_head.next != &cache_head )
		Cache_Free( cache_head.next->user ); // reclaim the space
}

void Cache_Flush()
{
	if ( cl.maxclients > 1 && !sv_cheats.value )
	{
		Con_Printf( "Server must enable sv_cheats to activate the flush command in multiplayer games.\n" );
		return;
	}

	Cache_Force_Flush();
}

/*
============
Cache_Report

============
*/
void Cache_Report()
{
	Con_DPrintf( "%4.1f megabyte data cache\n", ( hunk_size - hunk_low_used - hunk_high_used ) / (float)( 1024 * 1024 ) );
}

/*
============
Cache_Init

============
*/
void Cache_Init()
{
	cache_head.next = cache_head.prev = &cache_head;
	cache_head.lru_next = cache_head.lru_prev = &cache_head;

	Cmd_AddCommand( "flush", Cache_Flush );
}

//============================================================================

/*
========================
Memory_Init
========================
*/
void Memory_Init( void* buf, int size )
{
	int p;
	int zonesize = DYNAMIC_SIZE;
	memzone_t* zone;

	hunk_base = (byte*)buf;
	hunk_size = size;
	hunk_low_used = 0;
	hunk_high_used = 0;
	hunk_tempactive = FALSE;
	hunk_tempmark = 0;

	Cache_Init();

	p = COM_CheckParm( "-zone" );
	if ( p )
	{
		if ( p >= com_argc - 1 )
			Sys_Error( "Memory_Init: you must specify a size in KB after -zone" );
		zonesize = Q_atoi( com_argv[p + 1] ) * 1024;
	}

	zone = (memzone_t*)Hunk_AllocName( zonesize, "zone" );
	Z_ClearZone( zone, zonesize );
	mainzone = zone;
}

/*
========================
Z_ClearZone
========================
*/
void Z_ClearZone( memzone_t* zone, int size )
{
	memblock_t* block;

	// set the entire zone to one free block
	zone->blocklist.next = zone->blocklist.prev = block = (memblock_t*)( zone + 1 );
	zone->rover = block;
	block->prev = block->next = &zone->blocklist;
	zone->blocklist.tag = ZONE_ALLOCATED_TAG; // in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;

	block->tag = ZONE_FREE_TAG; // free block
	block->id = ZONE_ID;
	block->size = size - sizeof( memzone_t );
}

/*
========================
Z_Print
========================
*/
void Z_Print( memzone_t* zone )
{
	memblock_t* block;

	Con_Printf( "zone size: %i  location: %p\n", mainzone->size, mainzone );

	for ( block = zone->blocklist.next;; block = block->next )
	{
		Con_Printf( "block:%p    size:%7i    tag:%3i\n", block, block->size, block->tag );

		if ( block->next == &zone->blocklist )
			break; // all blocks have been hit
		if ( (byte*)block + block->size != (byte*)block->next )
			Con_Printf( "ERROR: block size does not touch the next block\n" );
		if ( block->next->prev != block )
			Con_Printf( "ERROR: next block doesn't have proper back link\n" );
		if ( block->tag == ZONE_FREE_TAG && block->next->tag == ZONE_FREE_TAG )
			Con_Printf( "ERROR: two consecutive free blocks\n" );
	}
}

/*
========================
Z_CheckHeap
========================
*/
void Z_CheckHeap()
{
	memblock_t* block;

	for ( block = mainzone->blocklist.next;; block = block->next )
	{
		if ( block->next == &mainzone->blocklist )
			break; // all blocks have been hit
		if ( (byte*)block + block->size != (byte*)block->next )
			Sys_Error( "Z_CheckHeap: block size does not touch the next block\n" );
		if ( block->next->prev != block )
			Sys_Error( "Z_CheckHeap: next block doesn't have proper back link\n" );
		if ( block->tag == ZONE_FREE_TAG && block->next->tag == ZONE_FREE_TAG )
			Sys_Error( "Z_CheckHeap: two consecutive free blocks\n" );
	}
}

char* CommatizeNumber( int num, char* pout )
{
	int i, len;
	char outbuf[50];
	char tempbuf[50];

	Q_memset( outbuf, 0, sizeof( outbuf ) );
	while ( num )
	{
		int temp = num % 1000;
		num = num / 1000;
		Q_strncpy( tempbuf, outbuf, sizeof( tempbuf ) - 1 );
		tempbuf[sizeof( tempbuf ) - 1] = 0;

		snprintf( outbuf, sizeof( outbuf ), ",%03i%s", temp, tempbuf );
	}

	len = Q_strlen( outbuf );

	//find first significant digit
	for ( i = 0; i < len; i++ )
	{
		if ( outbuf[i] != ',' && outbuf[i] != '0' )
			break;
	}

	if ( i == len )
		Q_strcpy( pout, "0" );
	else
		Q_strcpy( pout, &outbuf[i] ); //copy from i to get rid of the first comma and leading zeros

	return pout;
}

/*
==============
Hunk_Print

If "all" is specified, every single allocation is printed.
Otherwise, allocations with the same name will be totaled up before printing.
==============
*/
void Hunk_Print( qboolean all )
{
	hunk_t *h, *next, *endlow, *starthigh, *endhigh;
	int sum;
	int totalblocks;
	char name[HUNK_NAME_SIZE + 1];
	char commabuf[50];
	FileHandle_t file;

	name[HUNK_NAME_SIZE] = 0;

	file = FS_Open( mem_dbgfile.string, "a" );
	if ( !file )
		return;

	sum = 0;
	totalblocks = 0;

	h = (hunk_t*)hunk_base;
	endlow = (hunk_t*)( hunk_base + hunk_low_used );
	starthigh = (hunk_t*)( hunk_base + hunk_size - hunk_high_used );
	endhigh = (hunk_t*)( hunk_base + hunk_size );

	FS_FPrintf( file, "          :%16.16s total hunk size\n", CommatizeNumber( hunk_size, commabuf ) );
	FS_FPrintf( file, "-------------------------\n" );

	for ( ;; )
	{
		//
		// skip to the high hunk if done with low hunk
		//
		if ( h == endlow )
		{
			FS_FPrintf( file, "-------------------------\n" );
			FS_FPrintf( file, "          :%16.16s REMAINING\n",
				CommatizeNumber( hunk_size - hunk_low_used - hunk_high_used, commabuf ) );
			FS_FPrintf( file, "-------------------------\n" );
			h = starthigh;
		}

		//
		// if totally done, break
		//
		if ( h == endhigh )
			break;

		//
		// run consistancy checks
		//
		if ( h->sentinel != HUNK_SENTINEL )
			Sys_Error( "Hunk_Check: trahsed sentinal" );
		if ( h->size < 16 || h->size + (byte*)h - hunk_base > hunk_size )
			Sys_Error( "Hunk_Check: bad size" );

		next = (hunk_t*)( (byte*)h + h->size );
		totalblocks++;
		sum += h->size;

		//
		// print the single block
		//
		Q_memcpy( name, h->name, HUNK_NAME_SIZE );
		if ( all )
			FS_FPrintf( file, "%8p :%16.16s %16s\n", h, CommatizeNumber( h->size, commabuf ), name );

		//
		// print the total
		//
		if ( next == endhigh || next == endlow || Q_strncmp( h->name, next->name, HUNK_NAME_SIZE ) )
		{
			if ( !all )
				FS_FPrintf( file, "          :%16.16s %16s (TOTAL)\n", CommatizeNumber( sum, commabuf ), name );
			sum = 0;
		}

		h = next;
	}

	FS_FPrintf( file, "-------------------------\n" );
	FS_FPrintf( file, "%8i total blocks\n", totalblocks );

	FS_Close( file );
}

/*
============
CacheSystemCompare

Compares the names of two cache_system_t structs.
Used with qsort()
============
*/
int CacheSystemCompare( const void* ppcs1, const void* ppcs2 )
{
	cache_system_t* pcs1 = *(cache_system_t**)ppcs1;
	cache_system_t* pcs2 = *(cache_system_t**)ppcs2;
//	char buf[400];
//	sprintf(buf,"Comparing \"%s\" and \"%s\"\n",pcs1->name,pcs2->name);
//	OutputDebugString(buf);

	return Q_stricmp( pcs1->name, pcs2->name );
}

/*
============
ComparePath1

compares the first directory of two paths...
(so  "foo/bar" will match "foo/fred"
============
*/
int ComparePath1( char* path1, char* path2 )
{
	while ( *path1 != '\\' && *path1 != '/' && *path1 )
	{
		if ( *path1 != *path2 )
			return 0;
		path1++;
		path2++;
	}
	return 1;
}

/*
============
Cache_Print

============
*/
void Cache_Print()
{
	cache_system_t* cd;
	cache_system_t* sortarray[512];
	int i, j;
	FileHandle_t file;
	char commabuf[50];

	file = FS_Open( mem_dbgfile.string, "a" );
	if ( !file )
		return;

	Q_memset( sortarray, sizeof( sortarray ), 0 );

	FS_FPrintf( file, "\nCACHE:\n" );

	i = 0;
	for ( cd = cache_head.next; cd != &cache_head; cd = cd->next )
		sortarray[i++] = cd;

	//Sort the array alphabetically
	qsort( sortarray, i, sizeof( cache_system_t* ), CacheSystemCompare );

	for ( j = 0; j < i; j++ )
		FS_FPrintf( file, "%16.16s : %-16s\n", CommatizeNumber( sortarray[j]->size, commabuf ), sortarray[j]->name );

	FS_Close( file );
}

/*
============
Cache_Compact

============
*/
void Cache_Compact()
{
}

int Cache_TotalUsed()
{
	cache_system_t* cd;
	int total = 0;

	for ( cd = cache_head.next; cd != &cache_head; cd = cd->next )
		total += cd->size;

	return total;
}

void Cache_Print_Models_And_Totals()
{
	char buf[50];
	cache_system_t* cd;
	cache_system_t* sortarray[512];
	int i, j;
	int totalbytes;
	FileHandle_t file;

	file = FS_Open( mem_dbgfile.string, "a" );
	if ( !file )
		return;

	Q_memset( sortarray, sizeof( sortarray ), 0 );

	//pack names into the array.
	i = 0;
	for ( cd = cache_head.next; cd != &cache_head; cd = cd->next )
	{
		if ( Q_strstr( cd->name, ".mdl" ) )
			sortarray[i++] = cd;
	}

	qsort( sortarray, i, sizeof( cache_system_t* ), CacheSystemCompare );

	FS_FPrintf( file, "\nCACHED MODELS:\n" );

	//now process the sorted list.  
	totalbytes = 0;
	for ( j = 0; j < i; j++ )
	{
		FS_FPrintf( file, "\t%16.16s : %s\n", CommatizeNumber( sortarray[j]->size, buf ), sortarray[j]->name );
		totalbytes += sortarray[j]->size;
	}

	FS_FPrintf( file, "Total bytes in cache used by models:  %s\n", CommatizeNumber( totalbytes, buf ) );
	FS_Close( file );
}

/*
============
Cache_Print_Sounds_And_Totals

Prints out which sounds are in the cache, how much space they take, and the
directories that they're in.
============
*/
void Cache_Print_Sounds_And_Totals()
{
	char buf[50];
	char pathbuf[512];
	cache_system_t* cd;
	cache_system_t* sortarray[1024];
	int i, j;
	int totalsndbytes;
	int subtot;
	FileHandle_t file;

	file = FS_Open( mem_dbgfile.string, "a" );
	if ( !file )
		return;

	Q_memset( sortarray, sizeof( sortarray ), 0 );

	i = 0;
	//pack names into the array.
	for ( cd = cache_head.next; cd != &cache_head; cd = cd->next )
	{
		if ( Q_strstr( cd->name, ".wav" ) )
			sortarray[i++] = cd;
	}

	qsort( sortarray, i, sizeof( cache_system_t* ), CacheSystemCompare );

	FS_FPrintf( file, "\nCACHED SOUNDS:\n" );

	//now process the sorted list.  (totals by directory)
	subtot = 0;
	totalsndbytes = 0;
	for ( j = 0; j < i; j++ )
	{
		FS_FPrintf( file, "\t%16.16s : %s\n", CommatizeNumber( sortarray[j]->size, buf ), sortarray[j]->name );
		totalsndbytes += sortarray[j]->size;

		if ( j + 1 == i || !ComparePath1( sortarray[j]->name, sortarray[j + 1]->name ) )
		{
			Sys_SplitPath( sortarray[j]->name, NULL, pathbuf, NULL, NULL );
			FS_FPrintf(
				file, "\tTotal Bytes used in \"%s\": %s\n", pathbuf, CommatizeNumber( totalsndbytes - subtot, buf ) );
			subtot = totalsndbytes;
		}
	}

	FS_FPrintf( file, "Total bytes in cache used by sound:  %s\n", CommatizeNumber( totalsndbytes, buf ) );
	FS_Close( file );
}
