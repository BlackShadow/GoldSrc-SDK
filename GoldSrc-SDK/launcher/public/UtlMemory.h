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
// A growable memory class.
//=============================================================================

#ifndef UTLMEMORY_H
#define UTLMEMORY_H

#include <cstdlib>

template <class T>
class CUtlMemory
{
public:
	CUtlMemory()
		: m_pMemory( NULL )
		, m_NumAllocated( 0 )
		, m_GrowSize( 0 )
	{
	}

	void EnsureCapacity( int required )
	{
		if ( required <= m_NumAllocated )
		{
			return;
		}

		const int deficit = required - m_NumAllocated;
		int allocated = m_NumAllocated;

		if ( deficit >= m_GrowSize )
		{
			if ( m_GrowSize < 0 )
			{
				return;
			}

			allocated += deficit;
		}
		else if ( m_GrowSize == 0 )
		{
			allocated += allocated == 0 ? 1 : allocated;
		}
		else
		{
			allocated += m_GrowSize;
		}

		const size_t bytes = (size_t)allocated * sizeof( T );
		T* memory;

		if ( m_pMemory != NULL )
		{
			memory = (T*)realloc( m_pMemory, bytes );
		}
		else
		{
			memory = (T*)malloc( bytes );
		}

		m_pMemory = memory;
		m_NumAllocated = allocated;
	}

	// element access
	T& operator[]( int index )
	{
		return m_pMemory[index];
	}

	const T& operator[]( int index ) const
	{
		return m_pMemory[index];
	}

	void Purge()
	{
		if ( m_GrowSize < 0 )
		{
			return;
		}

		if ( m_pMemory != NULL )
		{
			free( m_pMemory );
			m_pMemory = NULL;
		}

		m_NumAllocated = 0;
	}

	T*	m_pMemory;
	int	m_NumAllocated;
	int	m_GrowSize;
};

#endif
