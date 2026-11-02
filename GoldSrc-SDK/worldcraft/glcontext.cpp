#include "glcontext.h"
#include <gl/GL.h>

CGLContext::CGLContext() : m_hWnd( NULL ), m_hDC( NULL ), m_hContext( NULL )
{
}

CGLContext::~CGLContext()
{
	ShutDown();
}

bool CGLContext::Initialize( HWND hWnd )
{
	ShutDown();
	if ( !hWnd )
		return false;

	m_hWnd = hWnd;
	m_hDC = GetDC( hWnd );
	if ( m_hDC == NULL )
	{
		ShutDown();
		return false;
	}

	PIXELFORMATDESCRIPTOR format = {};
	format.nSize = sizeof( format );
	format.nVersion = 1;
	format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	format.iPixelType = PFD_TYPE_RGBA;
	format.cColorBits = 24;
	format.cDepthBits = 32;
	format.iLayerType = PFD_MAIN_PLANE;

	int index = GetPixelFormat( m_hDC );
	if ( index == 0 )
	{
		index = ChoosePixelFormat( m_hDC, &format );
		if ( ( index == 0 ) || !SetPixelFormat( m_hDC, index, &format ) )
		{
			ShutDown();
			return false;
		}
	}
	else if ( !DescribePixelFormat( m_hDC, index, sizeof( format ), &format ) ||
			  ( format.iPixelType != PFD_TYPE_RGBA ) ||
			  ( ( format.dwFlags & ( PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER ) ) !=
				( PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER ) ) )
	{
		ShutDown();
		return false;
	}

	m_hContext = wglCreateContext( m_hDC );
	if ( ( m_hContext == NULL ) || !MakeCurrent() )
	{
		ShutDown();
		return false;
	}

	RECT client;
	GetClientRect( hWnd, &client );
	glViewport( 0, 0, client.right - client.left, client.bottom - client.top );
	glClearColor( 0, 0, 0, 1 );
	return true;
}

void CGLContext::ShutDown()
{
	if ( m_hContext != NULL )
	{
		if ( wglGetCurrentContext() == m_hContext )
			wglMakeCurrent( NULL, NULL );
		wglDeleteContext( m_hContext );
		m_hContext = NULL;
	}
	if ( m_hDC != NULL )
	{
		ReleaseDC( m_hWnd, m_hDC );
		m_hDC = NULL;
	}
	m_hWnd = NULL;
}

bool CGLContext::MakeCurrent() const
{
	return ( m_hContext != NULL ) && wglMakeCurrent( m_hDC, m_hContext );
}

bool CGLContext::Swap() const
{
	return ( m_hDC != NULL ) && SwapBuffers( m_hDC );
}
