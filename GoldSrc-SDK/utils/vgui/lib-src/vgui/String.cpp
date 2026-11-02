#include "VGUI_String.h"

#include <stdio.h>
#include <string.h>

namespace vgui
{

String::String()
{
	_text = (char*)"";
}

String::String( const char* text )
{
	int count;

	count = getCount( text );
	_text = new char[count + 1];
	memcpy( _text, text, count );
	_text[count] = 0;
}

String::String( const String& src )
{
	_text = src._text;
}

String::~String()
{
	_text = NULL;
}

int String::getCount( const char* text )
{
	int count;

	for ( count = 0; text[count] != 0; ++count )
	{
	}

	return count;
}

int String::getCount()
{
	return getCount( _text );
}

String String::operator+( String text )
{
	int count1;
	int count2;
	String* newString;

	count1 = getCount();
	count2 = getCount( text._text );

	newString = new String();
	newString->_text = new char[count1 + count2 + 1];
	memcpy( newString->_text, _text, count1 );
	memcpy( newString->_text + count1, text._text, count2 );
	newString->_text[count1 + count2] = 0;
	return *newString;
}

String String::operator+( const char* text )
{
	int count1;
	int count2;
	String* newString;

	count1 = getCount();
	count2 = getCount( text );

	newString = new String();
	newString->_text = new char[count1 + count2 + 1];
	memcpy( newString->_text, _text, count1 );
	memcpy( newString->_text + count1, text, count2 );
	newString->_text[count1 + count2] = 0;
	return *newString;
}

bool String::operator==( String text )
{
	const char* a;
	const char* b;

	a = _text;
	b = text._text;
	for ( ;; )
	{
		if ( *a != *b )
		{
			return false;
		}
		if ( *a == 0 )
		{
			break;
		}
		a++;
		b++;
	}

	return true;
}

bool String::operator==( const char* text )
{
	const char* a;

	a = _text;
	for ( ;; )
	{
		if ( *a != *text )
		{
			return false;
		}
		if ( *a == 0 )
		{
			break;
		}
		a++;
		text++;
	}

	return true;
}

char String::operator[]( int index )
{
	return _text[index];
}

const char* String::getChars()
{
	return _text;
}

void String::test()
{
	printf( "{\n" );

	String aa( "aa" );
	printf( "aa [%s]\n", aa._text );

	String bb( "bb" );
	printf( "bb [%s]\n", bb._text );

	String cc( "cc" );
	printf( "cc [%s]\n", cc._text );

	String dd( "dd" );
	printf( "dd [%s]\n", dd._text );

	String aacc = aa + cc;
	printf( "aacc [%s]\n", aacc._text );

	String ee( "aa" );
	printf( "}\n" );
}

}
