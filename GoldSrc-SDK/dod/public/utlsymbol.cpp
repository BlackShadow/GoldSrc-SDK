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

#include "platform.h"

#include <string.h>

#define INVALID_STRING_INDEX	((unsigned int)0xFFFFFFFF)

//-----------------------------------------------------------------------------
// symbol table stuff
//-----------------------------------------------------------------------------
struct LessCtx_t
{
	const char*			m_pUserString;
	CUtlSymbolTable*	m_pTable;

	LessCtx_t() : m_pUserString( NULL ), m_pTable( NULL )
	{
	}
};

static LessCtx_t g_LessCtx;

CUtlSymbolTable* CUtlSymbol::s_pSymbolTable = NULL;


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

const char* CUtlSymbol::String() const
{
	return CurrTable()->String( m_Id );
}

bool CUtlSymbolTable::SymLess( const unsigned int& i1, const unsigned int& i2 )
{
	const char* str1;
	const char* str2;

	if ( i1 == INVALID_STRING_INDEX )
	{
		str1 = g_LessCtx.m_pUserString;
	}
	else
	{
		str1 = &g_LessCtx.m_pTable->m_Strings[i1];
	}

	if ( i2 == INVALID_STRING_INDEX )
	{
		str2 = g_LessCtx.m_pUserString;
	}
	else
	{
		str2 = &g_LessCtx.m_pTable->m_Strings[i2];
	}

	return strcmp( str1, str2 ) < 0;
}

bool CUtlSymbolTable::SymLessi( const unsigned int& i1, const unsigned int& i2 )
{
	const char* str1;
	const char* str2;

	if ( i1 == INVALID_STRING_INDEX )
	{
		str1 = g_LessCtx.m_pUserString;
	}
	else
	{
		str1 = &g_LessCtx.m_pTable->m_Strings[i1];
	}

	if ( i2 == INVALID_STRING_INDEX )
	{
		str2 = g_LessCtx.m_pUserString;
	}
	else
	{
		str2 = &g_LessCtx.m_pTable->m_Strings[i2];
	}

	return strcasecmp( str1, str2 ) < 0;
}

CUtlSymbol::CUtlSymbol( const char* pStr )
{
	m_Id = CurrTable()->AddString( pStr );
}

bool CUtlSymbol::operator==( const char* pStr ) const
{
	if ( !IsValid() )
	{
		return false;
	}

	return strcmp( String(), pStr ) == 0;
}

CUtlSymbolTable::CUtlSymbolTable( int growSize, int initSize, bool caseInsensitive )
	: m_Lookup( growSize, initSize, caseInsensitive ? &CUtlSymbolTable::SymLessi : &CUtlSymbolTable::SymLess ),
	  m_Strings( 256 )
{
}

CUtlSymbolTable::~CUtlSymbolTable()
{
}

CUtlSymbol CUtlSymbolTable::Find( const char* pString )
{
	if ( !pString )
	{
		return CUtlSymbol();
	}

	// Store a special context used to help with insertion
	g_LessCtx.m_pUserString = pString;
	g_LessCtx.m_pTable = this;
	return CUtlSymbol( m_Lookup.Find( INVALID_STRING_INDEX ) );
}

CUtlSymbol CUtlSymbolTable::AddString( const char* pString )
{
	if ( !pString )
	{
		return CUtlSymbol();
	}

	CUtlSymbol symbol = Find( pString );

	if ( symbol.IsValid() )
	{
		return symbol;
	}

	int length = (int)strlen( pString ) + 1;
	unsigned int stringIndex = (unsigned int)m_Strings.AddMultipleToTail( length );
	memcpy( &m_Strings[stringIndex], pString, length );
	return CUtlSymbol( m_Lookup.Insert( stringIndex ) );
}


//-----------------------------------------------------------------------------
// Remove all symbols in the table.
//-----------------------------------------------------------------------------

void CUtlSymbolTable::RemoveAll()
{
	m_Lookup.RemoveAll();
	m_Strings.RemoveAll();
}
