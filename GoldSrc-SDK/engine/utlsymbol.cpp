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

#include "utlsymbol.h"
#include "common.h"

#include <cassert>
#include <cstring>

#define INVALID_STRING_INDEX ( (unsigned int)-1 )

CUtlSymbolTable* CUtlSymbol::s_pSymbolTable = NULL;

void CUtlSymbol::Initialize()
{
	// necessary to allow us to create global symbols
	static bool symbolsInitialized = false;
	if ( !symbolsInitialized )
	{
		s_pSymbolTable = new CUtlSymbolTable( 0, 32, false );
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
	return CurrTable()->String( *this );
}

//-----------------------------------------------------------------------------
// checks if the symbol matches a string
//-----------------------------------------------------------------------------

bool CUtlSymbol::operator==( const char* pStr ) const
{
	if ( m_Id == UTL_INVAL_SYMBOL )
		return false;

	Initialize();
	return strcmp( s_pSymbolTable->m_Strings.Base() + s_pSymbolTable->m_Lookup.Element( m_Id ), pStr ) == 0;
}

const char* CUtlSymbolTable::String( CUtlSymbol id ) const
{
	if ( (UtlSymId_t)id == UTL_INVAL_SYMBOL )
		return "";

#if defined( _WIN32 ) && !defined( NDEBUG )
	if ( !m_Lookup.IsValidIndex( (UtlSymId_t)id ) )
		_wassert( L"m_Lookup.IsValidIndex((UtlSymId_t)id)",
			L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\public\\utlsymbol.cpp", 178 );
#endif

	return &m_Strings[m_Lookup[(UtlSymId_t)id]];
}

//-----------------------------------------------------------------------------
// symbol table stuff
//-----------------------------------------------------------------------------
struct LessCtx_t
{
	const char* m_pUserString;
	CUtlSymbolTable* m_pTable;
};

static LessCtx_t g_LessCtx;

bool CUtlSymbolTable::SymLess( const unsigned int& i1, const unsigned int& i2 )
{
	const char* str1 = ( i1 == INVALID_STRING_INDEX ) ? g_LessCtx.m_pUserString : &g_LessCtx.m_pTable->m_Strings[i1];
	const char* str2 = ( i2 == INVALID_STRING_INDEX ) ? g_LessCtx.m_pUserString : &g_LessCtx.m_pTable->m_Strings[i2];

	return strcmp( str1, str2 ) < 0;
}

bool CUtlSymbolTable::SymLessi( const unsigned int& i1, const unsigned int& i2 )
{
	const char* str1 = ( i1 == INVALID_STRING_INDEX ) ? g_LessCtx.m_pUserString : &g_LessCtx.m_pTable->m_Strings[i1];
	const char* str2 = ( i2 == INVALID_STRING_INDEX ) ? g_LessCtx.m_pUserString : &g_LessCtx.m_pTable->m_Strings[i2];

	return _stricmp( str1, str2 ) < 0;
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
	return CUtlSymbol( m_Lookup.Find( INVALID_STRING_INDEX ) );
}

//-----------------------------------------------------------------------------
// Finds and/or creates a symbol based on the string
//-----------------------------------------------------------------------------

CUtlSymbol CUtlSymbolTable::AddString( const char* pString )
{
	if ( !pString )
		return CUtlSymbol();

	CUtlSymbol id = Find( pString );
	if ( id.IsValid() )
		return id;

	int len = (int)strlen( pString ) + 1;
	int stridx = m_Strings.AddMultipleToTail( len );
	memcpy( &m_Strings[stridx], pString, len );

	return CUtlSymbol( m_Lookup.Insert( stridx ) );
}

CUtlSymbol::CUtlSymbol() : m_Id( UTL_INVAL_SYMBOL )
{
}

CUtlSymbol::CUtlSymbol( UtlSymId_t id ) : m_Id( id )
{
}

CUtlSymbol::CUtlSymbol( const CUtlSymbol& other ) : m_Id( other.m_Id )
{
}

bool CUtlSymbol::IsValid() const
{
	return m_Id != UTL_INVAL_SYMBOL;
}

CUtlSymbol::operator const UtlSymId_t() const
{
	return m_Id;
}

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CUtlSymbolTable::CUtlSymbolTable( int growSize, int initialSize, bool caseInsensitive )
	: m_Lookup( growSize, initialSize, caseInsensitive ? SymLessi : SymLess ), m_Strings( 256, 0 )
{
}

CUtlSymbolTable::~CUtlSymbolTable()
{
}

//-----------------------------------------------------------------------------
// Remove all symbols in the table.
//-----------------------------------------------------------------------------

void CUtlSymbolTable::RemoveAll()
{
	m_Lookup.RemoveAll();
	m_Strings.RemoveAll();
}
