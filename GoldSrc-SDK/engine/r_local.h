#pragma once

#include "model.h"
#include "render.h"
#include "r_shared.h"
#include "d_iface.h"
#include "engine_color.h"
#include "water.h"

#include "../common/dlight.h"

#define ALIAS_LEFT_CLIP			1
#define ALIAS_TOP_CLIP			2
#define ALIAS_RIGHT_CLIP		4
#define ALIAS_BOTTOM_CLIP		8
#define ALIAS_Z_CLIP			16
#define ALIAS_ONSEAM			32
#define ALIAS_Z_CLIP_PLANE		5

#define ALIAS_XY_CLIP_MASK		( ALIAS_LEFT_CLIP | ALIAS_TOP_CLIP | ALIAS_RIGHT_CLIP | ALIAS_BOTTOM_CLIP )
#define ALIAS_CLIP_MASK			( ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP )

#define CACHE_SIZE				32
#define BMODEL_FULLY_CLIPPED	16

#define MAX_STACK_EDGES			14400
#define MAX_STACK_SURFACES		4000

typedef struct alight_s
{
	int		ambientlight;
	int		shadelight;
	vec3_t	color;
	float*	plightvec;
} alight_t;

//===========================================================================
// clipped bmodel edges

typedef struct bedge_s
{
	mvertex_t*		v[2];
	struct bedge_s*	pnext;
} bedge_t;

typedef struct
{
	float	fv[3];		// viewspace x, y
} auxvert_t;

typedef struct clipplane_s
{
	vec3_t				normal;
	float				dist;

	struct clipplane_s*	next;
	byte				leftedge;
	byte				rightedge;
	byte				reserved[2];
} clipplane_t;

typedef struct btofpoly_s
{
	int			clipflags;
	msurface_t*	psurf;
} btofpoly_t;

//===========================================================================

extern cvar_t		r_draworder;
extern cvar_t		r_speeds;
extern cvar_t		r_timegraph;
extern cvar_t		r_drawviewmodel;
extern cvar_t		r_aliasstats;
extern cvar_t		r_dspeeds;
extern cvar_t		r_reportsurfout;
extern cvar_t		r_reportedgeout;
extern cvar_t		r_maxsurfs;
extern cvar_t		r_maxedges;
extern cvar_t		r_aliastransbase;
extern cvar_t		r_aliastransadj;
extern cvar_t		r_mmx;
extern cvar_t		r_luminance;
extern cvar_t		r_graphheight;
extern cvar_t		r_waterwarp;
extern cvar_t		r_fullbright;
extern cvar_t		r_decals;
extern cvar_t		r_lightmap;
extern cvar_t		r_lightstyle;
extern cvar_t		r_drawentities;
extern cvar_t		r_ambient_r;
extern cvar_t		r_ambient_g;
extern cvar_t		r_ambient_b;
extern cvar_t		r_numsurfs;
extern cvar_t		r_numedges;
extern cvar_t		r_wadtextures;
extern cvar_t		r_traceglow;

extern qboolean		filterMode;
extern float		filterColorRed;
extern float		filterColorGreen;
extern float		filterColorBlue;
extern float		filterBrightness;

extern clipplane_t	view_clipplanes[4];
extern mplane_t		screenedge[4];
extern vec3_t		r_entorigin;
extern vec3_t		r_worldmodelorg;
extern int			r_visframecount;
extern qboolean		insubmodel;
extern int			c_faceclip;
extern int			r_polycount;
extern int			r_wholepolycount;
extern int*			pfrustum_indexes[4];
extern int			ubasestep;
extern int			errorterm;
extern int			erroradjustup;
extern int			erroradjustdown;
extern int			r_currentkey;
extern int			r_currentbkey;
extern int			numbtofpolys;
extern btofpoly_t*	pbtofpolys;
extern int			a_skinwidth;
extern finalvert_t*	pfinalverts;
extern auxvert_t*	pauxverts;
extern int			r_amodels_drawn;
extern edge_t*		auxedges;
extern int			r_numallocatededges;
extern edge_t*		r_edges;
extern edge_t*		edge_p;
extern edge_t*		edge_max;
extern edge_t*		newedges[1600];
extern edge_t*		removeedges[1600];
extern int			r_bmodelactive;
extern float		aliasxscale;
extern float		aliasyscale;
extern float		aliasxcenter;
extern float		aliasycenter;
extern int			r_outofsurfaces;
extern int			r_outofedges;
extern mvertex_t*	r_pcurrentvertbase;
extern float		r_time1;
extern float		dp_time1;
extern float		dp_time2;
extern float		db_time1;
extern float		db_time2;
extern float		rw_time1;
extern float		rw_time2;
extern float		se_time1;
extern float		se_time2;
extern float		de_time1;
extern float		de_time2;
extern float		dv_time1;
extern float		dv_time2;
extern int			r_frustum_indexes[24];
extern int			r_maxsurfsseen;
extern int			r_maxedgesseen;
extern qboolean		r_dowarpold;
extern qboolean		r_viewchanged;
extern mleaf_t*		r_viewleaf;
extern mleaf_t*		r_oldviewleaf;
extern vec3_t		r_emins;
extern vec3_t		r_emaxs;
extern mnode_t*		r_pefragtopnode;
extern int			r_clipflags;
extern int			r_cnumsurfs;
extern int			r_surfsonstack;
extern colorVec		r_icolormix;
extern float		screenAspect;
extern float		r_aliastransition;
extern float		r_resfudge;
extern qboolean		r_fov_greater_than_90;
extern qboolean		g_bUsingInGameAdvertisements;
extern int			lightgammatable[1024];

extern byte			r_lut[65536];
extern word			red_64klut[65536];
extern word			green_64klut[65536];
extern word			blue_64klut[65536];

extern float		entity_rotation[3][3];
extern polydesc_t	r_polydesc;
extern zpointdesc_t	r_zpointdesc;

extern espan_t*		span_p;
extern espan_t*		max_span_p;
extern int			current_iv;
// FIXME: make stack vars when debugging done
extern edge_t		edge_head;
extern edge_t		edge_tail;
extern edge_t		edge_aftertail;
extern edge_t		edge_sentinel;
extern int			edge_head_u_shift20;
extern int			edge_tail_u_shift20;

extern int			r_dlightframecount;
extern int			r_dlightchanged; // which ones changed
extern int			r_dlightactive; // which ones are active

extern trivertx_t*	r_apverts;
extern vec3_t		r_plightvec;
extern int			r_ambientlight;
extern float		r_shadelight;
extern int			r_anumverts;
extern float		aliastransform[3][4];
extern float		r_avertexnormals[162][3];
extern float		ziscale;
extern float		zishift;
extern mdl_t*		pmdl;
extern aliashdr_t*	paliashdr;

extern float		viewmatrix[3][4];

void		R_InitTextures();
void		R_Init();
void		R_SetVrect( vrect_t* in, vrect_t* out, int lineAdjustment );
void		R_ViewChanged( vrect_t* rect, int lineAdjustment, float aspect );
void		R_MarkLeaves();
void		R_DrawEntitiesOnList();
void		R_PreDrawViewModel();
void		R_DrawViewModel();
int			R_BmodelCheckBBox( model_t* model, float* bounds );
qboolean	R_CullBox( vec_t* mins, vec_t* maxs );
void		R_DrawBEntitiesOnList();
void		R_EdgeDrawing();
void		R_RenderView_();
void		R_SetStackBase();
void		R_RenderView();
void		R_LoadOverviewSpriteModel( model_t* model, byte* data, byte* palette, int xTiles, int yTiles );
model_t*	R_LoadMapSprite( const char* filename );
void		AllowFog( int allowed );

void	R_CheckVariables();
void	R_TimeRefresh_f();
void	R_LineGraph( int x, int y, int height );
void	R_TimeGraph();
void	R_PrintTimes();
void	R_PrintDSpeeds();
void	R_ScreenLuminance();
void	R_PrintAliasStats();
void	R_TransformFrustum();
void	R_SetUpFrustumIndexes();
void	R_SetupFrame();

void		R_DecalInit();
void		R_DrawInitLut();
texture_t*	R_TextureAnimation( texture_t* base );

void	R_RotateBmodel();
void	R_EntityRotate( vec_t* vector );
void	R_RecursiveClipBPoly( bedge_t* edges, mnode_t* node, msurface_t* surface );
void	R_DrawSubmodelPolygons( model_t* model, int clipflags );
void	R_DrawSolidClippedSubmodelPolygons( model_t* model );
void	R_RenderBmodelFaceReverse( model_t* model, msurface_t* surface );
void	R_RecursiveWorldNode( mnode_t* node, int clipflags );
void	R_RenderWorld();

void	R_EmitEdge( mvertex_t* first, mvertex_t* second );
void	R_ClipEdge( mvertex_t* first, mvertex_t* second, clipplane_t* clip );
void	R_EmitCachedEdge();
void	R_RenderFace( msurface_t* surface, int clipflags );
void	R_RenderPoly( msurface_t* surface, int clipflags );
void	R_RenderBmodelFace( bedge_t* edges, msurface_t* surface );
void	R_ZDrawSubmodelPolys( model_t* model );

void	R_BeginEdgeFrame();
void	R_DrawCulledPolys();
void	R_TrailingEdge( surf_t* surface, edge_t* edge );
void	R_RemoveEdges( edge_t* edge );
void	R_LeadingEdge( edge_t* edge );
void	R_CleanupSpan();
void	R_GenerateSpans();
void	R_InsertNewEdges( edge_t* edges, edge_t* list );
void	R_StepActiveU( edge_t* edge );
void	R_LeadingEdgeBackwards( edge_t* edge );
void	R_GenerateSpansBackward();
void	R_ScanEdges();

void	R_StoreEfrags( struct efrag_s** efrags );
void	R_RemoveEfrags( struct cl_entity_s* entity );
void	R_SplitEntityOnNode( mnode_t* node );
void	R_SplitEntityOnNode2( mnode_t* node );
void	R_AddEfrags( struct cl_entity_s* entity );

void		R_AnimateLight();
void		R_PushDlights();
void		R_MarkLights( dlight_t* light, int bit, mnode_t* node );
colorVec	R_LightVec( vec_t* start, vec_t* end );
colorVec	R_LightPoint( vec_t* point );

void	R_ClearSkyBox();
void	R_DrawSkyBox( int sortKey );
void	R_DrawSkyChain( msurface_t* surface );
void	R_InitFakeSkyModel();
void	R_LoadSkys();
void	R_InitSky();
void	R_FilterSkyBox();

void	R_AliasTransformAndProjectFinalVerts( finalvert_t* vertices, stvert_t* textureVertices );
void	R_AliasProjectFinalVert( finalvert_t* vertex, auxvert_t* auxiliaryVertex );
void	xR_AliasSetupFrame();

void	R_Alias_clip_left( finalvert_t* first, finalvert_t* second, finalvert_t* out );
void	R_Alias_clip_right( finalvert_t* first, finalvert_t* second, finalvert_t* out );
void	R_Alias_clip_z( finalvert_t* first, finalvert_t* second, finalvert_t* out );
void	R_Alias_clip_top( finalvert_t* first, finalvert_t* second, finalvert_t* out );
void	R_Alias_clip_bottom( finalvert_t* first, finalvert_t* second, finalvert_t* out );
int		R_AliasClip( finalvert_t* in, finalvert_t* out, int flag, int count,
		void ( *clip )( finalvert_t*, finalvert_t*, finalvert_t* ) );
void	R_AliasClipTriangle( mtriangle_t* triangle );

int		R_ClipSpriteFace( int count, clipplane_t* plane );
void	R_SetupAndDrawSprite();
void	R_DrawSprite();

void			R_GetSpriteAxes( struct cl_entity_s* entity, int type, vec_t* forward, vec_t* right, vec_t* up );
void			R_SpriteColor( colorVec* color, struct cl_entity_s* entity, int alpha );
float*			R_GetAttachmentPoint( int entity, int attachment );

void	R_ReadPointFile_f();
void	R_InitParticles();
void	R_ClearParticles();
//=========================================================
// particle stuff

void	R_DrawParticles();
int		ScreenTransform( vec_t* point, vec_t* screen );

void	RotatedBBox( vec_t* mins, vec_t* maxs, vec_t* angles, vec_t* rotatedMins, vec_t* rotatedMaxs );
void	AddTEntity( struct cl_entity_s* entity );
void	R_DrawTEntitiesOnList( qboolean clientOnly );

void	R_TriangleSetTexture( byte* texture, short width, short height, word* palette );

int		SignbitsForPlane( mplane_t* plane );
void	R_StudioVertBuffer();
void	R_StudioProjectFinalVert( finalvert_t* vertex, auxvert_t* auxiliaryVertex );
