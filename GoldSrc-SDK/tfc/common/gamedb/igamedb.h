#ifndef IGAMEDB_H
#define IGAMEDB_H

#include "gamedb_common.h"
#include "interface_registry.h"

namespace gamedb
{
	class Table;
	class TableRow;
	class Field;
}

typedef gamedb::Table* HTABLE;
typedef gamedb::TableRow* HROW;
typedef gamedb::Field* HFIELD;

class AddTableColumnDesc
{
public:
	const char*	m_pName;
	ColumnType	m_Type;
	HCOLUMN*	m_pColumnHandle;
};

class IGameDB : public IBaseInterface
{
public:
	virtual ~IGameDB() {}
	virtual void Release() = 0;
	virtual void Term() = 0;
	virtual bool LoadDB( const char* pFilename ) = 0;
	virtual bool SaveDB( const char* pFilename ) = 0;
	virtual HTABLE AddTable( const char* pName, int nColumns, AddTableColumnDesc* pColumns ) = 0;
	virtual HTABLE AddTable_VA( const char* pName, int nColumns, ... ) = 0;
	virtual HTABLE GetTable_VA( const char* pName, int nColumns, ... ) = 0;
	virtual HROW FindRow( HTABLE hTable, int nFields, bool bCreate, ... ) = 0;
	virtual HFIELD GetField( HROW hRow, HCOLUMN hColumn ) = 0;
	virtual bool GetFieldValueInt( HFIELD hField, int& value ) = 0;
	virtual bool SetFieldValueInt( HFIELD hField, int value ) = 0;
	virtual bool GetFieldValueString( HFIELD hField, char* pValue, unsigned int maxLen ) = 0;
	virtual bool SetFieldValueString( HFIELD hField, const char* pValue ) = 0;
};

#endif
