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

#include "public/utlsymbol.h"

#include <string.h>

#ifndef _WIN32
#include <strings.h>
#endif

#include "../public/buildslave_assert.h"
#define ASSERT_FILE	BUILDSLAVE_PATH( L"public\\utlsymbol.cpp" )

#define INVALID_STRING_INDEX ( (unsigned int)-1 )

//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------
CUtlSymbolTable* CUtlSymbol::s_pSymbolTable = 0;

//-----------------------------------------------------------------------------
// symbol methods
//-----------------------------------------------------------------------------
void CUtlSymbol::Initialize()
{
	// necessary to allow us to create global symbols
	static bool symbolsInitialized = false;
	if ( !symbolsInitialized )
	{
		s_pSymbolTable = new CUtlSymbolTable;
		symbolsInitialized = true;
	}
}

CUtlSymbolTable* CUtlSymbol::CurrTable()
{
	Initialize();
	return s_pSymbolTable;
}

//-----------------------------------------------------------------------------
// string->symbol->string
//-----------------------------------------------------------------------------

CUtlSymbol::CUtlSymbol( const char* pStr )
{
	m_Id = CurrTable()->AddString( pStr );
}

const char* CUtlSymbol::String() const
{
	return CurrTable()->String( m_Id );
}

//-----------------------------------------------------------------------------
// checks if the symbol matches a string
//-----------------------------------------------------------------------------
bool CUtlSymbol::operator==( const char* pStr ) const
{
	if ( m_Id == UTL_INVAL_SYMBOL )
		return false;
	return strcmp( String(), pStr ) == 0;
}

//-----------------------------------------------------------------------------
// symbol table stuff
//-----------------------------------------------------------------------------
struct LessCtx_t
{
	const char*			m_pUserString;
	CUtlSymbolTable*	m_pTable;
};

static LessCtx_t g_LessCtx;

bool CUtlSymbolTable::SymLess( const unsigned int& i1, const unsigned int& i2 )
{
	const char* str1 = ( i1 == INVALID_STRING_INDEX ) ? g_LessCtx.m_pUserString :
		&g_LessCtx.m_pTable->m_Strings[i1];
	const char* str2 = ( i2 == INVALID_STRING_INDEX ) ? g_LessCtx.m_pUserString :
		&g_LessCtx.m_pTable->m_Strings[i2];

	return strcmp( str1, str2 ) < 0;
}

bool CUtlSymbolTable::SymLessi( const unsigned int& i1, const unsigned int& i2 )
{
	const char* str1 = ( i1 == INVALID_STRING_INDEX ) ? g_LessCtx.m_pUserString :
		&g_LessCtx.m_pTable->m_Strings[i1];
	const char* str2 = ( i2 == INVALID_STRING_INDEX ) ? g_LessCtx.m_pUserString :
		&g_LessCtx.m_pTable->m_Strings[i2];

#ifdef _WIN32
	return _stricmp( str1, str2 ) < 0;
#else
	return strcasecmp( str1, str2 ) < 0;
#endif
}

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CUtlSymbolTable::CUtlSymbolTable( int growSize, int initSize, bool caseInsensitive ) :
	m_Lookup( growSize, initSize, caseInsensitive ? SymLessi : SymLess ), m_Strings( 256 )
{
}

CUtlSymbolTable::~CUtlSymbolTable()
{
}

CUtlSymbol CUtlSymbolTable::Find( const char* pString )
{
	if ( !pString )
		return CUtlSymbol();

	// Store a special context used to help with insertion
	g_LessCtx.m_pUserString = pString;
	g_LessCtx.m_pTable = this;

	// Passing this special invalid symbol makes the comparison function
	// use the string passed in the context
	UtlSymId_t idx = m_Lookup.Find( INVALID_STRING_INDEX );
	return CUtlSymbol( idx );
}

//-----------------------------------------------------------------------------
// Finds and/or creates a symbol based on the string
//-----------------------------------------------------------------------------
CUtlSymbol CUtlSymbolTable::AddString( const char* pString )
{
	if ( !pString )
		return CUtlSymbol( UTL_INVAL_SYMBOL );

	CUtlSymbol id = Find( pString );

	if ( id.IsValid() )
		return id;

	// didn't find, insert the string into the vector.
	int len = (int)strlen( pString ) + 1;
	int stridx = m_Strings.AddMultipleToTail( len );
	memcpy( &m_Strings[stridx], pString, len * sizeof( char ) );
	UtlSymId_t idx = m_Lookup.Insert( stridx );
	return CUtlSymbol( idx );
}

//-----------------------------------------------------------------------------
// Look up the string associated with a particular symbol
//-----------------------------------------------------------------------------
const char* CUtlSymbolTable::String( CUtlSymbol id ) const
{
	if ( !id.IsValid() )
		return "";

	ASSERT_AT( m_Lookup.IsValidIndex((UtlSymId_t)id), 178 );
	return &m_Strings[m_Lookup[id]];
}

//-----------------------------------------------------------------------------
// Remove all symbols in the table.
//-----------------------------------------------------------------------------
void CUtlSymbolTable::RemoveAll()
{
	m_Lookup.RemoveAll();
	m_Strings.RemoveAll();
}
