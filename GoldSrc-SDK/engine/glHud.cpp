#include "glquake.h"
#include "mathlib.h"
#include "vgui_intwrap2.h"

#include <cmath>

extern int glx, gly, glwidth, glheight;

void GLBeginHud()
{
	qglViewport( glx, gly, glwidth, glheight );
	qglMatrixMode( GL_PROJECTION );
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho( 0, glwidth, glheight, 0, -99999, 99999 );

	qglMatrixMode( GL_MODELVIEW );
	qglPushMatrix();
	qglLoadIdentity();

	qglDisable( GL_DEPTH_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );
	qglEnable( GL_ALPHA_TEST );

	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	VGUI2_ResetCurrentTexture();
}

void DrawWedge( float centerx, float centery, float angle1, float angle2, float radius )
{
	float a1, a2;

	a1 = (float)( angle1 * ( M_PI / 180.0 ) );
	a2 = (float)( angle2 * ( M_PI / 180.0 ) );

	qglBegin( GL_TRIANGLES );
	qglVertex2f( centerx, centery );
	qglVertex2f( (float)( centerx - cos( (double)a1 ) * radius ), (float)( centery - sin( (double)a1 ) * radius ) );
	qglVertex2f( (float)( centerx - cos( (double)a2 ) * radius ), (float)( centery - sin( (double)a2 ) * radius ) );
	qglEnd();
}

void GLFinishHud()
{
	qglMatrixMode( GL_PROJECTION );
	qglPopMatrix();

	qglMatrixMode( GL_MODELVIEW );
	qglPopMatrix();

	qglEnable( GL_DEPTH_TEST );
	qglEnable( GL_CULL_FACE );
	qglEnable( GL_BLEND );
}
