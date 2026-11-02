//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "MemPool.h"

#include <math.h>
#include <stdlib.h>

#ifdef _WIN32
#include <intrin.h>

#define MEMPOOL_ASSERT( exp ) \
	do \
	{ \
		if ( !( exp ) ) \
			__debugbreak(); \
	} while ( 0 )
#else
#define MEMPOOL_ASSERT( exp ) ( (void)0 )
#endif // _DEBUG

CMemoryPool::CMemoryPool( int blockSize, int numElements ) :
	_blockSize( blockSize )
{
	_numBlobs = 0;
	_numElements = 0;
	_blocksPerBlob = numElements;

	AddNewBlob();

	_peakAlloc = 0;
	_blocksAllocated = 0;
}

CMemoryPool::~CMemoryPool()
{
	for ( int i = 0; i < _numBlobs; i++ )
	{
		free( _memBlob[i] );
	}
}

void CMemoryPool::AddNewBlob()
{
	int sizeMultiplier = (int)pow( 2.0, (double)_numBlobs );
	int numBlocks = _blocksPerBlob * sizeMultiplier;

	_memBlob[_numBlobs] = malloc( _blockSize * numBlocks );
	MEMPOOL_ASSERT( _memBlob[_numBlobs] );

	_headOfFreeList = _memBlob[_numBlobs];
	MEMPOOL_ASSERT( _headOfFreeList );

	char* pBlock = (char*)_headOfFreeList;
	for ( int i = 1; i < numBlocks; i++ )
	{
		char* pNext = pBlock + _blockSize;

		*(void**)pBlock = pNext;
		pBlock = pNext;
	}

	*(void**)pBlock = NULL;
	_numBlobs++;
	_numElements += numBlocks;

	MEMPOOL_ASSERT( _numBlobs < MAX_BLOBS - 1 );
}

void* CMemoryPool::Alloc( unsigned int amount )
{
	void* pBlock;

	if ( amount > (unsigned int)_blockSize )
	{
		return NULL;
	}

	int allocated = _blocksAllocated + 1;

	if ( allocated >= _numElements )
		AddNewBlob();

	_blocksAllocated = allocated;

	if ( _blocksAllocated > _peakAlloc )
	{
		_peakAlloc = _blocksAllocated;
	}

	pBlock = _headOfFreeList;
	MEMPOOL_ASSERT( pBlock );
	_headOfFreeList = *(void**)pBlock;
	return pBlock;
}

void CMemoryPool::Free( void* memBlock )
{
	if ( !memBlock )
	{
		return;  // trying to delete NULL pointer, ignore
	}

	_blocksAllocated--;
	*(void**)memBlock = _headOfFreeList;
	_headOfFreeList = memBlock;
}
