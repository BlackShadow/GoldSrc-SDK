#include "mathlib.h"

#define BOX_GAP 0.0f

void CL_Particle( float* origin, int color, float life, int zpos, int zvel );

static int PM_boxpnt[6][4] = {
	{ 0, 4, 6, 2 },
	{ 0, 1, 5, 4 },
	{ 0, 2, 3, 1 },
	{ 7, 5, 1, 3 },
	{ 7, 3, 2, 6 },
	{ 7, 6, 4, 5 },
};

void PM_ParticleLine( vec_t* start, vec_t* end, int pcolor, float life, float vert )
{
	int i;
	float len, curdist;
	vec3_t diff, curpos;

	VectorSubtract( end, start, diff );
	len = VectorNormalize( diff );

	for ( curdist = 0; curdist <= len; curdist += 2.0f )
	{
		for ( i = 0; i < 3; i++ )
			curpos[i] = start[i] + curdist * diff[i];

		CL_Particle( curpos, pcolor, life, 0, (int)vert );
	}
}

void PM_DrawRectangle( vec_t* tl, vec_t* bl, vec_t* tr, vec_t* br, int pcolor, float life )
{
	PM_ParticleLine( tl, bl, pcolor, life, 0 );
	PM_ParticleLine( bl, br, pcolor, life, 0 );
	PM_ParticleLine( br, tr, pcolor, life, 0 );
	PM_ParticleLine( tr, tl, pcolor, life, 0 );
}

void PM_DrawBBox( vec_t* mins, vec_t* maxs, vec_t* origin, int pcolor, float life )
{
	int i, j;
	vec3_t p[8];

	for ( j = 0; j < 8; j++ )
	{
		for ( i = 0; i < 3; i++ )
			p[j][i] = origin[i] + ( ( j & ( 1 << i ) ) ? mins[i] - BOX_GAP : maxs[i] + BOX_GAP );
	}

	for ( j = 0; j < 6; j++ )
		PM_DrawRectangle( p[PM_boxpnt[j][1]], p[PM_boxpnt[j][0]], p[PM_boxpnt[j][2]], p[PM_boxpnt[j][3]], pcolor, life );
}
