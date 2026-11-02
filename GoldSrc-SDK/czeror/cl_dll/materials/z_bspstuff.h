#ifndef Z_BSPSTUFF_H
#define Z_BSPSTUFF_H

typedef struct
{
	int	fileofs;
	int	filelen;
} lump_t;

typedef struct
{
	int		version;
	lump_t	lumps[15];
} dheader_t;

typedef struct miptex_s
{
	char			name[16];
	unsigned int	width;
	unsigned int	height;
	unsigned int	offsets[4];
} miptex_t;

typedef struct
{
	int	nummiptex;
	int	dataofs[4];
} dmiptexlump_t;

extern unsigned char*	mod_base;
extern char				g_szDesiredMaterialNames[512][64];
extern int				g_iDesiredMaterials;

bool	TextureNameInList( char* textureName );
void	LoadTextures( lump_t* lump );
void	LoadTextureLump( char* map );

#endif
