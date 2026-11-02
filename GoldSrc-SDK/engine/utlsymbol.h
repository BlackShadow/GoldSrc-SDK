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
// This is a symbol, which is a easier way of dealing with strings.
//-----------------------------------------------------------------------------

typedef unsigned short UtlSymId_t;

#define UTL_INVAL_SYMBOL ( (UtlSymId_t)~0 )

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class CUtlSymbolTable;

class CUtlSymbol
{
public:
	CUtlSymbol();
	CUtlSymbol( UtlSymId_t id );
	CUtlSymbol( const char* text );
	CUtlSymbol( const CUtlSymbol& other );

	CUtlSymbol& operator=( const CUtlSymbol& other ) = default;

	// operator==
	bool operator==( const CUtlSymbol& other ) const
	{
		return m_Id == other.m_Id;
	}

	bool operator==( const char* text ) const;

	bool IsValid() const;
	operator const UtlSymId_t() const;
	const char* String() const;

	static void Initialize();
	// returns the current symbol table
	static CUtlSymbolTable* CurrTable();

private:
	UtlSymId_t m_Id;

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
	friend class CUtlSymbol;

public:
	CUtlSymbolTable( int growSize, int initialSize, bool caseInsensitive );
	~CUtlSymbolTable();

	CUtlSymbol AddString( const char* text );
	CUtlSymbol Find( const char* text );
	const char* String( CUtlSymbol symbol ) const;
	void RemoveAll();

protected:
	static bool SymLess( const unsigned int& left, const unsigned int& right );
	static bool SymLessi( const unsigned int& left, const unsigned int& right );

	// Stores the symbol lookup
	CUtlRBTree<unsigned int, unsigned short>	m_Lookup;
	CUtlVector<char>							m_Strings;
};
