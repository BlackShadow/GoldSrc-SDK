#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#define A3D_RENDER_WRITE_BUFFER	0x01
#define A3D_RENDER_LOOPING		0x02
#define A3D_RENDER_STEREO			0x04
#define A3D_RENDER_16BIT			0x08

#define A3D_RENDER_STOP_UNSET		0xFFFFFFFF

class DalInfo;
class DalBufferInfo;
class ResManBuffer;
class IA3dDal;
class IA3dDalBuffer;
struct CPlex;
struct a3d_platform_buffer_desc_s;

typedef struct a3d_render_description_s
{
	DWORD	dwFlags;
	DWORD	dwBufferBytes;
	DWORD	dwSampleRate;
	DWORD	dwBitsPerSample;
	DWORD	dwUpdateThreshold;
} a3d_render_description_t;

typedef struct a3d_render_stream_node_s
{
	struct a3d_render_stream_node_s*	pNext;
	struct a3d_render_stream_node_s*	pPrevious;
	DalBufferInfo*				pStream;
} a3d_render_stream_node_t;

typedef struct a3d_render_stream_list_s
{
	void Clear();

	a3d_render_stream_node_t*	pHead;
	a3d_render_stream_node_t*	pTail;
	LONG						cStreams;
	a3d_render_stream_node_t*	pFreeNodes;
	struct CPlex*	pBlocks;
	LONG						cBlockNodes;
} a3d_render_stream_list_t;

class DalInfo
{
public:
	DalInfo( IA3dDal* pBackend, const a3d_render_description_t* pDescription );

	IA3dDal* GetIDal() { return m_pBackend; }

	DWORD GetNumAvailable();
	HRESULT RefreshCapacity();
	HRESULT Update( DWORD dwTime );
	HRESULT CreateStaticDalBuffer( const a3d_platform_buffer_desc_s* pDescription, DalBufferInfo** ppStream );
	HRESULT CreateDalBuffer( DalBufferInfo* pSource, DalBufferInfo** ppStream );
	HRESULT CreateDalBuffer( DalBufferInfo** ppStream );
	HRESULT FindUnused( DalBufferInfo** ppStream );
	HRESULT RemoveStream( DalBufferInfo* pStream );

	IA3dDal*	m_pBackend;
	a3d_render_description_t	m_Description;
	LONG					m_cMaxStreams;
	a3d_render_stream_list_t	m_Streams;
	a3d_render_stream_list_t	m_Duplicates;
};

class DalBufferInfo
{
public:
	DalBufferInfo( DalInfo* pPool, IDirectSoundBuffer* pSoundBuffer,
		const a3d_render_description_t* pDescription, DWORD dwType );
	~DalBufferInfo();

	IDirectSoundBuffer* GetDSBuffer() { return m_pSoundBuffer; }
	IA3dDalBuffer* GetIDalBuffer() { return (IA3dDalBuffer*)m_pRenderBuffer; }

	BOOL IsUnused();
	BOOL IsExpired( DWORD dwTime );
	BOOL Refresh( LPDWORD pdwQueuedBytes );
	HRESULT GetBytesQueued( LPDWORD pdwQueuedBytes );
	HRESULT GetAudioBytesQueued( LPDWORD pdwRemainingBytes );
	HRESULT ConvertPcm( LPWORD pDestination, const BYTE* pSource, DWORD dwSamples, DWORD dwFlags );
	void Reset();
	HRESULT FormatAndFillBuffer( LPVOID pDestination, DWORD dwDestinationBytes, const BYTE* pSource,
		DWORD dwSourceBytes, LPDWORD pdwSourcePosition, DWORD dwFlags );
	HRESULT Write( DWORD dwTime, const BYTE* pSource, DWORD dwSourceBytes,
		LPDWORD pdwSourcePosition, DWORD dwFlags );

	DalInfo*			m_pPool;
	a3d_render_description_t	m_Description;
	IUnknown*				m_pRenderBuffer;
	IDirectSoundBuffer*		m_pSoundBuffer;
	ResManBuffer*		m_pOwner;
	DWORD					m_dwUnusedSince;
	DWORD					m_dwTimeout;
	DWORD					m_dwWritePosition;
	DWORD					m_dwStopPosition;
	DWORD					m_dwState;
	DWORD					m_dwType;
};
