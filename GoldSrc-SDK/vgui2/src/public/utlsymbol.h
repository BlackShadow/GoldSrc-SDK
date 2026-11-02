//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Defines a symbol table
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#ifndef UTLSYMBOL_H
#define UTLSYMBOL_H

#ifdef _WIN32
#pragma once
#endif

#include "utlrbtree.h"
#include "utlvector.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class CUtlSymbolTable;

//-----------------------------------------------------------------------------
// This is a symbol, which is a easier way of dealing with strings.
//-----------------------------------------------------------------------------

typedef unsigned short UtlSymId_t;

#define UTL_INVAL_SYMBOL	((UtlSymId_t)~0)

class CUtlSymbol
{
public:
	// constructor, destructor
	CUtlSymbol() : m_Id( UTL_INVAL_SYMBOL )
	{
	}

	CUtlSymbol( UtlSymId_t id ) : m_Id( id )
	{
	}

	CUtlSymbol( const char* pStr );

	CUtlSymbol( CUtlSymbol const& sym ) : m_Id( sym.m_Id )
	{
	}

	// operator=
	CUtlSymbol& operator=( CUtlSymbol const& src )
	{
		m_Id = src.m_Id;
		return *this;
	}

	// operator==
	bool operator==( CUtlSymbol const& src ) const
	{
		return m_Id == src.m_Id;
	}

	bool operator==( const char* pStr ) const;

	// Is valid?
	bool IsValid() const
	{
		return m_Id != UTL_INVAL_SYMBOL;
	}

	// Gets at the symbol
	operator UtlSymId_t() const
	{
		return m_Id;
	}

	const char* String() const;

protected:
	UtlSymId_t	m_Id;

	// Initializes the symbol table
	static void Initialize();
	// returns the current symbol table
	static CUtlSymbolTable* CurrTable();

	static CUtlSymbolTable*	s_pSymbolTable;
};

//-----------------------------------------------------------------------------
// CUtlSymbolTable:
// description:
//    This class defines a symbol table, which allows us to perform mappings
//    of strings to symbols and back. The symbol class itself contains
//    a static version of this class for creating global strings, but this
//    class can also be instanced to create local symbol tables.
//-----------------------------------------------------------------------------

class CUtlSymbolTable
{
public:
	// constructor, destructor
	CUtlSymbolTable( int growSize = 0, int initSize = 32, bool caseInsensitive = false );
	~CUtlSymbolTable();

	// Finds and/or creates a symbol based on the string
	CUtlSymbol AddString( const char* pString );
	// Finds the symbol for pString
	CUtlSymbol Find( const char* pString );

	const char* String( CUtlSymbol id ) const;

	void RemoveAll();

protected:
	// Less function, for sorting strings
	static bool SymLess( const unsigned int& i1, const unsigned int& i2 );
	// case insensitive less function
	static bool SymLessi( const unsigned int& i1, const unsigned int& i2 );

	// Stores the symbol lookup
	CUtlRBTree<unsigned int, unsigned short>	m_Lookup;
	CUtlVector<char>							m_Strings;
};

#endif // UTLSYMBOL_H
