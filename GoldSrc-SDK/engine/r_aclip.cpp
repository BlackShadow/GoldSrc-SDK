#include "r_local.h"

static finalvert_t fv[2][8];
static auxvert_t av[8];

void R_Alias_clip_z( finalvert_t* pfv0, finalvert_t* pfv1, finalvert_t* out )
{
	if ( pfv0->v[1] < pfv1->v[1] )
	{
		finalvert_t* temp = pfv0;
		pfv0 = pfv1;
		pfv1 = temp;
	}

	auxvert_t* pav0 = &av[pfv0 - fv[0]];
	auxvert_t* pav1 = &av[pfv1 - fv[0]];
	float scale = ( (float)ALIAS_Z_CLIP_PLANE - pav0->fv[2] ) / ( pav1->fv[2] - pav0->fv[2] );
	auxvert_t avout;
	avout.fv[0] = pav0->fv[0] + ( pav1->fv[0] - pav0->fv[0] ) * scale;
	avout.fv[1] = pav0->fv[1] + ( pav1->fv[1] - pav0->fv[1] ) * scale;
	avout.fv[2] = ALIAS_Z_CLIP_PLANE;
	for ( int i = 2; i < 5; i++ )
		out->v[i] = (int)( pfv0->v[i] + ( pfv1->v[i] - pfv0->v[i] ) * scale );
	R_AliasProjectFinalVert( out, &avout );
	if ( out->v[0] < r_refdef.aliasvrect.x )
		out->flags |= ALIAS_LEFT_CLIP;
	if ( out->v[1] < r_refdef.aliasvrect.y )
		out->flags |= ALIAS_TOP_CLIP;
	if ( out->v[0] > r_refdef.aliasvrectright )
		out->flags |= ALIAS_RIGHT_CLIP;
	if ( out->v[1] > r_refdef.aliasvrectbottom )
		out->flags |= ALIAS_BOTTOM_CLIP;
}

void R_Alias_clip_left( finalvert_t* pfv0, finalvert_t* pfv1, finalvert_t* out )
{
	if ( pfv0->v[1] < pfv1->v[1] )
	{
		finalvert_t* temp = pfv0;
		pfv0 = pfv1;
		pfv1 = temp;
	}

	float scale = (float)( r_refdef.aliasvrect.x - pfv0->v[0] ) / ( pfv1->v[0] - pfv0->v[0] );
	for ( int i = 0; i < 6; i++ )
		out->v[i] = (int)( pfv0->v[i] + ( pfv1->v[i] - pfv0->v[i] ) * scale + 0.5 );
}

void R_Alias_clip_right( finalvert_t* pfv0, finalvert_t* pfv1, finalvert_t* out )
{
	if ( pfv0->v[1] < pfv1->v[1] )
	{
		finalvert_t* temp = pfv0;
		pfv0 = pfv1;
		pfv1 = temp;
	}

	float scale = (float)( r_refdef.aliasvrectright - pfv0->v[0] ) / ( pfv1->v[0] - pfv0->v[0] );
	for ( int i = 0; i < 6; i++ )
		out->v[i] = (int)( pfv0->v[i] + ( pfv1->v[i] - pfv0->v[i] ) * scale + 0.5 );
}

void R_Alias_clip_top( finalvert_t* pfv0, finalvert_t* pfv1, finalvert_t* out )
{
	if ( pfv0->v[1] < pfv1->v[1] )
	{
		finalvert_t* temp = pfv0;
		pfv0 = pfv1;
		pfv1 = temp;
	}

	float scale = (float)( r_refdef.aliasvrect.y - pfv0->v[1] ) / ( pfv1->v[1] - pfv0->v[1] );
	for ( int i = 0; i < 6; i++ )
		out->v[i] = (int)( pfv0->v[i] + ( pfv1->v[i] - pfv0->v[i] ) * scale + 0.5 );
}

void R_Alias_clip_bottom( finalvert_t* pfv0, finalvert_t* pfv1, finalvert_t* out )
{
	if ( pfv0->v[1] < pfv1->v[1] )
	{
		finalvert_t* temp = pfv0;
		pfv0 = pfv1;
		pfv1 = temp;
	}

	float scale = (float)( r_refdef.aliasvrectbottom - pfv0->v[1] ) / ( pfv1->v[1] - pfv0->v[1] );
	for ( int i = 0; i < 6; i++ )
		out->v[i] = (int)( pfv0->v[i] + ( pfv1->v[i] - pfv0->v[i] ) * scale + 0.5 );
}

int R_AliasClip( finalvert_t* in, finalvert_t* out, int flag, int count,
	void ( *clip )( finalvert_t*, finalvert_t*, finalvert_t* ) )
{
	int outcount = 0;
	int previous = count - 1;
	for ( int i = 0; i < count; previous = i++ )
	{
		int flags = in[i].flags & flag;
		int prevflags = in[previous].flags & flag;
		if ( flags && prevflags )
			continue;
		if ( flags != prevflags )
		{
			finalvert_t* vert = &out[outcount++];
			clip( &in[previous], &in[i], vert );
			vert->flags = 0;
			if ( vert->v[0] < r_refdef.aliasvrect.x )
				vert->flags |= ALIAS_LEFT_CLIP;
			if ( vert->v[1] < r_refdef.aliasvrect.y )
				vert->flags |= ALIAS_TOP_CLIP;
			if ( vert->v[0] > r_refdef.aliasvrectright )
				vert->flags |= ALIAS_RIGHT_CLIP;
			if ( vert->v[1] > r_refdef.aliasvrectbottom )
				vert->flags |= ALIAS_BOTTOM_CLIP;
		}
		if ( !flags )
			out[outcount++] = in[i];
	}
	return outcount;
}

/*
================
R_AliasClipTriangle
================
*/
void R_AliasClipTriangle( mtriangle_t* ptri )
{
	for ( int i = 0; i < 3; i++ )
	{
		fv[0][i] = pfinalverts[ptri->vertindex[i]];
		if ( !ptri->facesfront && ( fv[0][i].flags & ALIAS_ONSEAM ) )
			fv[0][i].v[2] += r_affinetridesc.seamfixupX16;
	}
	int clipflags = fv[0][0].flags | fv[0][1].flags | fv[0][2].flags;
	int count = 3;
	int pingpong = 0;
	if ( clipflags & ALIAS_Z_CLIP )
	{
		for ( int i = 0; i < 3; i++ )
			av[i] = pauxverts[ptri->vertindex[i]];
		count = R_AliasClip( fv[0], fv[1], ALIAS_Z_CLIP, count, R_Alias_clip_z );
		if ( !count )
			return;
		pingpong = 1;
		clipflags = fv[1][0].flags | fv[1][1].flags | fv[1][2].flags;
	}

	if ( clipflags & ALIAS_LEFT_CLIP )
	{
		count = R_AliasClip( fv[pingpong], fv[pingpong ^ 1], ALIAS_LEFT_CLIP, count, R_Alias_clip_left );
		if ( !count )
			return;
		pingpong ^= 1;
	}

	if ( clipflags & ALIAS_RIGHT_CLIP )
	{
		count = R_AliasClip( fv[pingpong], fv[pingpong ^ 1], ALIAS_RIGHT_CLIP, count, R_Alias_clip_right );
		if ( !count )
			return;
		pingpong ^= 1;
	}

	if ( clipflags & ALIAS_BOTTOM_CLIP )
	{
		count = R_AliasClip( fv[pingpong], fv[pingpong ^ 1], ALIAS_BOTTOM_CLIP, count, R_Alias_clip_bottom );
		if ( !count )
			return;
		pingpong ^= 1;
	}

	if ( clipflags & ALIAS_TOP_CLIP )
	{
		count = R_AliasClip( fv[pingpong], fv[pingpong ^ 1], ALIAS_TOP_CLIP, count, R_Alias_clip_top );
		if ( !count )
			return;
		pingpong ^= 1;
	}

	finalvert_t* verts = fv[pingpong];
	for ( int i = 0; i < count; i++ )
	{
		if ( verts[i].v[0] < r_refdef.aliasvrect.x )
			verts[i].v[0] = r_refdef.aliasvrect.x;
		else if ( verts[i].v[0] > r_refdef.aliasvrectright )
			verts[i].v[0] = r_refdef.aliasvrectright;
		if ( verts[i].v[1] < r_refdef.aliasvrect.y )
			verts[i].v[1] = r_refdef.aliasvrect.y;
		else if ( verts[i].v[1] > r_refdef.aliasvrectbottom )
			verts[i].v[1] = r_refdef.aliasvrectbottom;
		verts[i].flags = 0;
	}

	mtriangle_t triangle;
	triangle.facesfront = ptri->facesfront;
	triangle.vertindex[0] = 0;
	r_affinetridesc.ptriangles = &triangle;
	r_affinetridesc.pfinalverts = verts;
	for ( int i = 1; i < count - 1; i++ )
	{
		triangle.vertindex[1] = i;
		triangle.vertindex[2] = i + 1;
		D_PolysetDraw();
	}
}
