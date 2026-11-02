#if defined( SW_RENDERER )

#include "model.h"

void DT_Initialize()
{
}

void DT_LoadDetailMapFile( char* )
{
}

void DT_LoadDetailTexture( char*, int )
{
}

int DT_SetRenderState( int )
{
	return 0;
}

void DT_SetTextureCoordinates( float, float )
{
}

void DT_ClearRenderState()
{
}

#else

#include "const.h"
#include "cvar.h"
#include "conprint.h"
#include "filesystem.h"
#include "gl_model.h"
#include "glquake.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

#define DETAIL_IMAGE_BYTES ( 1024 * 1024 )

#define GL_COMBINE 0x8570
#define GL_COMBINE_RGB 0x8571
#define GL_RGB_SCALE 0x8573

extern int TEXTURE0_SGIS;
extern int TEXTURE1_SGIS;
extern int TEXTURE2_SGIS;

class DetailMap
{
public:
	DetailMap();
	DetailMap( const std::string& diffuseName, const std::string& detailName, float sScale, float tScale,
		GLint textureId, GLint detailId )
		: _diffuseName( diffuseName ), _detailName( detailName ), _sScale( sScale ), _tScale( tScale ),
		  _oglTextureId( textureId ), _oglDetailId( detailId )
	{
	}
	~DetailMap() = default;
	const std::string getDiffuseName()
	{
		return _diffuseName;
	}
	const std::string getDetailName()
	{
		return _detailName;
	}
	float getSScale();
	float getTScale();
	GLint getOglTextureId();
	GLint getOglDetailId();
	void setDiffuseName( const std::string& name );
	void setDetailName( const std::string& name );
	void setSScale( float scale );
	void setTScale( float scale );
	void setOglTextureId( GLint id )
	{
		_oglTextureId = id;
	}
	void setOglDetailId( GLint id )
	{
		_oglDetailId = id;
	}

private:
	std::string _diffuseName;
	std::string _detailName;
	float _sScale;
	float _tScale;
	GLint _oglTextureId;
	GLint _oglDetailId;
};

float DetailMap::getSScale()
{
	return _sScale;
}

float DetailMap::getTScale()
{
	return _tScale;
}

GLint DetailMap::getOglTextureId()
{
	return _oglTextureId;
}

GLint DetailMap::getOglDetailId()
{
	return _oglDetailId;
}

typedef enum
{
	PS_START,
	PS_DIFFUSE_NAME,
	PS_DETAIL_TEXTURE,
	PS_S_SCALE_FACTOR,
	PS_T_SCALE_FACTOR,
	PS_ENTRY,
	PS_ERROR,
	PS_COMPLETE
} ParseState;

static std::vector<DetailMap*> g_detailVector;
static std::map<int, DetailMap*> g_idMap;
static std::map<std::string, int> g_detailNameToIdMap;
static std::map<std::string, int> g_detailLoadFailedMap;
static bool g_demandLoad;
static bool g_detTexLoaded;
static std::string g_levelName;
static std::vector<std::pair<std::string, int>> g_decalTexIDs;
static bool detTexSupported;

static const char whiteSpace[] = { '\r', '\n', '\t', ' ' };
static const char extraTokenCharacters[] = { '_', '.', '-', '/', '{', '~', '!', '+', '[', ']' };

static GLint lastDetTex = -1;
static GLfloat lastDetSScale = 1.0f;
static GLfloat lastDetTScale = 1.0f;

cvar_t r_detailtextures = { "r_detailtextures", "0", FCVAR_ARCHIVE };
cvar_t r_detailtexturessupported = { "r_detailtexturessupported", "1", FCVAR_SPONLY };

static bool DT_IsWhiteSpace( char c )
{
	for ( char space : whiteSpace )
	{
		if ( c == space )
			return true;
	}

	return false;
}

static bool DT_IsTokenCharacter( char c )
{
	if ( ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) || ( c >= '0' && c <= '9' ) )
		return true;

	for ( char extra : extraTokenCharacters )
	{
		if ( c == extra )
			return true;
	}

	return false;
}

static bool DT_GetToken( char*& cursor, ParseState& parseState, std::string& token )
{
	char* start;
	char* end;

	start = cursor;
	while ( 1 )
	{
		while ( DT_IsWhiteSpace( *start ) )
			start++;

		if ( *start == (char)-1 )
		{
			cursor = start;
			parseState = PS_COMPLETE;
			return false;
		}

		if ( start[0] != '/' || start[1] != '/' )
			break;

		start += 2;
		while ( *start != '\r' && *start != '\n' && *start != (char)-1 )
			start++;
	}

	end = start;
	while ( DT_IsTokenCharacter( *end ) )
		end++;

	cursor = end;
	if ( start == end )
		parseState = PS_ERROR;
	else
		token = std::string( start, end - start );

	return parseState != PS_ERROR && parseState != PS_COMPLETE;
}

void DT_Parse( char* buffer )
{
	std::string diffuseName;
	std::string detailName;
	std::string token;
	char* cursor = buffer;
	float sScale = 0.0f;
	float tScale = 0.0f;
	ParseState state = PS_START;

	while ( state != PS_COMPLETE )
	{
		switch ( state )
		{
		case PS_START:
			cursor = buffer;
			state = PS_DIFFUSE_NAME;
			break;

		case PS_DIFFUSE_NAME:
			if ( DT_GetToken( cursor, state, diffuseName ) )
				state = PS_DETAIL_TEXTURE;
			break;

		case PS_DETAIL_TEXTURE:
			if ( DT_GetToken( cursor, state, detailName ) )
				state = PS_S_SCALE_FACTOR;
			break;

		case PS_S_SCALE_FACTOR:
			if ( DT_GetToken( cursor, state, token ) )
			{
				sScale = (float)atof( token.c_str() );
				state = PS_T_SCALE_FACTOR;
			}
			break;

		case PS_T_SCALE_FACTOR:
			if ( DT_GetToken( cursor, state, token ) )
			{
				tScale = (float)atof( token.c_str() );
				state = PS_ENTRY;
			}
			break;

		case PS_ENTRY:
			g_detailVector.push_back( new DetailMap( diffuseName, detailName, sScale, tScale, 0, 0 ) );
			state = PS_DIFFUSE_NAME;
			break;

		case PS_ERROR:
			Con_Printf( "Error parsing detail texture file.\n" );
			return;

		case PS_COMPLETE:
			break;
		}
	}
}

static GLint loadTextureFile( const char* name )
{
	byte* data;
#ifdef _WIN32
	char path[MAX_PATH];
#else
	char path[4096];
#endif
	int width, height;
	GLint texture;

	data = (byte*)malloc( DETAIL_IMAGE_BYTES );
	snprintf( path, sizeof( path ), "gfx/%s.tga", name );
	if ( !data )
		return -1;

	texture = -1;
	if ( LoadTGA2( path, data, DETAIL_IMAGE_BYTES, &width, &height, 0 ) )
	{
		texture = GL_LoadTexture2(
			(char*)name, GLT_SPRITE, width, height, data, true, TEX_TYPE_RGBA, NULL, GL_LINEAR_MIPMAP_LINEAR );
	}

	free( data );
	return texture;
}

void DT_LoadDetailMapFile( char* levelName )
{
	size_t i;
	FileHandle_t file;
	unsigned int length;
	char* buffer;

	if ( !detTexSupported )
		return;

	if ( !r_detailtextures.value )
	{
		g_detTexLoaded = false;
		g_levelName = levelName;
		return;
	}

	g_detTexLoaded = true;
	if ( !g_demandLoad )
		g_decalTexIDs.clear();

	for ( i = 0; i < g_detailVector.size(); i++ )
	{
		delete g_detailVector[i];
		g_detailVector[i] = NULL;
	}

	g_detailVector.clear();
	g_idMap.clear();
	g_detailNameToIdMap.clear();
	g_detailLoadFailedMap.clear();

	std::string fileName = "maps/" + std::string( levelName ) + "_detail.txt";
	file = FS_Open( fileName.c_str(), "rb" );
	if ( !file )
	{
		Con_Printf( "No detail texture mapping file: %s\n", fileName.c_str() );
		return;
	}

	length = FS_FileSize( fileName.c_str() );
	buffer = new char[length + 10];
	if ( FS_Read( buffer, length, 1, file ) > 0 )
	{
		FS_Close( file );
		buffer[length] = (char)-1;
		DT_Parse( buffer );
		delete[] buffer;
	}
	else
	{
		Con_Printf( "Detail texture mapping file read failed\n" );
	}
}

void DT_LoadDetailTexture( char* diffuseName, int diffuseId )
{
	size_t i;
	DetailMap* detail;
	GLint texture;

	if ( !detTexSupported )
		return;

	if ( !r_detailtextures.value )
	{
		g_decalTexIDs.push_back( std::make_pair( std::string( diffuseName ), diffuseId ) );
		return;
	}

	for ( i = 0; i < g_detailVector.size(); i++ )
	{
		detail = g_detailVector[i];
		if ( _stricmp( diffuseName, detail->getDiffuseName().c_str() ) )
			continue;

		std::string detailName = detail->getDetailName();
		if ( g_detailLoadFailedMap.find( detailName ) != g_detailLoadFailedMap.end() )
			return;

		auto loaded = g_detailNameToIdMap.find( detailName );
		if ( loaded != g_detailNameToIdMap.end() )
		{
			detail->setOglTextureId( diffuseId );
			detail->setOglDetailId( loaded->second );
			g_idMap.insert( std::make_pair( diffuseId, detail ) );
			return;
		}

		texture = loadTextureFile( detailName.c_str() );
		if ( texture == -1 )
		{
			Con_Printf( "Detail texture map load failed: %s\n", detailName.c_str() );
			g_detailLoadFailedMap.insert( std::make_pair( detailName, 0 ) );
			return;
		}

		detail->setOglTextureId( diffuseId );
		detail->setOglDetailId( texture );
		g_idMap.insert( std::make_pair( diffuseId, detail ) );
		g_detailNameToIdMap.insert( std::make_pair( detailName, texture ) );
		return;
	}
}

const char* DT_GetDetailTextureName( char* diffuseName )
{
	static char buffer[128];
	size_t i;
	DetailMap* detail;

	for ( i = 0; i < g_detailVector.size(); i++ )
	{
		detail = g_detailVector[i];
		if ( _stricmp( diffuseName, detail->getDiffuseName().c_str() ) )
			continue;

		std::string detailName = detail->getDetailName();
		if ( detailName.empty() )
			return NULL;

		if ( detailName.length() >= sizeof( buffer ) )
			abort();
		strcpy( buffer, detailName.c_str() );
		return buffer;
	}

	return NULL;
}

void DT_Initialize()
{
	Cvar_RegisterVariable( &r_detailtextures );
	Cvar_RegisterVariable( &r_detailtexturessupported );

	qglSelectTextureSGIS( TEXTURE2_SGIS );
	qglEnable( GL_TEXTURE_2D );
	qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, (GLfloat)GL_COMBINE );
	qglTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB, (GLfloat)GL_MODULATE );
	qglTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f );
	qglDisable( GL_TEXTURE_2D );
	qglSelectTextureSGIS( TEXTURE0_SGIS );

	detTexSupported = true;
}

int DT_SetRenderState( int diffuseId )
{
	size_t i;
	DetailMap* detail;
	bool textureChanged, scaleChanged;

	if ( !detTexSupported || !r_detailtextures.value )
		return 0;

	if ( !g_detTexLoaded )
	{
		Con_Printf( "Loading Detail Textures...\n" );
		g_demandLoad = true;
		DT_LoadDetailMapFile( (char*)g_levelName.c_str() );
		for ( i = 0; i < g_decalTexIDs.size(); i++ )
		{
			std::pair<std::string, int> texture = g_decalTexIDs[i];
			DT_LoadDetailTexture( (char*)texture.first.c_str(), texture.second );
		}
		g_detTexLoaded = true;
		g_demandLoad = false;
	}

	auto found = g_idMap.find( diffuseId );
	if ( found == g_idMap.end() )
		return 0;

	detail = found->second;
	if ( !detail->getOglDetailId() || !detail->getOglTextureId() )
		return 0;

	qglSelectTextureSGIS( TEXTURE2_SGIS );
	qglEnable( GL_TEXTURE_2D );

	textureChanged = lastDetTex == -1 || lastDetTex != detail->getOglDetailId();
	if ( textureChanged )
		lastDetTex = detail->getOglDetailId();

	scaleChanged = !( lastDetSScale == detail->getSScale() && lastDetTScale == detail->getTScale() );
	if ( scaleChanged )
	{
		lastDetSScale = detail->getSScale();
		lastDetTScale = detail->getTScale();
	}

	if ( textureChanged )
		qglBindTexture( GL_TEXTURE_2D, lastDetTex );

	if ( scaleChanged )
	{
		qglMatrixMode( GL_TEXTURE );
		qglLoadIdentity();
		qglScalef( detail->getSScale(), detail->getTScale(), 1.0f );
		qglMatrixMode( GL_MODELVIEW );
	}

	return 1;
}

void DT_ClearRenderState()
{
	qglSelectTextureSGIS( TEXTURE2_SGIS );
	qglDisable( GL_TEXTURE_2D );
	qglSelectTextureSGIS( TEXTURE1_SGIS );
}

void DT_SetTextureCoordinates( float u, float v )
{
	qglMTexCoord2fSGIS( TEXTURE2_SGIS, u, v );
}

#endif
