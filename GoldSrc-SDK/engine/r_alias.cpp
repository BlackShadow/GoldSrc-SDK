#include "r_local.h"
#include "client.h"
#include "conprint.h"

affinetridesc_t r_affinetridesc;
int a_skinwidth;
mtriangle_t* ptriangles;
aliashdr_t* paliashdr;
// TODO: these probably will go away with optimized rasterization
mdl_t* pmdl;
finalvert_t* pfinalverts;
auxvert_t* pauxverts;
int r_amodels_drawn;

trivertx_t* r_apverts;
vec3_t r_plightvec;
int r_ambientlight;
float r_shadelight;
float ziscale;
float zishift;
int r_anumverts;
float aliastransform[3][4];

vec3_t r_avertexnormals[NUMVERTEXNORMALS] =
{
	{ -0.525731f, 0.000000f, 0.850651f },
	{ -0.442863f, 0.238856f, 0.864188f },
	{ -0.295242f, 0.000000f, 0.955423f },
	{ -0.309017f, 0.500000f, 0.809017f },
	{ -0.162460f, 0.262866f, 0.951056f },
	{ 0.000000f, 0.000000f, 1.000000f },
	{ 0.000000f, 0.850651f, 0.525731f },
	{ -0.147621f, 0.716567f, 0.681718f },
	{ 0.147621f, 0.716567f, 0.681718f },
	{ 0.000000f, 0.525731f, 0.850651f },
	{ 0.309017f, 0.500000f, 0.809017f },
	{ 0.525731f, 0.000000f, 0.850651f },
	{ 0.295242f, 0.000000f, 0.955423f },
	{ 0.442863f, 0.238856f, 0.864188f },
	{ 0.162460f, 0.262866f, 0.951056f },
	{ -0.681718f, 0.147621f, 0.716567f },
	{ -0.809017f, 0.309017f, 0.500000f },
	{ -0.587785f, 0.425325f, 0.688191f },
	{ -0.850651f, 0.525731f, 0.000000f },
	{ -0.864188f, 0.442863f, 0.238856f },
	{ -0.716567f, 0.681718f, 0.147621f },
	{ -0.688191f, 0.587785f, 0.425325f },
	{ -0.500000f, 0.809017f, 0.309017f },
	{ -0.238856f, 0.864188f, 0.442863f },
	{ -0.425325f, 0.688191f, 0.587785f },
	{ -0.716567f, 0.681718f, -0.147621f },
	{ -0.500000f, 0.809017f, -0.309017f },
	{ -0.525731f, 0.850651f, 0.000000f },
	{ 0.000000f, 0.850651f, -0.525731f },
	{ -0.238856f, 0.864188f, -0.442863f },
	{ 0.000000f, 0.955423f, -0.295242f },
	{ -0.262866f, 0.951056f, -0.162460f },
	{ 0.000000f, 1.000000f, 0.000000f },
	{ 0.000000f, 0.955423f, 0.295242f },
	{ -0.262866f, 0.951056f, 0.162460f },
	{ 0.238856f, 0.864188f, 0.442863f },
	{ 0.262866f, 0.951056f, 0.162460f },
	{ 0.500000f, 0.809017f, 0.309017f },
	{ 0.238856f, 0.864188f, -0.442863f },
	{ 0.262866f, 0.951056f, -0.162460f },
	{ 0.500000f, 0.809017f, -0.309017f },
	{ 0.850651f, 0.525731f, 0.000000f },
	{ 0.716567f, 0.681718f, 0.147621f },
	{ 0.716567f, 0.681718f, -0.147621f },
	{ 0.525731f, 0.850651f, 0.000000f },
	{ 0.425325f, 0.688191f, 0.587785f },
	{ 0.864188f, 0.442863f, 0.238856f },
	{ 0.688191f, 0.587785f, 0.425325f },
	{ 0.809017f, 0.309017f, 0.500000f },
	{ 0.681718f, 0.147621f, 0.716567f },
	{ 0.587785f, 0.425325f, 0.688191f },
	{ 0.955423f, 0.295242f, 0.000000f },
	{ 1.000000f, 0.000000f, 0.000000f },
	{ 0.951056f, 0.162460f, 0.262866f },
	{ 0.850651f, -0.525731f, 0.000000f },
	{ 0.955423f, -0.295242f, 0.000000f },
	{ 0.864188f, -0.442863f, 0.238856f },
	{ 0.951056f, -0.162460f, 0.262866f },
	{ 0.809017f, -0.309017f, 0.500000f },
	{ 0.681718f, -0.147621f, 0.716567f },
	{ 0.850651f, 0.000000f, 0.525731f },
	{ 0.864188f, 0.442863f, -0.238856f },
	{ 0.809017f, 0.309017f, -0.500000f },
	{ 0.951056f, 0.162460f, -0.262866f },
	{ 0.525731f, 0.000000f, -0.850651f },
	{ 0.681718f, 0.147621f, -0.716567f },
	{ 0.681718f, -0.147621f, -0.716567f },
	{ 0.850651f, 0.000000f, -0.525731f },
	{ 0.809017f, -0.309017f, -0.500000f },
	{ 0.864188f, -0.442863f, -0.238856f },
	{ 0.951056f, -0.162460f, -0.262866f },
	{ 0.147621f, 0.716567f, -0.681718f },
	{ 0.309017f, 0.500000f, -0.809017f },
	{ 0.425325f, 0.688191f, -0.587785f },
	{ 0.442863f, 0.238856f, -0.864188f },
	{ 0.587785f, 0.425325f, -0.688191f },
	{ 0.688191f, 0.587785f, -0.425325f },
	{ -0.147621f, 0.716567f, -0.681718f },
	{ -0.309017f, 0.500000f, -0.809017f },
	{ 0.000000f, 0.525731f, -0.850651f },
	{ -0.525731f, 0.000000f, -0.850651f },
	{ -0.442863f, 0.238856f, -0.864188f },
	{ -0.295242f, 0.000000f, -0.955423f },
	{ -0.162460f, 0.262866f, -0.951056f },
	{ 0.000000f, 0.000000f, -1.000000f },
	{ 0.295242f, 0.000000f, -0.955423f },
	{ 0.162460f, 0.262866f, -0.951056f },
	{ -0.442863f, -0.238856f, -0.864188f },
	{ -0.309017f, -0.500000f, -0.809017f },
	{ -0.162460f, -0.262866f, -0.951056f },
	{ 0.000000f, -0.850651f, -0.525731f },
	{ -0.147621f, -0.716567f, -0.681718f },
	{ 0.147621f, -0.716567f, -0.681718f },
	{ 0.000000f, -0.525731f, -0.850651f },
	{ 0.309017f, -0.500000f, -0.809017f },
	{ 0.442863f, -0.238856f, -0.864188f },
	{ 0.162460f, -0.262866f, -0.951056f },
	{ 0.238856f, -0.864188f, -0.442863f },
	{ 0.500000f, -0.809017f, -0.309017f },
	{ 0.425325f, -0.688191f, -0.587785f },
	{ 0.716567f, -0.681718f, -0.147621f },
	{ 0.688191f, -0.587785f, -0.425325f },
	{ 0.587785f, -0.425325f, -0.688191f },
	{ 0.000000f, -0.955423f, -0.295242f },
	{ 0.000000f, -1.000000f, 0.000000f },
	{ 0.262866f, -0.951056f, -0.162460f },
	{ 0.000000f, -0.850651f, 0.525731f },
	{ 0.000000f, -0.955423f, 0.295242f },
	{ 0.238856f, -0.864188f, 0.442863f },
	{ 0.262866f, -0.951056f, 0.162460f },
	{ 0.500000f, -0.809017f, 0.309017f },
	{ 0.716567f, -0.681718f, 0.147621f },
	{ 0.525731f, -0.850651f, 0.000000f },
	{ -0.238856f, -0.864188f, -0.442863f },
	{ -0.500000f, -0.809017f, -0.309017f },
	{ -0.262866f, -0.951056f, -0.162460f },
	{ -0.850651f, -0.525731f, 0.000000f },
	{ -0.716567f, -0.681718f, -0.147621f },
	{ -0.716567f, -0.681718f, 0.147621f },
	{ -0.525731f, -0.850651f, 0.000000f },
	{ -0.500000f, -0.809017f, 0.309017f },
	{ -0.238856f, -0.864188f, 0.442863f },
	{ -0.262866f, -0.951056f, 0.162460f },
	{ -0.864188f, -0.442863f, 0.238856f },
	{ -0.809017f, -0.309017f, 0.500000f },
	{ -0.688191f, -0.587785f, 0.425325f },
	{ -0.681718f, -0.147621f, 0.716567f },
	{ -0.442863f, -0.238856f, 0.864188f },
	{ -0.587785f, -0.425325f, 0.688191f },
	{ -0.309017f, -0.500000f, 0.809017f },
	{ -0.147621f, -0.716567f, 0.681718f },
	{ -0.425325f, -0.688191f, 0.587785f },
	{ -0.162460f, -0.262866f, 0.951056f },
	{ 0.442863f, -0.238856f, 0.864188f },
	{ 0.162460f, -0.262866f, 0.951056f },
	{ 0.309017f, -0.500000f, 0.809017f },
	{ 0.147621f, -0.716567f, 0.681718f },
	{ 0.000000f, -0.525731f, 0.850651f },
	{ 0.425325f, -0.688191f, 0.587785f },
	{ 0.587785f, -0.425325f, 0.688191f },
	{ 0.688191f, -0.587785f, 0.425325f },
	{ -0.955423f, 0.295242f, 0.000000f },
	{ -0.951056f, 0.162460f, 0.262866f },
	{ -1.000000f, 0.000000f, 0.000000f },
	{ -0.850651f, 0.000000f, 0.525731f },
	{ -0.955423f, -0.295242f, 0.000000f },
	{ -0.951056f, -0.162460f, 0.262866f },
	{ -0.864188f, 0.442863f, -0.238856f },
	{ -0.951056f, 0.162460f, -0.262866f },
	{ -0.809017f, 0.309017f, -0.500000f },
	{ -0.864188f, -0.442863f, -0.238856f },
	{ -0.951056f, -0.162460f, -0.262866f },
	{ -0.809017f, -0.309017f, -0.500000f },
	{ -0.681718f, 0.147621f, -0.716567f },
	{ -0.681718f, -0.147621f, -0.716567f },
	{ -0.850651f, 0.000000f, -0.525731f },
	{ -0.688191f, 0.587785f, -0.425325f },
	{ -0.587785f, 0.425325f, -0.688191f },
	{ -0.425325f, 0.688191f, -0.587785f },
	{ -0.425325f, -0.688191f, -0.587785f },
	{ -0.587785f, -0.425325f, -0.688191f },
	{ -0.688191f, -0.587785f, -0.425325f }
};

/*
================
R_AliasTransformAndProjectFinalVerts
================
*/
void R_AliasTransformAndProjectFinalVerts( finalvert_t* fv, stvert_t* pstverts )
{
	trivertx_t* verts = r_apverts;

	for ( int i = 0; i < r_anumverts; i++, fv++, pstverts++, verts++ )
	{
		double zi = 1.0 / ( verts->v[0] * (double)aliastransform[2][0]
			+ verts->v[1] * (double)aliastransform[2][1]
			+ verts->v[2] * (double)aliastransform[2][2] + aliastransform[2][3] );
		float depth = (float)zi;
		fv->v[5] = (int)depth;

		float u = (float)( ( verts->v[0] * (double)aliastransform[0][0]
			+ verts->v[1] * (double)aliastransform[0][1]
			+ verts->v[2] * (double)aliastransform[0][2] + aliastransform[0][3] ) * zi + aliasxcenter );
		fv->v[0] = (int)u;
		float v = (float)( zi * ( verts->v[0] * (double)aliastransform[1][0]
			+ verts->v[1] * (double)aliastransform[1][1]
			+ verts->v[2] * (double)aliastransform[1][2] + aliastransform[1][3] ) + aliasycenter );
		fv->v[1] = (int)v;
		fv->v[2] = pstverts->s;
		fv->v[3] = pstverts->t;
		fv->flags = pstverts->onseam;

		int light = r_ambientlight;
		vec_t* normal = r_avertexnormals[verts->lightnormalindex];
		double lightcos = normal[0] * (double)r_plightvec[0]
			+ normal[1] * (double)r_plightvec[1] + normal[2] * (double)r_plightvec[2];
		if ( lightcos < 0 )
		{
			float shade = (float)( lightcos * r_shadelight );
			light += (int)shade;
			// clamp; because we limited the minimum ambient and shading light, we
			// don't have to clamp low light, just bright
			if ( light < 0 )
				light = 0;
		}
		fv->v[4] = light;
	}
}

/*
================
R_AliasProjectFinalVert
================
*/
void R_AliasProjectFinalVert( finalvert_t* fv, auxvert_t* av )
{
	float zi = (float)( 1.0 / av->fv[2] );
	fv->v[5] = (int)( zi * ziscale );
	fv->v[0] = (int)( av->fv[0] * aliasxscale * zi + aliasxcenter );
	fv->v[1] = (int)( av->fv[1] * aliasyscale * zi + aliasycenter );
}

void xR_AliasSetupFrame()
{
	static trivertx_t iterpverts[2000];
	int frame = (int)currententity->curstate.frame;

	if ( frame >= pmdl->numframes || frame < 0 )
	{
		Con_DPrintf( "R_AliasSetupFrame: no such frame %d\n", frame );
		frame = 0;
	}

	maliasframedesc_t* pframe = &paliashdr->frames[frame];
	if ( pframe->type == ALIAS_SINGLE )
	{
		if ( currententity->prevstate.frame != (float)frame )
		{
			currententity->latched.prevanimtime = (float)cl.time;
			currententity->prevstate.frame = (float)frame;
		}

		if ( currententity == &cl.viewent || !r_dointerp || pmdl->numframes <= currententity->prevstate.frame )
		{
			r_apverts = (trivertx_t*)( (byte*)paliashdr + pframe->frame );
			return;
		}

		float time = (float)cl.time;
		float fraction = (float)( time * 10.0 - 10.0 * currententity->latched.prevanimtime );
		double blend = fraction;
		double oldblend;
		if ( fraction > 1 || fraction < 0 )
		{
			blend = 1;
			oldblend = 0;
		}
		else
		{
			oldblend = 1.0 - fraction;
		}

		int oldframe = (int)currententity->prevstate.frame;
		trivertx_t* oldverts = (trivertx_t*)( (byte*)paliashdr + paliashdr->frames[oldframe].frame );
		trivertx_t* verts = (trivertx_t*)( (byte*)paliashdr + pframe->frame );
		for ( int i = 0; i < pmdl->numverts; i++ )
		{
			for ( int j = 0; j < 3; j++ )
			{
				float position = (float)( oldverts[i].v[j] * oldblend + verts[i].v[j] * blend );
				iterpverts[i].v[j] = (byte)(int)position;
			}
			iterpverts[i].lightnormalindex = verts[i].lightnormalindex;
		}
		r_apverts = iterpverts;
		return;
	}

	maliasgroup_t* group = (maliasgroup_t*)( (byte*)paliashdr + pframe->frame );
	float* intervals = (float*)( (byte*)paliashdr + group->intervals );
	float fullinterval = intervals[group->numframes - 1];
	float time = (float)( currententity->syncbase + cl.time );
	float cycle = time / fullinterval;
	double targettime = time - fullinterval * (double)(int)cycle;

	int i;
	for ( i = 0; i < group->numframes - 1; i++ )
	{
		if ( intervals[i] > targettime )
			break;
	}
	r_apverts = (trivertx_t*)( (byte*)paliashdr + group->frames[i].frame );
}
