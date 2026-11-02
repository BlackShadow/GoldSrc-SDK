#ifndef VFONTDATA_H
#define VFONTDATA_H

#define VFONT_GLYPH_COUNT	256

class FileImageStream;

class VFontData
{
public:
	VFontData();
	~VFontData();

	int				m_CharWidths[VFONT_GLYPH_COUNT];
	int				m_BitmapCharWidth;
	int				m_BitmapCharHeight;
	unsigned char*	m_pBitmap;
};

bool	LoadVFontDataFrom32BitTGA( FileImageStream* fp, VFontData* pData );

#endif
