#ifndef INFOSTRING_H
#define INFOSTRING_H

class InfoString
{
public:
	InfoString();
	InfoString( unsigned int maxSize );
	InfoString( char* string );
	InfoString( char* string, unsigned int maxSize );
	virtual ~InfoString();

	bool SetString( char* string );
	void SetMaxSize( unsigned int maxSize );
	int GetMaxSize();
	int GetCurrentSize();
	void Clear();
	char* GetString();

	char* ValueForKey( const char* key );
	bool RemoveKey( const char* key );
	void RemovePrefixedKeys( char prefix );
	bool SetValueForStarKey( const char* key, const char* value );
	bool SetValueForKey( const char* key, const char* value );

protected:
	int		m_MaxSize;
	char*	m_String;
};

#endif
