//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Revision: $
// $NoKeywords: $
//
// common.h  -- general definitions -- shared by client and server portions of host
//=============================================================================

#pragma once

#ifdef _WIN32
#include <windows.h>
#else
typedef unsigned char	BYTE;
typedef short			WORD;
typedef unsigned int	DWORD;
typedef int				LONG;
typedef unsigned int	COLORREF;

typedef struct BITMAPINFOHEADER
{
	DWORD	biSize;
	LONG	biWidth;
	LONG	biHeight;
	WORD	biPlanes;
	WORD	biBitCount;
	DWORD	biCompression;
	DWORD	biSizeImage;
	LONG	biXPelsPerMeter;
	LONG	biYPelsPerMeter;
	DWORD	biClrUsed;
	DWORD	biClrImportant;
} BITMAPINFOHEADER;

#pragma pack( push, 2 )
typedef struct BITMAPFILEHEADER
{
	WORD	bfType;
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} BITMAPFILEHEADER;
#pragma pack( pop )

typedef struct RGBQUAD
{
	BYTE	rgbBlue;
	BYTE	rgbGreen;
	BYTE	rgbRed;
	BYTE	rgbReserved;
} RGBQUAD;

typedef struct BITMAPINFO
{
	BITMAPINFOHEADER	bmiHeader;
	RGBQUAD				bmiColors[1];
} BITMAPINFO;

typedef BITMAPINFO* LPBITMAPINFO;
#endif
