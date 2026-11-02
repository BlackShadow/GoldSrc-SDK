#ifndef HAMMER_SPRITEFILE_H
#define HAMMER_SPRITEFILE_H
#pragma once

#include <vector>
#include "../utils/sprgen/spritegn.h"

struct SpriteImage_t
{
	int						   width;
	int						   height;
	int						   left;
	int						   up;
	std::vector<unsigned char> pixels;
};

struct SpriteFrame_t
{
	std::vector<float>		   intervals;
	std::vector<SpriteImage_t> images;
};

class CSpriteFile
{
public:
	CSpriteFile();
	bool Load( const char* pszPath );
	bool Decode( const unsigned char* pData, size_t size );

	dsprite_t				   header;
	std::vector<SpriteFrame_t> frames;
};

#endif
