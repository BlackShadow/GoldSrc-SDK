#include "VGuiKeyValues.h"

#include "MemPool.h"
#include "platform.h"
#include "tier1/interface.h"
#include "vgui/ILocalize.h"
#include "vgui_internal.h"

#include <string.h>

EXPOSE_SINGLE_INTERFACE( CVGuiKeyValues, IKeyValues, "KeyValues003" );

void CVGuiKeyValues::RegisterSizeofKeyValues( int size )
{
	if ( m_iMaxKeyValuesSize < size )
	{
		m_iMaxKeyValuesSize = size;
	}
}

void* CVGuiKeyValues::AllocKeyValuesMemory( int size )
{
	if ( !m_pMemPool )
	{
		m_pMemPool = new CMemoryPool( m_iMaxKeyValuesSize, 1024 );
	}

	return m_pMemPool->Alloc( size );
}

void CVGuiKeyValues::AddKeyValuesToMemoryLeakList( void*, HKeySymbol )
{
}

void CVGuiKeyValues::RemoveKeyValuesFromMemoryLeakList( void* )
{
}

const char* CVGuiKeyValues::GetStringForSymbol( HKeySymbol symbol )
{
	return &m_Strings[symbol];
}

void CVGuiKeyValues::GetANSIFromLocalized( const wchar_t* wchar, char* outBuf, int ansiBufferSizeInBytes )
{
	vgui2::g_pLocalize->ConvertUnicodeToANSI( wchar, outBuf, ansiBufferSizeInBytes );
}

CVGuiKeyValues::CVGuiKeyValues() :
	m_Strings( 1024 ),
	m_HashItemMemPool( sizeof( hash_item_t ), 64 ),
	m_HashTable( 0 ),
	m_KeyValuesTrackingList( 0, 0, MemoryLeakTrackerLessFunc )
{
	int i;

	m_HashTable.AddMultipleToTail( 2047 );

	for ( i = 0; i < m_HashTable.Count(); i++ )
	{
		m_HashTable[i].stringIndex = 0;
		m_HashTable[i].next = NULL;
	}

	m_Strings.AddToTail( 0 );

	m_pMemPool = NULL;
	m_iMaxKeyValuesSize = 4;
}

CVGuiKeyValues::~CVGuiKeyValues()
{
	delete m_pMemPool;
}

void CVGuiKeyValues::FreeKeyValuesMemory( void* pMem )
{
	m_pMemPool->Free( pMem );
}

HKeySymbol CVGuiKeyValues::GetSymbolForString( const char* name )
{
	int bucket;
	hash_item_t* item;
	int length;
	HKeySymbol symbol;

	bucket = CaseInsensitiveHash( name, m_HashTable.Count() );
	item = &m_HashTable[bucket];

	while ( 1 )
	{
		if ( strcasecmp( name, &m_Strings[item->stringIndex] ) == 0 )
		{
			return item->stringIndex;
		}

		if ( !item->next )
		{
			break;
		}

		item = item->next;
	}

	if ( item->stringIndex != 0 )
	{
		hash_item_t* chain;

		chain = (hash_item_t*)m_HashItemMemPool.Alloc( sizeof( hash_item_t ) );
		item->next = chain;
		item = chain;
	}

	item->next = NULL;
	length = (int)strlen( name ) + 1;
	symbol = (HKeySymbol)m_Strings.AddMultipleToTail( length );
	item->stringIndex = symbol;
	strcpy( &m_Strings[symbol], name );
	return symbol;
}

void CVGuiKeyValues::GetLocalizedFromANSI( const char* ansi, wchar_t* outBuf, int unicodeBufferSizeInBytes )
{
	wchar_t* localized;

	if ( *ansi == '#' )
	{
		localized = vgui2::g_pLocalize->Find( ansi );

		if ( localized )
		{
			vgui2::g_pLocalize->ConstructString( outBuf, unicodeBufferSizeInBytes, L"%s1", 1, localized );
			return;
		}
	}

	vgui2::g_pLocalize->ConvertANSIToUnicode( ansi, outBuf, unicodeBufferSizeInBytes );
}

int CVGuiKeyValues::CaseInsensitiveHash( const char* string, int iBounds )
{
	unsigned int hash;
	unsigned char character;

	if ( !*string )
	{
		return 0;
	}

	hash = 0;

	do
	{
		character = (unsigned char)*string;
		hash = ( hash << 1 ) + (signed char)character;

		if ( character >= 'A' && character <= 'Z' )
		{
			hash += 'a' - 'A';
		}

		++string;
	}
	while ( *string );

	return (int)( hash % (unsigned int)iBounds );
}

bool CVGuiKeyValues::MemoryLeakTrackerLessFunc( const MemoryLeakTracker_t& lhs, const MemoryLeakTracker_t& rhs )
{
	return lhs.pMem < rhs.pMem;
}
