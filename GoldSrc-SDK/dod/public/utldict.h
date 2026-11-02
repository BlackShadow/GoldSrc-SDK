//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: A dictionary mapping from symbol to structure 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#pragma once

#include "utlrbtree.h"
#include "utlsymbol.h"

template <class T, class I = unsigned short>
class CUtlDict
{
public:
	CUtlDict( int growSize = 0, int initSize = 0, bool caseInsensitive = true )
		: m_Elements( growSize, initSize, DictLessFunc ),
		  m_SymbolTable( growSize, initSize, caseInsensitive )
	{
	}

	I Find( const char* name ) const
	{
		DictElement_t element;
		element.m_Name = m_SymbolTable.AddString( name );
		return m_Elements.Find( element );
	}

	I Insert( const char* name )
	{
		DictElement_t element;
		element.m_Name = m_SymbolTable.AddString( name );
		return m_Elements.Insert( element );
	}

	T& Element( I index )
	{
		return m_Elements.Element( index ).m_Data;
	}

	static I InvalidIndex()
	{
		return (I)-1;
	}

protected:
	struct DictElement_t
	{
		CUtlSymbol	m_Name;
		T			m_Data;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
	};

	static bool DictLessFunc( const DictElement_t& left, const DictElement_t& right )
	{
		return (UtlSymId_t)left.m_Name < (UtlSymId_t)right.m_Name;
	}

	CUtlRBTree<DictElement_t, I>	m_Elements;
	mutable CUtlSymbolTable		m_SymbolTable;
};
