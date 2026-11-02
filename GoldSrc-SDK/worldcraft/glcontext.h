#ifndef HAMMER_GLCONTEXT_H
#define HAMMER_GLCONTEXT_H
#pragma once

#include <windows.h>

class CGLContext
{
public:
	CGLContext();
	~CGLContext();
	bool Initialize( HWND hWnd );
	void ShutDown();
	bool MakeCurrent() const;
	bool Swap() const;

private:
	CGLContext( const CGLContext& );
	CGLContext& operator=( const CGLContext& );

	HWND	m_hWnd;
	HDC		m_hDC;
	HGLRC	m_hContext;
};

#endif
