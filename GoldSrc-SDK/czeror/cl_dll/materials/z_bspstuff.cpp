#include "hud.h"
#include "z_bspstuff.h"
#include "z_materials.h"

#include <string.h>

byte* mod_base;
char g_szDesiredMaterialNames[512][64];
int g_iDesiredMaterials;

bool TextureNameInList( char* textureName )
{
	for ( int i = 0; i < g_iDesiredMaterials; ++i )
	{
		if ( !strcmp( textureName, g_szDesiredMaterialNames[i] ) )
			return true;
	}

	return false;
}

void LoadTextures( lump_t* lump )
{
	g_iDesiredMaterials = 0;
	dmiptexlump_t* textures = (dmiptexlump_t*)( mod_base + lump->fileofs );

	for ( int i = 0; i < textures->nummiptex; ++i )
	{
		miptex_t* texture = (miptex_t*)( (byte*)textures + textures->dataofs[i] );
		if ( texture )
		{
			char* name = new char[strlen( texture->name ) + 1];
			g_pMaterialSystemPointer->FixTextureName( texture->name, name );

			if ( !TextureNameInList( name ) && g_pMaterialSystemPointer->IsValidMaterialName( name ) )
			{
				strcpy( g_szDesiredMaterialNames[g_iDesiredMaterials], name );
				++g_iDesiredMaterials;
			}

			delete[] name;
		}
	}
}

void LoadTextureLump( char* map )
{
	int length;
	byte* file = gEngfuncs.COM_LoadFile( map, 4, &length );
	if ( file )
	{
		mod_base = file;
		dheader_t* header = (dheader_t*)file;
		LoadTextures( &header->lumps[2] );
	}
}
