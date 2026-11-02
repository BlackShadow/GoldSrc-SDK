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
#include <new>

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
	if ( _numBlobs >= (int)( sizeof( _memBlob ) / sizeof( _memBlob[0] ) ) )
		throw std::bad_alloc();
	int sizeMultiplier = (int)pow( 2.0, (double)_numBlobs );
	int numBlocks = _blocksPerBlob * sizeMultiplier;
	void* pBlob = malloc( _blockSize * numBlocks );
	if ( !pBlob )
		throw std::bad_alloc();
	char* pBlock = (char*)pBlob;

	_memBlob[_numBlobs] = pBlob;
	_headOfFreeList = pBlob;

	for ( int i = 1; i < numBlocks; i++ )
	{
		char* pNext = pBlock + _blockSize;

		*(void**)pBlock = pNext;
		pBlock = pNext;
	}

	*(void**)pBlock = NULL;
	_numBlobs++;
	_numElements += numBlocks;
}

void* CMemoryPool::Alloc( unsigned int amount )
{
	void* pBlock;

	if ( amount > (unsigned int)_blockSize )
	{
		//Assert( !"CMemoryPool::Alloc: tried to make new blob with GROW_NONE" );
		return NULL;
	}

	if ( _blocksAllocated + 1 >= _numElements )
	{
		AddNewBlob();
	}
	_blocksAllocated++;
	if ( _blocksAllocated > _peakAlloc )
		_peakAlloc = _blocksAllocated;

	pBlock = _headOfFreeList;
	_headOfFreeList = *(void**)pBlock;
	return pBlock;
}

void CMemoryPool::Free( void* memBlock )
{
	if ( !memBlock )
	{
		return;
	}

	_blocksAllocated--;
	*(void**)memBlock = _headOfFreeList;
	_headOfFreeList = memBlock;
}
