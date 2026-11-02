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

#pragma once

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

#define UTL_INVAL_SYMBOL	( (UtlSymId_t)~0 )

class CUtlSymbol
{
public:
	// constructor, destructor
	CUtlSymbol() : m_Id( UTL_INVAL_SYMBOL ) {}
	CUtlSymbol( UtlSymId_t id ) : m_Id( id ) {}
	CUtlSymbol( const char* pStr );
	CUtlSymbol( const CUtlSymbol& sym ) : m_Id( sym.m_Id ) {}

	// operator=
	CUtlSymbol& operator=( const CUtlSymbol& src ) { m_Id = src.m_Id; return *this; }

	// operator==
	bool operator==( const CUtlSymbol& src ) const { return m_Id == src.m_Id; }
	bool operator==( const char* pStr ) const;

	// Is valid?
	bool IsValid() const { return m_Id != UTL_INVAL_SYMBOL; }

	// Gets at the symbol
	operator const UtlSymId_t() const { return m_Id; }

	// Gets the string associated with the symbol
	const char* String() const;

protected:
	UtlSymId_t		m_Id;

	// Initializes the symbol table
	static void Initialize();

	// returns the current symbol table
	static CUtlSymbolTable* CurrTable();

	// The standard global symbol table
	static CUtlSymbolTable* s_pSymbolTable;
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

	// Look up the string associated with a particular symbol
	const char* String( CUtlSymbol id ) const;

	// Remove all symbols in the table.
	void RemoveAll();

protected:
	// Stores the symbol lookup
	CUtlRBTree<unsigned int, unsigned short>	m_Lookup;

	// stores the string data
	CUtlVector<char>							m_Strings;

	// Less function, for sorting strings
	static bool SymLess( const unsigned int& i1, const unsigned int& i2 );
	// case insensitive less function
	static bool SymLessi( const unsigned int& i1, const unsigned int& i2 );
};
