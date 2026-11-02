#pragma once

#include <cstddef>

class SurfacePlat;

class FileImageStream
{
public:
	virtual void Read( void* pData, int len ) = 0;
	virtual bool ErrorStatus() = 0;
};

class FileImageStream_Memory : public FileImageStream
{
public:
	FileImageStream_Memory( void* pData, int dataLen );

	void Read( void* pData, int len ) override;
	bool ErrorStatus() override;

private:
	unsigned char*	m_pData;
	int				m_DataLen;
	int				m_CurPos;
	bool			m_bError;
};

class FileImage
{
public:
	void Term()
	{
		if ( m_pData )
			delete[] m_pData;

		m_Width = 0;
		m_Height = 0;
		m_pData = NULL;
	}

	int				m_Width;
	int				m_Height;
	unsigned char*	m_pData;
};

class VFontData
{
public:
	VFontData();
	~VFontData();

	int				m_CharWidths[256];
	int				m_BitmapCharWidth;
	int				m_BitmapCharHeight;
	unsigned char*	m_pBitmap;
};

class BaseFontPlat
{
public:
	virtual ~BaseFontPlat() = default;

	virtual bool equals( const char* name, int tall, int wide, float rotation, int weight, bool italic, bool underline,
		bool strikeout, bool symbol ) = 0;
	virtual void getCharRGBA( int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba ) = 0;
	virtual void GetCharABCwide( int ch, int& a, int& b, int& c ) = 0;
	virtual int GetTall() = 0;
	virtual void DrawSetTextFont( SurfacePlat* plat ) = 0;
	virtual int getId() = 0;
};

class FontPlat : public BaseFontPlat
{
public:
	FontPlat( const char* name, int tall, int wide, float rotation, int weight, bool italic, bool underline,
		bool strikeout, bool symbol, int id );
	~FontPlat() override;

	bool equals( const char* name, int tall, int wide, float rotation, int weight, bool italic, bool underline,
		bool strikeout, bool symbol ) override;
	void getCharRGBA( int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba ) override;
	void GetCharABCwide( int ch, int& a, int& b, int& c ) override;
	int GetTall() override;
	void DrawSetTextFont( SurfacePlat* plat ) override;
	int getId() override;

private:
	VFontData	m_BitmapFont;
	bool		m_bBitmapFont;

	char*		_name;
	int			_wide;
	int			_tall;
	float		_rotation;
	int			_weight;
	bool		_italic;
	bool		_underline;
	bool		_strikeout;
	bool		_symbol;
	int			_id;
};

class FontPlat_Bitmap : public BaseFontPlat
{
public:
	FontPlat_Bitmap();
	~FontPlat_Bitmap() override;

	static FontPlat_Bitmap* Create( const char* name, FileImageStream* pStream, int id );

	bool equals( const char* name, int tall, int wide, float rotation, int weight, bool italic, bool underline,
		bool strikeout, bool symbol ) override;
	void getCharRGBA( int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba ) override;
	void GetCharABCwide( int ch, int& a, int& b, int& c ) override;
	int GetTall() override;
	void DrawSetTextFont( SurfacePlat* plat ) override;
	int getId() override;

private:
	VFontData	m_FontData;
	char*		m_pName;
	int			_id;
};

class EngineFont
{
public:
	EngineFont( const char* name, void* pFileData, int fileDataLen, int tall, int wide, float rotation, int weight,
		bool italic, bool underline, bool strikeout, bool symbol );
	virtual ~EngineFont();

	virtual void Init( const char* name, void* pFileData, int fileDataLen, int tall, int wide, float rotation,
		int weight, bool italic, bool underline, bool strikeout, bool symbol );
	virtual void getCharRGBA( int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba );
	virtual void GetCharABCwide( int ch, int& a, int& b, int& c );
	virtual void GetTextSize( const char* text, int& wide, int& tall );
	virtual int GetTall();
	virtual int getId();

	BaseFontPlat* getPlat();

private:
	char*			_name;
	BaseFontPlat*	_plat;
	int				_id;
};

bool	Load32BitTGA( FileImageStream* fp, FileImage* pImage );
bool	LoadVFontDataFrom32BitTGA( FileImageStream* fp, VFontData* pData );
void	Save32BitTGA( void* fp, FileImage* pImage );

void	Font_Reset();
