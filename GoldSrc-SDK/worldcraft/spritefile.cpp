#include "spritefile.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits.h>

class CSpriteReader
{
public:
	CSpriteReader( const unsigned char* pData, size_t size ) : m_pData( pData ), m_Remaining( size ) {}
	bool Read( void* pOutput, size_t size )
	{
		if ( size > m_Remaining )
			return false;
		memcpy( pOutput, m_pData, size );
		m_pData += size;
		m_Remaining -= size;
		return true;
	}
	size_t Remaining() const { return m_Remaining; }

private:
	const unsigned char* m_pData;
	size_t				 m_Remaining;
};

static bool ReadSpriteImage( CSpriteReader& reader, SpriteImage_t& image, const unsigned char* pPalette,
							 unsigned short colors, int format )
{
	dspriteframe_t frame;
	if ( !reader.Read( &frame, sizeof( frame ) ) || frame.width <= 0 || frame.height <= 0 ||
		 frame.width > INT_MAX / 4 / frame.height )
		return false;
	size_t count = (size_t)frame.width * frame.height;
	if ( count > reader.Remaining() )
		return false;
	std::vector<unsigned char> indices( count );
	if ( !reader.Read( indices.data(), count ) )
		return false;
	image.width = frame.width;
	image.height = frame.height;
	image.left = frame.origin[0];
	image.up = frame.origin[1];
	image.pixels.resize( count * 4 );
	for ( size_t i = 0; i < count; i++ )
	{
		unsigned int index = indices[i];
		if ( index >= colors )
			return false;
		const unsigned char* pColor = pPalette + 3 * ( format == SPR_INDEXALPHA ? 255 : index );
		unsigned char*		 pPixel = &image.pixels[i * 4];
		memcpy( pPixel, pColor, 3 );
		pPixel[3] =
			format == SPR_INDEXALPHA ? (unsigned char)index : ( format == SPR_ALPHTEST && index == 255 ? 0 : 255 );
	}
	return true;
}

CSpriteFile::CSpriteFile() : header()
{
}

bool CSpriteFile::Load( const char* pszPath )
{
	std::ifstream file( pszPath, std::ios::binary | std::ios::ate );
	if ( !file )
		return false;
	std::streamoff size = file.tellg();
	if ( size < (std::streamoff)sizeof( dsprite_t ) || size > INT_MAX )
		return false;
	std::vector<unsigned char> data( (size_t)size );
	file.seekg( 0 );
	if ( !file.read( (char*)data.data(), (std::streamsize)size ) )
		return false;
	return Decode( data.data(), data.size() );
}

bool CSpriteFile::Decode( const unsigned char* pData, size_t size )
{
	if ( !pData )
		return false;
	CSpriteReader reader( pData, size );
	CSpriteFile	  sprite;
	if ( !reader.Read( &sprite.header, sizeof( sprite.header ) ) )
		return false;
	const dsprite_t& info = sprite.header;
	if ( info.ident != IDSPRITEHEADER || info.version != SPRITE_VERSION || info.width <= 0 || info.height <= 0 ||
		 info.type < SPR_VP_PARALLEL_UPRIGHT || info.type > SPR_VP_PARALLEL_ORIENTED || info.texFormat < SPR_NORMAL ||
		 info.texFormat > SPR_ALPHTEST || info.numframes <= 0 ||
		 (size_t)info.numframes > reader.Remaining() / sizeof( dspriteframetype_t ) )
		return false;
	unsigned short colors;
	unsigned char  palette[256 * 3] = {};
	if ( !reader.Read( &colors, sizeof( colors ) ) || colors == 0 || colors > 256 ||
		 ( info.texFormat == SPR_INDEXALPHA && colors != 256 ) || !reader.Read( palette, colors * 3 ) )
		return false;
	for ( int i = 0; i < info.numframes; i++ )
	{
		dspriteframetype_t type;
		if ( !reader.Read( &type, sizeof( type ) ) || ( type.type != SPR_SINGLE && type.type != SPR_GROUP ) )
			return false;
		SpriteFrame_t frame;
		int			  count = 1;
		if ( type.type == SPR_GROUP )
		{
			if ( !reader.Read( &count, sizeof( count ) ) || count <= 0 ||
				 (size_t)count > reader.Remaining() / ( sizeof( float ) + sizeof( dspriteframe_t ) + 1 ) )
				return false;
			frame.intervals.resize( count );
			float previous = 0;
			for ( int j = 0; j < count; j++ )
			{
				float& interval = frame.intervals[j];
				if ( !reader.Read( &interval, sizeof( interval ) ) || !std::isfinite( interval ) ||
					 interval <= previous )
					return false;
				previous = interval;
			}
		}
		for ( int j = 0; j < count; j++ )
		{
			SpriteImage_t image;
			if ( !ReadSpriteImage( reader, image, palette, colors, info.texFormat ) )
				return false;
			frame.images.push_back( std::move( image ) );
		}
		sprite.frames.push_back( std::move( frame ) );
	}
	header = sprite.header;
	frames.swap( sprite.frames );
	return true;
}
