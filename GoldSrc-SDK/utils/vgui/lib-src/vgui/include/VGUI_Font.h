#ifndef VGUI_FONT_H
#define VGUI_FONT_H

#include "VGUI.h"

#define VGUI_FIRST_FONT_ID	100

namespace vgui
{

class BaseFontPlat;

class VGUIAPI Font
{
public:
	Font( const char* name, int tall, int wide, float rotation, int weight,
		bool italic, bool underline, bool strikeout, bool symbol );
	Font( const char* name, void* pFileData, int fileDataLen, int tall, int wide, float rotation, int weight,
		bool italic, bool underline, bool strikeout, bool symbol );

#if defined( _WIN32 )
private:
#endif
	virtual void init( const char* name, void* pFileData, int fileDataLen, int tall, int wide,
		float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol );

public:
	BaseFontPlat* getPlat();
	virtual void getCharRGBA( int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, uchar* rgba );
	virtual void getCharABCwide( int ch, int& a, int& b, int& c );
	virtual void getTextSize( const char* text, int& wide, int& tall );
	virtual int getTall();
#if !defined( _WIN32 )
	virtual int getWide();
#endif
	virtual int getId();

private:
	char*			_name;
	BaseFontPlat*	_plat;
	int				_id;

public:
	Font& operator=( const Font& ) = default;
};

}

#endif
