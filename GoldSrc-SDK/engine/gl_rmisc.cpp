#include "gl_rmisc.h"

#include "client.h"
#include "cmd.h"
#include "common.h"
#include "conprint.h"
#include "cvar.h"
#include "gl_model.h"
#include "glquake.h"
#include "ivideomode.h"
#include "render.h"
#include "sys.h"
#include "zone.h"

#include <cstdio>
#include <cstdlib>

cvar_t gl_watersides = { "gl_watersides", "0" };
cvar_t gl_flashblend = { "gl_flashblend", "0" };

cvar_t ati_npatch = { "ati_npatch", "1.0", FCVAR_ARCHIVE };
cvar_t gl_wireframe = { "gl_wireframe", "0", FCVAR_SPONLY };
cvar_t gl_widescreen_yfov = { "gl_widescreen_yfov", "1", FCVAR_ARCHIVE };
cvar_t r_bmodelinterp = { "r_bmodelinterp", "1" };
cvar_t r_norefresh = { "r_norefresh", "0" };
cvar_t r_lightmap = { "r_lightmap", "0" };
cvar_t r_fullbright = { "r_fullbright", "0" };
cvar_t r_decals = { "r_decals", "4096" };
cvar_t sp_decals = { "sp_decals", "4096", FCVAR_ARCHIVE };
cvar_t mp_decals = { "mp_decals", "300", FCVAR_ARCHIVE };
cvar_t r_drawentities = { "r_drawentities", "1" };
cvar_t r_drawviewmodel = { "r_drawviewmodel", "1" };
cvar_t r_mirroralpha = { "r_mirroralpha", "1" };
cvar_t r_wateralpha = { "r_wateralpha", "1" };
cvar_t r_dynamic = { "r_dynamic", "1" };
cvar_t r_novis = { "r_novis", "0" };
cvar_t r_speeds = { "r_speeds", "0" };
cvar_t d_spriteskip = { "d_spriteskip", "0" };
cvar_t r_wadtextures = { "r_wadtextures", "0" };
cvar_t r_shadows = { "r_shadows", "0" };
cvar_t r_mmx = { "r_mmx", "0" };
cvar_t r_traceglow = { "r_traceglow", "0" };
cvar_t r_glowshellfreq = { "r_glowshellfreq", "2.2" };
cvar_t r_prefertexturefiltering = { "r_prefertexturefiltering", "1", FCVAR_ARCHIVE };
cvar_t gl_clear = { "gl_clear", "0" };
cvar_t gl_cull = { "gl_cull", "1" };
cvar_t gl_affinemodels = { "gl_affinemodels", "0" };
cvar_t gl_dither = { "gl_dither", "1", FCVAR_ARCHIVE };
cvar_t gl_spriteblend = { "gl_spriteblend", "1", FCVAR_ARCHIVE };
cvar_t gl_polyoffset = { "gl_polyoffset", "4", FCVAR_ARCHIVE };
cvar_t gl_lightholes = { "gl_lightholes", "1", FCVAR_ARCHIVE };
cvar_t gl_keeptjunctions = { "gl_keeptjunctions", "1", FCVAR_ARCHIVE };
cvar_t gl_wateramp = { "gl_wateramp", "0.3", FCVAR_ARCHIVE };
cvar_t gl_overbright = { "gl_overbright", "1", FCVAR_ARCHIVE };
cvar_t gl_zmax = { "gl_zmax", "4096" };
cvar_t gl_alphamin = { "gl_alphamin", "0.25" };
cvar_t gl_flipmatrix = { "gl_flipmatrix", "0", FCVAR_ARCHIVE };
cvar_t gl_monolights = { "gl_monolights", "0", FCVAR_ARCHIVE };
cvar_t gl_fog = { "gl_fog", "1", FCVAR_ARCHIVE };
cvar_t gl_use_shaders = { "gl_use_shaders", "1", FCVAR_ARCHIVE };
cvar_t gl_reduce_shader_changes = { "gl_reduce_shader_changes", "1" };
cvar_t gl_display_lists = { "gl_display_lists", "0" };
cvar_t gl_envmapsize = { "gl_envmapsize", "256" };

qboolean filterMode;
float filterColorRed = 1.0f;
float filterColorGreen = 1.0f;
float filterColorBlue = 1.0f;
float filterBrightness = 1.0f;
qboolean atismoothing;
qboolean gl_texsort = TRUE;

extern cvar_t r_cachestudio;
extern cvar_t r_cullsequencebox;
extern texture_t* r_notexture_mip;
extern const char* gl_vendor;
extern const char* gl_renderer;
extern const char* gl_version;
extern const char* gl_extensions;
extern cl_entity_t r_worldentity;
extern mleaf_t* r_viewleaf;
extern int d_lightstylevalue[256];
extern int playertextures[16];
extern int particletexture;
extern int skytexturenum;
extern int mirrortexturenum;
extern qboolean envmap;
extern int gl_mtexable;
extern int glx, gly, glwidth, glheight;
extern cvar_t gl_ansio;

void GL_LoadFilterTexture( float red, float green, float blue, float brightness );
void GL_BuildLightmaps();
void R_Envmap_f();
void GL_Bind( int texture );
void R_TimeRefresh_f();
void R_ReadPointFile_f();
void R_ReloadShaders();
void R_InitParticles();
void R_ClearParticles();
void R_DecalInit();
void V_InitLevel();
void GL_UnloadTextures();
void GL_BeginRendering( int* x, int* y, int* width, int* height );
void GL_EndRendering();
void R_RenderView();
void VID_TakeSnapshotRect( char* name, int x, int y, int width, int height );

void GL_Dump_f()
{
	if ( gl_vendor )
		Con_Printf( "GL Vendor: %s\n", gl_vendor );
	if ( gl_renderer )
		Con_Printf( "GL Renderer: %s\n", gl_renderer );
	if ( gl_version )
		Con_Printf( "GL Version: %s\n", gl_version );
	if ( gl_extensions )
		Con_Printf( "GL Extensions: %s\n", gl_extensions );
}

void R_CheckVariables()
{
	static float oldFilterMode;
	static float oldFilterColorRed;
	static float oldFilterColorGreen;
	static float oldFilterColorBlue;
	static float oldFilterBrightness;

	if ( (float)filterMode == oldFilterMode && filterColorRed == oldFilterColorRed
		 && filterColorGreen == oldFilterColorGreen && filterColorBlue == oldFilterColorBlue
		 && filterBrightness == oldFilterBrightness )
		return;

	oldFilterMode = (float)filterMode;
	oldFilterColorRed = filterColorRed;
	oldFilterColorGreen = filterColorGreen;
	oldFilterColorBlue = filterColorBlue;
	oldFilterBrightness = filterBrightness;

	GL_LoadFilterTexture( filterColorRed, filterColorGreen, filterColorBlue, filterBrightness );
	GL_BuildLightmaps();
}

/*
==================
R_InitTextures
==================
*/
void R_InitTextures()
{
	int x, y, m;
	byte* dest;

	// create a simple checkerboard texture for the default
	r_notexture_mip = (texture_t*)Hunk_AllocName( sizeof( texture_t ) + 16 * 16 + 8 * 8 + 4 * 4 + 2 * 2, "notexture" );

	r_notexture_mip->width = r_notexture_mip->height = 16;
	r_notexture_mip->offsets[0] = sizeof( texture_t );
	r_notexture_mip->offsets[1] = r_notexture_mip->offsets[0] + 16 * 16;
	r_notexture_mip->offsets[2] = r_notexture_mip->offsets[1] + 8 * 8;
	r_notexture_mip->offsets[3] = r_notexture_mip->offsets[2] + 4 * 4;

	for ( m = 0; m < 4; m++ )
	{
		dest = (byte*)r_notexture_mip + r_notexture_mip->offsets[m];
		for ( y = 0; y < ( 16 >> m ); y++ )
		{
			for ( x = 0; x < ( 16 >> m ); x++ )
			{
				if ( ( y < ( 8 >> m ) ) ^ ( x < ( 8 >> m ) ) )
					*dest++ = 0;
				else
					*dest++ = 0xFF;
			}
		}
	}
}

byte dottexture[8][8] =
{
	{ 0, 1, 1, 0, 0, 0, 0, 0 },
	{ 1, 1, 1, 1, 0, 0, 0, 0 },
	{ 1, 1, 1, 1, 0, 0, 0, 0 },
	{ 0, 1, 1, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0 },
};

static void r_prefertexturefiltering_hook_callback( cvar_t* var )
{
	if ( var )
		Cvar_Set( "gl_texturemode", var->value == 0.0f ? "GL_NEAREST" : "GL_LINEAR_MIPMAP_LINEAR" );
}

cvarhook_t r_prefertexturefiltering_hook = { r_prefertexturefiltering_hook_callback, NULL, NULL };

void R_InitParticleTexture()
{
	int x, y;
	byte data[8][8][4];
	int width, height, bpp;

	//
	// particle texture
	//
	particletexture = GL_GenTexture();
	GL_Bind( particletexture );

	for ( x = 0; x < 8; x++ )
	{
		for ( y = 0; y < 8; y++ )
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = (byte)-dottexture[x][y];
		}
	}

	VideoMode_GetCurrentVideoMode( &width, &height, &bpp );
	qglTexImage2D( GL_TEXTURE_2D, 0, bpp == 32 ? GL_RGBA8 : GL_RGB5_A1, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

	qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, gl_ansio.value );
}

void R_UploadEmptyTex()
{
	byte pPal[768] = {};

	pPal[765] = 255;
	pPal[767] = 255;

	r_notexture_mip->gl_texturenum = GL_LoadTexture( "**empty**", GLT_SYSTEM, r_notexture_mip->width,
		r_notexture_mip->height, (byte*)( r_notexture_mip + 1 ), TRUE, 0, pPal );
}

void R_Init()
{
	int i;
	qboolean useDisplayLists;

	Cmd_AddCommandWithFlags( "timerefresh", R_TimeRefresh_f, CMD_FLAG_FILTERED );
	Cmd_AddCommand( "envmap", R_Envmap_f );
	Cmd_AddCommand( "pointfile", R_ReadPointFile_f );
	Cmd_AddCommand( "gl_dump", GL_Dump_f );

	Cvar_RegisterVariable( &ati_npatch );
	Cvar_RegisterVariable( &gl_wireframe );
	Cvar_RegisterVariable( &gl_widescreen_yfov );
	Cvar_RegisterVariable( &r_cachestudio );
	Cvar_RegisterVariable( &r_cullsequencebox );
	Cvar_RegisterVariable( &r_bmodelinterp );
	Cvar_RegisterVariable( &r_norefresh );
	Cvar_RegisterVariable( &r_lightmap );
	Cvar_RegisterVariable( &r_fullbright );
	Cvar_RegisterVariable( &r_decals );
	Cvar_RegisterVariable( &sp_decals );
	Cvar_RegisterVariable( &mp_decals );
	Cvar_SetValue( "r_decals", 4096 );
	Cvar_RegisterVariable( &r_drawentities );
	Cvar_RegisterVariable( &r_drawviewmodel );
	Cvar_RegisterVariable( &r_mirroralpha );
	Cvar_RegisterVariable( &r_wateralpha );
	Cvar_RegisterVariable( &r_dynamic );
	Cvar_RegisterVariable( &r_novis );
	Cvar_RegisterVariable( &r_speeds );
	Cvar_RegisterVariable( &d_spriteskip );
	Cvar_RegisterVariable( &r_wadtextures );
	Cvar_RegisterVariable( &r_shadows );
	Cvar_RegisterVariable( &r_mmx );
	Cvar_RegisterVariable( &r_traceglow );
	Cvar_RegisterVariable( &r_glowshellfreq );
	Cvar_RegisterVariable( &r_prefertexturefiltering );
	Cvar_HookVariable( r_prefertexturefiltering.name, &r_prefertexturefiltering_hook );

	Cvar_RegisterVariable( &gl_clear );
	Cvar_RegisterVariable( &gl_cull );
	Cvar_RegisterVariable( &gl_affinemodels );
	Cvar_RegisterVariable( &gl_dither );
	Cvar_RegisterVariable( &gl_spriteblend );
	Cvar_RegisterVariable( &gl_polyoffset );
	Cvar_RegisterVariable( &gl_lightholes );
	Cvar_RegisterVariable( &gl_keeptjunctions );
	Cvar_RegisterVariable( &gl_wateramp );
	Cvar_RegisterVariable( &gl_overbright );
	Cvar_RegisterVariable( &gl_zmax );
	Cvar_RegisterVariable( &gl_alphamin );
	Cvar_RegisterVariable( &gl_flipmatrix );
	Cvar_RegisterVariable( &gl_monolights );
	Cvar_RegisterVariable( &gl_fog );
	Cvar_RegisterVariable( &gl_use_shaders );
	Cvar_RegisterVariable( &gl_reduce_shader_changes );
	Cvar_RegisterVariable( &gl_display_lists );
	Cmd_AddCommand( "gl_reloadshaders", R_ReloadShaders );

	if ( gl_mtexable )
	{
		gl_texsort = FALSE;
		Cvar_SetValue( "gl_overbright", 0 );
	}

	useDisplayLists = COM_CheckParm( "-displaylists" ) && !COM_CheckParm( "-nodisplaylists" );
	if ( getenv( "SteamDeck" ) )
	{
		Con_Printf( "Running on Steam Deck.\n" );
		useDisplayLists = TRUE;
	}
	Cvar_Set( "gl_display_lists", useDisplayLists ? "1" : "0" );

	R_InitParticles();
	R_InitParticleTexture();
	R_UploadEmptyTex();

	for ( i = 0; i < 16; i++ )
		playertextures[i] = GL_GenTexture();
}

/*
===============
R_NewMap
===============
*/
void R_NewMap()
{
	int i;
	texture_t* tx;

	for ( i = 0; i < 256; i++ )
		d_lightstylevalue[i] = 264; // normal light value

	Q_memset( &r_worldentity, 0, sizeof( r_worldentity ) );
	r_worldentity.model = cl.worldmodel;

	// clear out efrags in case the level hasn't been reloaded
// FIXME: is this one short?
	for ( i = 0; i < cl.worldmodel->numleafs; i++ )
		cl.worldmodel->leafs[i].efrags = NULL;

	r_viewleaf = NULL;
	R_ClearParticles();

	R_DecalInit();
	V_InitLevel();
	GL_BuildLightmaps();

	// identify sky texture
	skytexturenum = -1;
	mirrortexturenum = -1;
	for ( i = 0; i < cl.worldmodel->numtextures; i++ )
	{
		tx = cl.worldmodel->textures[i];
		if ( !tx )
			continue;
		if ( !Q_strncmp( tx->name, "sky", 3 ) )
			skytexturenum = i;
		if ( !Q_strncmp( tx->name, "window02_1", 10 ) )
			mirrortexturenum = i;
		tx->texturechain = NULL;
	}
	R_LoadSkys();
	cl_entities[0].curstate.scale = gl_wateramp.value;

	GL_UnloadTextures();
}

/*
===============
R_Envmap_f

Grab six views for environment mapping tests
===============
*/
void R_Envmap_f()
{
	static const char* names[6] = { "%srt.bmp", "%sbk.bmp", "%slf.bmp", "%sft.bmp", "%sup.bmp", "%sdn.bmp" };
	char name[1024];
	vrect_t saveVrect;
	char base[64];
	int i, size;

	if ( !cl.worldmodel )
	{
		Con_Printf( "No map loaded\n" );
		return;
	}
	if ( gl_envmapsize.value <= 0.0f )
	{
		Con_Printf( "Invalid gl_envmapsize value\n" );
		return;
	}

	if ( cl.num_entities && cl_entities[0].model )
		COM_FileBase( cl_entities[0].model->name, base );
	else
		Q_strcpy( base, "Env" );

	qglDrawBuffer( GL_FRONT );
	qglReadBuffer( GL_FRONT );
	envmap = TRUE;

	saveVrect = r_refdef.vrect;
	r_refdef.vrect.x = 0;
	r_refdef.vrect.y = 0;
	r_refdef.vrect.width = (int)gl_envmapsize.value;
	r_refdef.vrect.height = (int)gl_envmapsize.value;

	VectorClear( r_refdef.viewangles );

	for ( i = 0; i < 6; i++ )
	{
		if ( i >= 4 )
		{
			r_refdef.viewangles[PITCH] = ( i == 4 ) ? -90.0f : 90.0f;
			r_refdef.viewangles[YAW] = 0;
		}
		else if ( i > 0 )
		{
			r_refdef.viewangles[YAW] = i * 90.0f;
		}

		GL_BeginRendering( &glx, &gly, &glwidth, &glheight );
		R_RenderView();
		snprintf( name, sizeof( name ), names[i], base );
		size = (int)gl_envmapsize.value;
		VID_TakeSnapshotRect( name, 0, 0, size, size );
	}

	r_refdef.vrect = saveVrect;
	envmap = FALSE;
	qglDrawBuffer( GL_BACK );
	qglReadBuffer( GL_BACK );
	GL_EndRendering();
}

void D_FlushCaches()
{
	GL_BuildLightmaps();
}

/*
====================
R_TimeRefresh_f

For program optimization
====================
*/
void R_TimeRefresh_f()
{
	int i;
	float start, stop, time;

	if ( !cl.worldmodel )
	{
		Con_Printf( "No map loaded\n" );
		return;
	}
	if ( !sv_cheats.value )
	{
		Con_Printf( "sv_cheats not enabled\n" );
		return;
	}

	qglDrawBuffer( GL_FRONT );
	qglFinish();

	start = (float)Sys_FloatTime();
	for ( i = 0; i < 128; i++ )
	{
		r_refdef.viewangles[YAW] = (float)( i / 128.0 * 360.0 );
		R_RenderView();
	}

	qglFinish();
	stop = (float)Sys_FloatTime();
	time = stop - start;
	Con_Printf( "%f seconds (%f fps)\n", time, 128 / time );

	qglDrawBuffer( GL_BACK );
	GL_EndRendering();
}

void R_TranslatePlayerSkin( int )
{
}
