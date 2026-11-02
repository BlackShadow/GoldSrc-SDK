#include "stdafx.h"
#include <mmsystem.h>
#include "render3dgl.h"
#include "camera.h"
#include "mapdoc.h"
#include "mapworld.h"
#include "mapclass.h"
#include "mapatom.h"
#include "IEditorTexture.h"
#include "toolmanager.h"
#include "toolinterface.h"

#include <gl/GLU.h>
#include <algorithm>

static int g_nRenderFrame;
static int g_nTextureID;

CRender3DGL::CRender3DGL()
	: m_hWnd( NULL ), m_pDoc( NULL ), m_pCamera( NULL ), m_eRenderMode( RENDER_MODE_NONE ),
	  m_eDefaultRenderMode( RENDER_MODE_WIREFRAME ), m_eProjectionMode( PROJECTION_PERSPECTIVE ), m_nWidth( 1 ),
	  m_nHeight( 1 ), m_fAspect( 1 ), m_nRenderFrame( 0 ), m_dwLastFrame( 0 ), m_flElapsedTime( 0 ),
	  m_bPicking( false ), m_bDrawFPS( false ), m_flPickX( 0 ), m_flPickY( 0 ), m_flPickWidth( 0 ),
	  m_flPickHeight( 0 ), m_pHitObject( NULL ), m_nHitHandle( 0 ), m_SelectionBuffer()
{
	memset( m_RenderState, 0, sizeof( m_RenderState ) );
}

CRender3DGL::~CRender3DGL()
{
	ShutDown();
}

bool CRender3DGL::Initialize( HWND hWnd, CMapDoc* pDoc )
{
	if ( !m_Context.Initialize( hWnd ) )
		return false;

	const unsigned char white[] = { 255, 255, 255, 255 };
	glBindTexture( GL_TEXTURE_2D, 0 );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white );

	m_hWnd = hWnd;
	m_pDoc = pDoc;
	RECT client;
	GetClientRect( hWnd, &client );
	SetSize( client.right - client.left, client.bottom - client.top );

	CMapWorld* pWorld = pDoc ? pDoc->GetMapWorld() : NULL;
	if ( pWorld )
		pWorld->RenderPreload( this, true );
	return true;
}

void CRender3DGL::ShutDown()
{
	UncacheAllTextures();
	m_Context.ShutDown();
	m_hWnd = NULL;
	m_pDoc = NULL;
	m_pCamera = NULL;
}

void CRender3DGL::BindTexture( IEditorTexture* pTexture )
{
	if ( !pTexture || pTexture->IsDummy() || !pTexture->Load() )
	{
		glBindTexture( GL_TEXTURE_2D, 0 );
		return;
	}

	for ( size_t i = 0; i < m_TextureCache.size(); i++ )
	{
		texturecache_t& entry = m_TextureCache[i];
		if ( entry.pTexture == pTexture && entry.textureID == pTexture->GetTextureID() )
		{
			glBindTexture( GL_TEXTURE_2D, entry.texture );
			bool filtered = IsEnabled( RENDER_FILTER_TEXTURES );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST );
			return;
		}
	}

	if ( m_TextureCache.size() >= TEXTURE_CACHE_SIZE )
	{
		glBindTexture( GL_TEXTURE_2D, 0 );
		return;
	}

	bool alpha = pTexture->HasAlpha();
	int	 size = alpha ? pTexture->GetImageDataRGBA() : pTexture->GetImageDataRGB();
	if ( size <= 0 )
	{
		glBindTexture( GL_TEXTURE_2D, 0 );
		return;
	}

	std::vector<unsigned char> pixels( size );
	if ( alpha )
		pTexture->GetImageDataRGBA( &pixels[0] );
	else
		pTexture->GetImageDataRGB( &pixels[0] );

	GLuint texture = 0;
	glGenTextures( 1, &texture );
	if ( !texture )
		return;

	glBindTexture( GL_TEXTURE_2D, texture );
	bool filtered = IsEnabled( RENDER_FILTER_TEXTURES );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	GLint alignment;
	glGetIntegerv( GL_UNPACK_ALIGNMENT, &alignment );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	int error = gluBuild2DMipmaps( GL_TEXTURE_2D, alpha ? 4 : 3, pTexture->GetImageWidth(), pTexture->GetImageHeight(),
								   alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, &pixels[0] );
	glPixelStorei( GL_UNPACK_ALIGNMENT, alignment );
	if ( error )
	{
		glDeleteTextures( 1, &texture );
		return;
	}

	if ( pTexture->GetTextureID() == TEXTURE_ID_NONE )
		pTexture->SetTextureID( g_nTextureID++ );
	texturecache_t entry = { pTexture, pTexture->GetTextureID(), texture };
	m_TextureCache.push_back( entry );
}

void CRender3DGL::UncacheAllTextures()
{
	if ( !m_TextureCache.empty() && m_Context.MakeCurrent() )
		for ( size_t i = 0; i < m_TextureCache.size(); i++ )
			glDeleteTextures( 1, &m_TextureCache[i].texture );
	m_TextureCache.clear();
}

float CRender3DGL::GetGridSize()
{
	return m_pDoc ? (float)m_pDoc->GetGridSpacing() : 1;
}

float CRender3DGL::GetGridDistance()
{
	return min( max( GetGridSize() * 10, 64.0f ), 2048.0f );
}

void CRender3DGL::GetViewPoint( Vector& point )
{
	m_pCamera->GetViewPoint( point );
}

void CRender3DGL::GetViewForward( Vector& forward )
{
	m_pCamera->GetViewForward( forward );
}

void CRender3DGL::GetViewUp( Vector& up )
{
	m_pCamera->GetViewUp( up );
}

void CRender3DGL::GetViewRight( Vector& right )
{
	m_pCamera->GetViewRight( right );
}

bool CRender3DGL::IsEnabled( RenderState_t state )
{
	return state >= RENDER_CENTER_CROSSHAIR && state <= RENDER_REVERSE_SELECTION && m_RenderState[state];
}

float CRender3DGL::LightPlane( Vector& normal )
{
	static Vector light( 1, 2, 3 );
	VectorNormalize( light );
	double shade = ( light.z * normal.z + light.y * normal.y + light.x * normal.x ) * 0.5 + 0.8;
	return (float)min( shade, 1.0 );
}

void CRender3DGL::SetDefaultRenderMode( RenderMode_t mode )
{
	m_eDefaultRenderMode = mode;
	SetRenderMode( mode );
}

void CRender3DGL::SetRenderMode( RenderMode_t mode, bool force )
{
	if ( mode == RENDER_MODE_DEFAULT )
		mode = m_eDefaultRenderMode;
	if ( mode == m_eRenderMode && !force )
		return;

	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glDepthMask( GL_TRUE );
	glDisable( GL_BLEND );
	glDisable( GL_ALPHA_TEST );
	glFrontFace( GL_CW );
	glPolygonMode( GL_FRONT_AND_BACK, mode == RENDER_MODE_WIREFRAME ? GL_LINE : GL_FILL );
	if ( mode == RENDER_MODE_WIREFRAME )
		glDisable( GL_CULL_FACE );
	else
		glEnable( GL_CULL_FACE );
	if ( mode == RENDER_MODE_TEXTURED )
		glEnable( GL_TEXTURE_2D );
	else
		glDisable( GL_TEXTURE_2D );
	if ( mode == RENDER_MODE_TRANSLUCENT_FLAT || mode == RENDER_MODE_SELECTION_OVERLAY )
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDepthMask( GL_FALSE );
	}
	m_eRenderMode = mode;
}

bool CRender3DGL::SetSize( int width, int height )
{
	m_nWidth = max( width, 1 );
	m_nHeight = max( height, 1 );
	m_fAspect = (float)( (double)m_nWidth / (double)m_nHeight );
	if ( !m_Context.MakeCurrent() )
		return false;
	glViewport( 0, 0, m_nWidth, m_nHeight );
	return true;
}

void CRender3DGL::SetupCamera()
{
	if ( m_pCamera == NULL )
		return;

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	if ( m_bPicking )
	{
		GLint viewport[4];
		glGetIntegerv( GL_VIEWPORT, viewport );
		gluPickMatrix( m_flPickX, m_flPickY, m_flPickWidth, m_flPickHeight, viewport );
		glSelectBuffer( SELECTION_BUFFER_SIZE, m_SelectionBuffer );
		glRenderMode( GL_SELECT );
		glInitNames();
	}

	if ( m_eProjectionMode == PROJECTION_PERSPECTIVE )
	{
		float fov = m_pCamera->GetHorizontalFOV();
		float fWidth = (float)m_nWidth;
		float fHeight = (float)m_nHeight;
		if ( fov < 1 || fov > 179 )
			fov = 90;
		double verticalFOV =
			atan2( fHeight / ( fWidth / tan( (double)fov * ( 1.0f / 360.0f ) * M_PI ) ), 1.0 ) * 360.0f * ( 1.0 / M_PI );
		gluPerspective( verticalFOV, m_fAspect, m_pCamera->GetNearClip(), m_pCamera->GetFarClip() );
	}
	else
	{
		double zoom = m_pCamera->GetZoom();
		glOrtho( -m_nWidth / ( 2 * zoom ), m_nWidth / ( 2 * zoom ), -m_nHeight / ( 2 * zoom ), m_nHeight / ( 2 * zoom ),
				 m_pCamera->GetNearClip(), m_pCamera->GetFarClip() );
	}

	matrix4_t view;
	m_pCamera->GetViewMatrix( view );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glMultMatrixf( view.Base() );
}

void CRender3DGL::PreRender()
{
	m_Context.MakeCurrent();
	m_nRenderFrame = ++g_nRenderFrame;
	DWORD now = timeGetTime();
	m_flElapsedTime = (float)( (double)( now - m_dwLastFrame ) * 0.001 );
	m_dwLastFrame = now;
	m_RenderLast.clear();
	SetupCamera();
	if ( !m_bPicking )
		RenderClear();
	SetRenderMode( m_eDefaultRenderMode, true );
}

void CRender3DGL::PostRender()
{
	if ( m_bPicking )
	{
		glFlush();
		return;
	}
	m_Context.Swap();
	if ( m_bDrawFPS )
	{
		char text[128];
		sprintf( text, "FPS=%3.2f, y=%.2f, p=%.2f, r=%.2f", m_flElapsedTime > 0 ? 1.0 / m_flElapsedTime : 0,
				 m_pCamera->GetYaw(), m_pCamera->GetPitch(), m_pCamera->GetRoll() );
		HDC hDC = GetDC( m_hWnd );
		TextOut( hDC, 10, 10, text, (int)strlen( text ) );
		ReleaseDC( m_hWnd, hDC );
	}
}

void CRender3DGL::RenderMapClass( CMapClass* pObject )
{
	if ( !pObject || pObject->GetRenderFrame() == m_nRenderFrame )
		return;
	if ( !pObject->IsVisible() )
		return;

	if ( pObject->ShouldRenderLast() )
		m_RenderLast.push_back( pObject );
	else
		pObject->Render3D( this );
	POSITION   pos;
	CMapClass* pChild = pObject->GetFirstChild( pos );
	while ( pChild )
	{
		RenderMapClass( pChild );
		pChild = pObject->GetNextChild( pos );
	}
	pObject->SetRenderFrame( m_nRenderFrame );
}

void CRender3DGL::Render()
{
	if ( !m_pCamera || !m_pDoc || !m_Context.MakeCurrent() )
		return;
	PreRender();
	RenderLine( Vector( 0, 0, 0 ), Vector( 100, 0, 0 ), 255, 0, 0 );
	RenderLine( Vector( 0, 0, 0 ), Vector( 0, 100, 0 ), 0, 255, 0 );
	RenderLine( Vector( 0, 0, 0 ), Vector( 0, 0, 100 ), 0, 0, 255 );
	SetRenderMode( RENDER_MODE_DEFAULT );
	RenderMapClass( m_pDoc->GetMapWorld() );

	for ( int i = 0; i < g_pToolManager->GetToolCount(); i++ )
		g_pToolManager->GetTool( i )->RenderTool3D( this );
	for ( size_t i = 0; i < m_RenderLast.size(); i++ )
		m_RenderLast[i]->Render3D( this );

	if ( m_pDoc->m_nPFPoints > 1 )
		for ( int i = 1; i < m_pDoc->m_nPFPoints; i++ )
			RenderLine( m_pDoc->m_pPFPoints[i - 1], m_pDoc->m_pPFPoints[i], 255, 0, 0 );
	if ( !m_bPicking && ( IsEnabled( RENDER_CENTER_CROSSHAIR ) || IsEnabled( RENDER_FRAME_RECT ) ) )
	{
		BeginParallel();
		if ( IsEnabled( RENDER_CENTER_CROSSHAIR ) )
		{
			int x = m_nWidth / 2;
			int y = m_nHeight / 2;
			glBegin( GL_LINES );
			glColor3ub( 0, 0, 0 );
			glVertex2i( x - 5, y - 1 );
			glVertex2i( x + 6, y - 1 );
			glVertex2i( x - 5, y + 1 );
			glVertex2i( x + 6, y + 1 );
			glVertex2i( x - 1, y - 6 );
			glVertex2i( x - 1, y + 6 );
			glVertex2i( x + 1, y - 6 );
			glVertex2i( x + 1, y + 6 );
			glColor3ub( 255, 255, 255 );
			glVertex2i( x - 5, y );
			glVertex2i( x + 6, y );
			glVertex2i( x, y - 6 );
			glVertex2i( x, y + 6 );
			glEnd();
		}
		if ( IsEnabled( RENDER_FRAME_RECT ) )
		{
			glBegin( GL_LINE_LOOP );
			glColor3ub( 255, 0, 0 );
			glVertex2i( 1, 0 );
			glVertex2i( m_nWidth, 0 );
			glVertex2i( m_nWidth, m_nHeight - 1 );
			glVertex2i( 1, m_nHeight - 1 );
			glEnd();
		}
		EndParallel();
	}
	PostRender();
}

void CRender3DGL::RenderClear()
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void CRender3DGL::RenderEnable( RenderState_t state, bool enable )
{
	if ( state < RENDER_CENTER_CROSSHAIR || state > RENDER_REVERSE_SELECTION )
		return;
	m_RenderState[state] = enable;
	if ( state == RENDER_POLYGON_OFFSET_FILL || state == RENDER_POLYGON_OFFSET_LINE )
	{
		GLenum cap = state == RENDER_POLYGON_OFFSET_FILL ? GL_POLYGON_OFFSET_FILL : GL_POLYGON_OFFSET_LINE;
		if ( enable )
		{
			glEnable( cap );
			glPolygonOffset( -1, -1 );
		}
		else
		{
			glDisable( cap );
		}
	}
}

void CRender3DGL::RenderLine( const Vector& start, const Vector& end, unsigned char red, unsigned char green,
							  unsigned char blue )
{
	glBindTexture( GL_TEXTURE_2D, 0 );
	glColor3ub( red, green, blue );
	glBegin( GL_LINES );
	glVertex3fv( start.Base() );
	glVertex3fv( end.Base() );
	glEnd();
}

void CRender3DGL::RenderWireframeBox( const Vector& mins, const Vector& maxs, unsigned char red, unsigned char green,
									  unsigned char blue )
{
	glBindTexture( GL_TEXTURE_2D, 0 );
	glColor3ub( red, green, blue );
	glBegin( GL_LINE_STRIP );
	glVertex3f( mins.x, mins.y, mins.z );
	glVertex3f( maxs.x, mins.y, mins.z );
	glVertex3f( maxs.x, maxs.y, mins.z );
	glVertex3f( mins.x, maxs.y, mins.z );
	glVertex3f( mins.x, mins.y, mins.z );
	glVertex3f( mins.x, mins.y, maxs.z );
	glVertex3f( maxs.x, mins.y, maxs.z );
	glVertex3f( maxs.x, maxs.y, maxs.z );
	glVertex3f( mins.x, maxs.y, maxs.z );
	glVertex3f( mins.x, mins.y, maxs.z );
	glEnd();
	glBegin( GL_LINES );
	glVertex3f( maxs.x, mins.y, mins.z );
	glVertex3f( maxs.x, mins.y, maxs.z );
	glVertex3f( maxs.x, maxs.y, mins.z );
	glVertex3f( maxs.x, maxs.y, maxs.z );
	glVertex3f( mins.x, maxs.y, mins.z );
	glVertex3f( mins.x, maxs.y, maxs.z );
	glEnd();
}

void CRender3DGL::RenderBox( const Vector& mins, const Vector& maxs, unsigned char red, unsigned char green,
							 unsigned char blue, SelectionState_t selected )
{
	RenderMode_t mode = m_eRenderMode;
	if ( mode == RENDER_MODE_WIREFRAME )
	{
		RenderWireframeBox( mins, maxs, red, green, blue );
		return;
	}

	Vector points[8];
	for ( int i = 0; i < 8; i++ )
		points[i].Init( ( i & 1 ) ? maxs.x : mins.x, ( i & 2 ) ? maxs.y : mins.y, ( i & 4 ) ? maxs.z : mins.z );
	int faces[6][4] = { { 0, 1, 3, 2 }, { 4, 6, 7, 5 }, { 0, 4, 5, 1 },
						{ 2, 3, 7, 6 }, { 0, 2, 6, 4 }, { 1, 5, 7, 3 } };
	SetRenderMode( RENDER_MODE_FLAT );
	for ( int i = 0; i < 6; i++ )
	{
		Vector normal =
			( points[faces[i][2]] - points[faces[i][0]] ).Cross( points[faces[i][1]] - points[faces[i][0]] );
		VectorNormalize( normal );
		float shade = LightPlane( normal );
		glColor3ub( (unsigned char)( red * shade ), (unsigned char)( green * shade ), (unsigned char)( blue * shade ) );
		glBegin( GL_QUADS );
		for ( int j = 0; j < 4; j++ )
			glVertex3fv( points[faces[i][j]].Base() );
		glEnd();
	}
	if ( selected != SELECT_NONE )
	{
		SetRenderMode( RENDER_MODE_WIREFRAME );
		RenderEnable( RENDER_POLYGON_OFFSET_LINE, true );
		RenderWireframeBox( mins, maxs, SELECT_EDGE_RED, SELECT_EDGE_GREEN, SELECT_EDGE_BLUE );
		RenderEnable( RENDER_POLYGON_OFFSET_LINE, false );
	}
	SetRenderMode( mode );
}

void CRender3DGL::RenderArrow( const Vector& start, const Vector& end, unsigned char red, unsigned char green,
							   unsigned char blue )
{
	RenderLine( start, end, red, green, blue );
	Vector axis = end - start;
	float  length = VectorNormalize( axis );
	float  tipLength = length * 0.125f;
	RenderCone( start + axis * ( length - tipLength ), end, tipLength * 0.333f, 6, red, green, blue );
}

void CRender3DGL::RenderCone( const Vector& base, const Vector& tip, float radius, int slices, unsigned char red,
							  unsigned char green, unsigned char blue )
{
	if ( slices < 3 )
		return;
	Vector axis = tip - base;
	VectorNormalize( axis );
	Vector reference = fabs( axis.z ) < 0.9 ? Vector( 0, 0, 1 ) : Vector( 1, 0, 0 );
	Vector right = axis.Cross( reference );
	VectorNormalize( right );
	Vector up = right.Cross( axis );
	SetRenderMode( RENDER_MODE_FLAT );
	glColor3ub( red, green, blue );
	glBegin( GL_TRIANGLE_FAN );
	glVertex3fv( tip.Base() );
	for ( int i = 0; i <= slices; i++ )
	{
		double angle = i * 2 * M_PI / slices;
		Vector point = base + right * (float)( cos( angle ) * radius ) + up * (float)( sin( angle ) * radius );
		glVertex3fv( point.Base() );
	}
	glEnd();
}

void CRender3DGL::RenderSphere( const Vector& center, float radius, int theta, int phi, unsigned char red,
								unsigned char green, unsigned char blue )
{
	GLUquadric* pQuadric = gluNewQuadric();
	if ( !pQuadric )
		return;
	SetRenderMode( RENDER_MODE_FLAT );
	glColor3ub( red, green, blue );
	glPushMatrix();
	glTranslatef( center.x, center.y, center.z );
	gluSphere( pQuadric, radius, theta, phi );
	glPopMatrix();
	gluDeleteQuadric( pQuadric );
}

void CRender3DGL::RenderWireframeSphere( const Vector& center, float radius, int theta, int phi, unsigned char red,
										 unsigned char green, unsigned char blue )
{
	GLUquadric* pQuadric = gluNewQuadric();
	if ( !pQuadric )
		return;
	SetRenderMode( RENDER_MODE_WIREFRAME );
	glColor3ub( red, green, blue );
	gluQuadricDrawStyle( pQuadric, GLU_LINE );
	glPushMatrix();
	glTranslatef( center.x, center.y, center.z );
	gluSphere( pQuadric, radius, theta, phi );
	glPopMatrix();
	gluDeleteQuadric( pQuadric );
}

int CRender3DGL::ObjectsAt( float x, float y, float width, float height, RenderHitInfo_t* pObjects, int count )
{
	if ( !pObjects || count <= 0 || width <= 0 || height <= 0 )
		return 0;
	m_flPickX = x;
	m_flPickY = m_nHeight - y - 1;
	m_flPickWidth = width;
	m_flPickHeight = height;
	m_Hits.clear();
	m_bPicking = true;
	Render();
	glRenderMode( GL_RENDER );
	m_bPicking = false;
	SetupCamera();
	bool reverse = IsEnabled( RENDER_REVERSE_SELECTION );
	std::stable_sort( m_Hits.begin(), m_Hits.end(),
					  [reverse]( const RenderHitInfo_t& first, const RenderHitInfo_t& second )
					  { return reverse ? first.nDepth > second.nDepth : first.nDepth < second.nDepth; } );
	int copied = min( count, (int)m_Hits.size() );
	for ( int i = 0; i < copied; i++ )
		pObjects[i] = m_Hits[i];
	return copied;
}

void CRender3DGL::BeginRenderHitTarget( CMapAtom* pObject, unsigned int handle )
{
	if ( !m_bPicking )
		return;
	m_pHitObject = pObject;
	m_nHitHandle = handle;
	glPushName( (GLuint)(UINT_PTR)pObject );
	glPushName( handle );
}

void CRender3DGL::EndRenderHitTarget()
{
	if ( !m_bPicking )
		return;
	glPopName();
	glPopName();
	int count = glRenderMode( GL_SELECT );
	if ( count > 0 && m_SelectionBuffer[0] == 2 && m_Hits.size() < MAX_PICK_HITS )
	{
		RenderHitInfo_t hit = { m_pHitObject, m_nHitHandle, m_SelectionBuffer[1] };
		m_Hits.push_back( hit );
	}
}

void CRender3DGL::WorldToScreen( Vector2D& screen, const Vector& world )
{
	GLdouble model[16], projection[16], x, y, z;
	GLint	 viewport[4];
	glGetDoublev( GL_MODELVIEW_MATRIX, model );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );
	gluProject( world.x, world.y, world.z, model, projection, viewport, &x, &y, &z );
	screen.Init( (float)x, (float)y );
}

void CRender3DGL::ScreenToWorld( Vector& world, const Vector2D& screen )
{
	GLdouble model[16], projection[16], x, y, z;
	GLint	 viewport[4];
	glGetDoublev( GL_MODELVIEW_MATRIX, model );
	glGetDoublev( GL_PROJECTION_MATRIX, projection );
	glGetIntegerv( GL_VIEWPORT, viewport );
	gluUnProject( screen.x, screen.y, 0, model, projection, viewport, &x, &y, &z );
	world.Init( (float)x, (float)y, (float)z );
}

void CRender3DGL::ScreenToClient( Vector2D& client, const Vector2D& screen )
{
	client.Init( screen.x, m_nHeight - screen.y - 1 );
}

void CRender3DGL::ClientToScreen( Vector2D& screen, const Vector2D& client )
{
	screen.Init( client.x, m_nHeight - client.y - 1 );
}

void CRender3DGL::BeginParallel()
{
	glPushAttrib( GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT );
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	glOrtho( 0, m_nWidth, 0, m_nHeight, -99999, 99999 );
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );
	glDisable( GL_CULL_FACE );
	glDisable( GL_ALPHA_TEST );
	glDisable( GL_LIGHTING );
	glDisable( GL_TEXTURE_2D );
}

void CRender3DGL::EndParallel()
{
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopAttrib();
}

void CRender3DGL::ResetFocus()
{
	SetFocus( m_hWnd );
}

bool CRender3DGL::NeedsOverlay() const
{
	return m_eRenderMode == RENDER_MODE_FLAT || m_eRenderMode == RENDER_MODE_TEXTURED;
}

void CRender3DGL::DebugHook1( void* )
{
	m_bDrawFPS = !m_bDrawFPS;
}

void CRender3DGL::DebugHook2( void* )
{
}
