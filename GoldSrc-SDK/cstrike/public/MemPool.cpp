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

#include <climits>
#include <cmath>
#include <cstdlib>
#include <new>

CMemoryPool::CMemoryPool( int blockSize, int blocksPerBlob )
{
	_blockSize = blockSize;
	_numBlobs = 0;
	_numElements = 0;
	_blocksPerBlob = blocksPerBlob;
	AddNewBlob();
	_peakAlloc = 0;
	_blocksAllocated = 0;
}

CMemoryPool::~CMemoryPool()
{
	for ( int i = 0; i < _numBlobs; i++ )
		free( _memBlob[i] );
}

void CMemoryPool::AddNewBlob()
{
	if ( _numBlobs < 0 || _numBlobs >= MAX_BLOBS || _blocksPerBlob <= 0 || _blockSize < (int)sizeof( void* ) )
		throw std::bad_alloc();

	int sizeMultiplier = (int)pow( 2.0, (double)_numBlobs );
	if ( _blocksPerBlob > INT_MAX / sizeMultiplier )
		throw std::bad_alloc();
	int nElements = _blocksPerBlob * sizeMultiplier;
	if ( _blockSize > INT_MAX / nElements || _numElements > INT_MAX - nElements )
		throw std::bad_alloc();
	int blobSize = _blockSize * nElements;

	void* memory = malloc( blobSize );
	if ( !memory )
		throw std::bad_alloc();

	void** newBlob = (void**)memory;
	for ( int j = 0; j < nElements - 1; j++ )
	{
		newBlob[0] = (char*)newBlob + _blockSize;
		newBlob = (void**)newBlob[0];
	}

	// null terminate list
	newBlob[0] = NULL;

	_memBlob[_numBlobs] = memory;
	_headOfFreeList = memory;
	_numBlobs++;
	_numElements += nElements;
}

void* CMemoryPool::Alloc( unsigned int amount )
{
	void* returnBlock;

	if ( amount > (unsigned int)_blockSize )
		return NULL;

	int allocated = _blocksAllocated + 1;
	if ( allocated >= _numElements )
		AddNewBlob();

	_blocksAllocated = allocated;
	if ( _peakAlloc <= _blocksAllocated )
		_peakAlloc = _blocksAllocated;

	returnBlock = _headOfFreeList;
	_headOfFreeList = *(void**)_headOfFreeList;

	return returnBlock;
}

void CMemoryPool::Free( void* memblock )
{
	if ( !memblock )
		return;

	_blocksAllocated--;
	*(void**)memblock = _headOfFreeList;
	_headOfFreeList = memblock;
}
