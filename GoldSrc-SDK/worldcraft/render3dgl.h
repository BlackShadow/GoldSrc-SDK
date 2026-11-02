#ifndef RENDER3DGL_H
#define RENDER3DGL_H
#pragma once

#include <windows.h>
#include <gl/GL.h>
#include <vector>
#include "render3d.h"
#include "glcontext.h"

#define TEXTURE_CACHE_SIZE 2048
#define MAX_PICK_HITS 512
#define SELECTION_BUFFER_SIZE 200

class CMapClass;

class CRender3DGL : public CRender3D
{
public:
	CRender3DGL();
	~CRender3DGL() override;

	bool Initialize( HWND hWnd, CMapDoc* pDoc ) override;
	void ShutDown() override;
	void BindTexture( IEditorTexture* pTexture ) override;
	void UncacheAllTextures() override;

	RenderMode_t GetCurrentRenderMode() override { return m_eRenderMode; }
	RenderMode_t GetDefaultRenderMode() override { return m_eDefaultRenderMode; }
	float GetElapsedTime() override { return m_flElapsedTime; }
	float GetGridDistance() override;
	float GetGridSize() override;
	void GetViewPoint( Vector& point ) override;
	void GetViewForward( Vector& forward ) override;
	void GetViewUp( Vector& up ) override;
	void GetViewRight( Vector& right ) override;
	float GetRenderFrame() override { return (float)m_nRenderFrame; }
	bool IsEnabled( RenderState_t state ) override;
	bool IsPicking() override { return m_bPicking; }
	float LightPlane( Vector& normal ) override;

	void SetCamera( CCamera* pCamera ) override { m_pCamera = pCamera; }
	void SetDefaultRenderMode( RenderMode_t mode ) override;
	void SetRenderMode( RenderMode_t mode, bool force = false ) override;
	bool SetSize( int width, int height ) override;
	void SetProjectionMode( ProjectionMode_t mode ) override { m_eProjectionMode = mode; }
	void PreRender() override;
	void PostRender() override;
	void Render() override;
	void RenderClear() override;
	void RenderEnable( RenderState_t state, bool enable ) override;

	void RenderWireframeBox( const Vector& mins, const Vector& maxs, unsigned char red, unsigned char green,
							 unsigned char blue ) override;
	void RenderBox( const Vector& mins, const Vector& maxs, unsigned char red, unsigned char green, unsigned char blue,
					SelectionState_t selected ) override;
	void RenderLine( const Vector& start, const Vector& end, unsigned char red, unsigned char green,
					 unsigned char blue ) override;
	void RenderArrow( const Vector& start, const Vector& end, unsigned char red, unsigned char green,
					  unsigned char blue ) override;
	void RenderCone( const Vector& base, const Vector& tip, float radius, int slices, unsigned char red,
					 unsigned char green, unsigned char blue ) override;
	void RenderSphere( const Vector& center, float radius, int theta, int phi, unsigned char red, unsigned char green,
					   unsigned char blue ) override;
	void RenderWireframeSphere( const Vector& center, float radius, int theta, int phi, unsigned char red,
								unsigned char green, unsigned char blue ) override;

	int ObjectsAt( float x, float y, float width, float height, RenderHitInfo_t* pObjects, int count ) override;
	void BeginRenderHitTarget( CMapAtom* pObject, unsigned int handle = 0 ) override;
	void EndRenderHitTarget() override;
	void WorldToScreen( Vector2D& screen, const Vector& world ) override;
	void ScreenToWorld( Vector& world, const Vector2D& screen ) override;
	void ScreenToClient( Vector2D& client, const Vector2D& screen ) override;
	void ClientToScreen( Vector2D& screen, const Vector2D& client ) override;
	void BeginParallel() override;
	void EndParallel() override;
	void ResetFocus() override;
	bool NeedsOverlay() const override;
	bool DeferRendering() const override { return false; }
	void DebugHook1( void* pData = NULL ) override;
	void DebugHook2( void* pData = NULL ) override;

private:
	void SetupCamera();
	void RenderMapClass( CMapClass* pObject );

	typedef struct texturecache_s
	{
		IEditorTexture*	pTexture;
		int				textureID;
		GLuint			texture;
	} texturecache_t;

	CGLContext						m_Context;
	HWND							m_hWnd;
	CMapDoc*						m_pDoc;
	CCamera*						m_pCamera;
	RenderMode_t					m_eRenderMode;
	RenderMode_t					m_eDefaultRenderMode;
	ProjectionMode_t				m_eProjectionMode;
	int								m_nWidth;
	int								m_nHeight;
	float							m_fAspect;
	int								m_nRenderFrame;
	DWORD							m_dwLastFrame;
	float							m_flElapsedTime;
	bool							m_RenderState[7];
	bool							m_bPicking;
	bool							m_bDrawFPS;
	float							m_flPickX;
	float							m_flPickY;
	float							m_flPickWidth;
	float							m_flPickHeight;
	CMapAtom*						m_pHitObject;
	unsigned int					m_nHitHandle;
	GLuint							m_SelectionBuffer[SELECTION_BUFFER_SIZE];
	std::vector<texturecache_t>		m_TextureCache;
	std::vector<RenderHitInfo_t>	m_Hits;
	std::vector<CMapClass*>			m_RenderLast;
};

#endif
