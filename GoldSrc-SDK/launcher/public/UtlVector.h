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
// A growable array class that maintains a free list and keeps elements
// in the same location
//=============================================================================

#ifndef UTLVECTOR_H
#define UTLVECTOR_H

#include "UtlMemory.h"

template <class T>
class CUtlVector
{
public:
	CUtlVector()
		: m_Size( 0 )
		, m_pElements( NULL )
	{
	}

	T* Add( const T& value )
	{
		const int index = AddToTail();
		T* slot = &m_Memory[index];
		*slot = value;
		return slot;
	}

	int AddToTail()
	{
		m_Memory.EnsureCapacity( m_Size + 1 );
		return m_Size++;
	}

	int Count() const
	{
		return m_Size;
	}

	T& operator[]( int index )
	{
		return m_Memory[index];
	}

	const T& operator[]( int index ) const
	{
		return m_Memory[index];
	}

	void RemoveAll()
	{
		m_Size = 0;
	}

	void Purge()
	{
		RemoveAll();
		m_Memory.Purge();
	}

	CUtlMemory<T>	m_Memory;
	int				m_Size;
	T*				m_pElements;
};

#endif
