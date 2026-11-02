#include "gl_model.h"
#if !defined( SW_RENDERER )
#include "glquake.h"
#endif
#include "common.h"
#include "zone.h"
#include "engine_studio.h"

#include <cstdio>

#if defined( SW_RENDERER )
extern byte texgammatable[256];
#endif

/*
=================
Mod_LoadStudioModel
=================
*/
void Mod_LoadStudioModel( model_t* mod, void* buffer )
{
	int i;
#if !defined( SW_RENDERER )
	int total;
#endif
	byte* pin;
#if !defined( SW_RENDERER )
	byte* pal;
#endif
	studiohdr_t* phdr;
	mstudiotexture_t* ptexture;
#if !defined( SW_RENDERER )
	char name[256];
#endif

	pin = (byte*)buffer;
	phdr = (studiohdr_t*)pin;

	if ( LittleLong( phdr->version ) != STUDIO_VERSION )
	{
		Q_memset( phdr, 0, sizeof( *phdr ) );
		Q_strcpy( phdr->name, "bogus" );
		phdr->length = sizeof( *phdr );
		phdr->texturedataindex = sizeof( *phdr );
	}

	mod->type = mod_studio;
	mod->flags = phdr->flags;

#if defined( SW_RENDERER )
	Cache_Alloc( &mod->cache, phdr->length + phdr->numtextures * ( 256 * 4 * sizeof( word ) - 256 * 3 ), mod->name );
	byte* pout = (byte*)mod->cache.data;
	if ( !pout )
		return;
	if ( phdr->textureindex )
	{
		Q_memcpy( pout, pin, phdr->texturedataindex );
		ptexture = (mstudiotexture_t*)( pout + phdr->textureindex );
		byte* input = pin + phdr->texturedataindex;
		byte* output = pout + phdr->texturedataindex;
		for ( i = 0; i < phdr->numtextures; i++, ptexture++ )
		{
			int size = ptexture->width * ptexture->height;
			ptexture->index = (int)( output - pout );
			Q_memcpy( output, input, size );
			input += size;
			output += size;
			word* palette = (word*)output;
			for ( int j = 0; j < 256; j++, input += 3, palette += 4 )
			{
				palette[0] = texgammatable[input[0]];
				palette[1] = texgammatable[input[1]];
				palette[2] = texgammatable[input[2]];
				palette[3] = 0;
			}
			output += 256 * 4 * sizeof( word );
		}
	}
	else
		Q_memcpy( pout, pin, phdr->length );
#else
	total = phdr->textureindex ? phdr->texturedataindex : phdr->length;

	Cache_Alloc( &mod->cache, total, mod->name );
	if ( !mod->cache.data )
		return;

	if ( phdr->textureindex )
	{
		ptexture = (mstudiotexture_t*)( pin + phdr->textureindex );
		for ( i = 0; i < phdr->numtextures; i++, ptexture++ )
		{
			snprintf( name, sizeof( name ), "%s%s", mod->name, ptexture->name );
			pal = pin + ptexture->index + ptexture->width * ptexture->height;
			ptexture->index = GL_LoadTexture( name, GLT_STUDIO, ptexture->width, ptexture->height, pin + ptexture->index,
				!( ptexture->flags & STUDIO_NF_NOMIPS ), ( ptexture->flags & STUDIO_NF_MASKED ) != 0, pal );
		}
	}

	Q_memcpy( mod->cache.data, buffer, total );
#endif
}
