#include "client.h"
#include "cmd.h"
#include "common.h"
#include "conprint.h"
#include "vgui_intwrap2.h"
#include "utlbuffer.h"
#include "utllinkedlist.h"
#include "utlvector.h"
#include "../external/steamworks/isteamhttp.h"
#include "../external/steamworks/steam_api_common.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>


#if !defined( _WIN32 )
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#endif

#if defined( _WIN32 ) && !defined( SW_RENDERER )
extern "C" _ACRTIMP void __cdecl _wassert( _In_z_ const wchar_t* expression, _In_z_ const wchar_t* filename,
	_In_ unsigned int line );
#endif

enum HTTPStatus
{
	HTTP_CONNECTING = 0,
	HTTP_FETCH = 1,
	HTTP_DONE = 2,
	HTTP_ABORTED = 3,
	HTTP_ERROR = 4
};

enum HTTPError
{
	HTTP_ERROR_NONE = 0,
	HTTP_ERROR_ZERO_LENGTH_FILE = 1,
	HTTP_ERROR_CONNECTION_CLOSED = 2,
	HTTP_ERROR_INVALID_URL = 3,
	HTTP_ERROR_INVALID_PROTOCOL = 4,
	HTTP_ERROR_CANT_BIND_SOCKET = 5,
	HTTP_ERROR_CANT_CONNECT = 6,
	HTTP_ERROR_NO_HEADERS = 7,
	HTTP_ERROR_FILE_NONEXISTENT = 8,
	HTTP_ERROR_API = 9,
	HTTP_ERROR_MAX = 10
};

struct RequestContext
{
	HTTPStatus status;
	HTTPError error;
	char baseURL[1024];
	char gamePath[1024];
	char serverURL[1024];
	HTTPRequestHandle hOpenResource;
};

static_assert( sizeof( RequestContext ) == 3084 );

static_assert( sizeof( CUtlLinkedList<RequestContext*, unsigned short> ) == 28 );

namespace SteamHTTPTypes
{

template <class T, class I, class L> class CUtlRBTree
{
public:
	struct Links_t
	{
		I m_Left;
		I m_Right;
		I m_Parent;
		I m_Tag;
	};

	struct Node_t : Links_t
	{
		T m_Data;
	};

	explicit CUtlRBTree( L less )
		: m_LessFunc( less ), m_Root( InvalidIndex() ), m_NumElements( 0 ), m_FirstFree( InvalidIndex() ),
		  m_TotalElements( 0 ), m_pElements( nullptr )
	{
	}

	~CUtlRBTree()
	{
		if ( m_TotalElements > 0 )
		{
			I nextFree = InvalidIndex();
			for ( I index = m_TotalElements - 1;; --index )
			{
				m_Elements[index].m_Right = nextFree;
				m_Elements[index].m_Left = index;
				nextFree = index;
				if ( index == 0 )
					break;
			}
			m_FirstFree = 0;
		}
		else
		{
			m_FirstFree = InvalidIndex();
		}
		m_Root = InvalidIndex();
		m_NumElements = 0;
	}

	static I InvalidIndex()
	{
		return static_cast<I>( -1 );
	}

	T& Element( I index )
	{
		return m_Elements[index].m_Data;
	}

	int TotalElements() const
	{
		return m_TotalElements;
	}

	bool IsValidIndex( I index ) const
	{
		return index >= 0 && index < m_TotalElements && m_Elements[index].m_Left != index;
	}

	I Find( const T& key )
	{
		I index = m_Root;
		while ( index != InvalidIndex() )
		{
			if ( m_LessFunc( key, m_Elements[index].m_Data ) )
				index = LeftChild( index );
			else if ( m_LessFunc( m_Elements[index].m_Data, key ) )
				index = RightChild( index );
			else
				return index;
		}
		return InvalidIndex();
	}

	I Insert( const T& key, bool insertDuplicates )
	{
		I parent = InvalidIndex();
		I index = m_Root;
		bool left = false;
		while ( index != InvalidIndex() )
		{
			parent = index;
			if ( m_LessFunc( key, m_Elements[index].m_Data ) )
			{
				left = true;
				index = LeftChild( index );
			}
			else
			{
				if ( !insertDuplicates && !m_LessFunc( m_Elements[index].m_Data, key ) )
				{
					m_Elements[index].m_Data = key;
					return index;
				}
				left = false;
				index = RightChild( index );
			}
		}

		index = AllocNode();
		Node_t& node = m_Elements[index];
		node.m_Parent = parent;
		node.m_Left = InvalidIndex();
		node.m_Right = InvalidIndex();
		node.m_Tag = RED;
		if ( parent == InvalidIndex() )
			m_Root = index;
		else if ( left )
			m_Elements[parent].m_Left = index;
		else
			m_Elements[parent].m_Right = index;
		InsertRebalance( index );
		++m_NumElements;
		m_Elements[index].m_Data = key;
		return index;
	}

	void Unlink( I elem )
	{
		if ( elem == InvalidIndex() )
			return;

		I y;
		if ( LeftChild( elem ) == InvalidIndex() || RightChild( elem ) == InvalidIndex() )
			y = elem;
		else
		{
			y = RightChild( elem );
			while ( LeftChild( y ) != InvalidIndex() )
				y = LeftChild( y );
		}

		I x = LeftChild( y );
		if ( x == InvalidIndex() )
			x = RightChild( y );

		int yColor = Tag( y );
		if ( x != InvalidIndex() )
			SetParent( x, Parent( y ) );
		if ( y != m_Root )
		{
			if ( y == LeftChild( Parent( y ) ) )
				SetLeftChild( Parent( y ), x );
			else
				SetRightChild( Parent( y ), x );
		}
		else
		{
			m_Root = x;
		}

		if ( y != elem )
		{
			SetParent( y, Parent( elem ) );
			if ( elem != m_Root )
			{
				if ( elem == LeftChild( Parent( elem ) ) )
					SetLeftChild( Parent( elem ), y );
				else
					SetRightChild( Parent( elem ), y );
			}
			else
			{
				m_Root = y;
			}
			SetLeftChild( y, LeftChild( elem ) );
			if ( LeftChild( elem ) != InvalidIndex() )
				SetParent( LeftChild( elem ), y );
			SetRightChild( y, RightChild( elem ) );
			if ( RightChild( elem ) != InvalidIndex() )
				SetParent( RightChild( elem ), y );
			SetColor( y, Tag( elem ) );
		}

		if ( yColor == BLACK && x != InvalidIndex() )
			RemoveRebalance( x );
	}

	void Free( I elem )
	{
		m_Elements[elem].m_Left = elem;
		m_Elements[elem].m_Right = m_FirstFree;
		--m_NumElements;
		m_FirstFree = elem;
	}

private:
	enum
	{
		RED = 0,
		BLACK = 1
	};

	Node_t& Links( I index )
	{
		static Node_t sentinel =
		{
			{ InvalidIndex(), InvalidIndex(), InvalidIndex(), BLACK },
			T()
		};
		if ( index == InvalidIndex() )
			return sentinel;
#if defined( _WIN32 ) && !defined( SW_RENDERER ) && !defined( HL25_HW_ENGINE )
		if ( index < 0 || index >= m_Elements.NumAllocated() )
			_wassert( L"IsIdxValid(i)", L"engine/download.cpp", __LINE__ );
#endif
		return m_Elements[index];
	}

	I LeftChild( I index )
	{
		return Links( index ).m_Left;
	}

	I RightChild( I index )
	{
		return Links( index ).m_Right;
	}

	I Parent( I index )
	{
		return Links( index ).m_Parent;
	}

	int Tag( I index )
	{
		return Links( index ).m_Tag;
	}

	void SetParent( I index, I parent )
	{
		m_Elements[index].m_Parent = parent;
	}

	void SetLeftChild( I index, I child )
	{
		Links( index ).m_Left = child;
	}

	void SetRightChild( I index, I child )
	{
		Links( index ).m_Right = child;
	}

	void SetColor( I index, int color )
	{
		if ( index == InvalidIndex() )
			return;
		m_Elements[index].m_Tag = color;
	}

	I AllocNode()
	{
		I index = m_FirstFree;
		if ( index == InvalidIndex() )
		{
			if ( m_TotalElements == m_Elements.NumAllocated() )
				m_Elements.Grow( 0 );
			index = m_TotalElements;
			++m_TotalElements;
		}
		else
		{
			m_FirstFree = m_Elements[index].m_Right;
		}
		m_pElements = m_Elements.Base();
		return index;
	}

	void RotateLeft( I index )
	{
		I right = RightChild( index );
		Links( index ).m_Right = LeftChild( right );
		if ( LeftChild( right ) != InvalidIndex() )
			SetParent( LeftChild( right ), index );
		if ( right != InvalidIndex() )
			SetParent( right, Parent( index ) );
		if ( index == m_Root )
			m_Root = right;
		else if ( index == LeftChild( Parent( index ) ) )
			SetLeftChild( Parent( index ), right );
		else
			SetRightChild( Parent( index ), right );
		Links( right ).m_Left = index;
		SetParent( index, right );
	}

	void RotateRight( I index )
	{
		I left = LeftChild( index );
		Links( index ).m_Left = RightChild( left );
		if ( RightChild( left ) != InvalidIndex() )
			SetParent( RightChild( left ), index );
		if ( left != InvalidIndex() )
			SetParent( left, Parent( index ) );
		if ( index == m_Root )
			m_Root = left;
		else if ( index == RightChild( Parent( index ) ) )
			SetRightChild( Parent( index ), left );
		else
			SetLeftChild( Parent( index ), left );
		Links( left ).m_Right = index;
		SetParent( index, left );
	}

	void InsertRebalance( I index )
	{
		while ( index != m_Root && Tag( Parent( index ) ) == RED )
		{
			I parent = Parent( index );
			I grandparent = Parent( parent );
			if ( parent == LeftChild( grandparent ) )
			{
				I uncle = RightChild( grandparent );
				if ( Tag( uncle ) == RED )
				{
					Links( parent ).m_Tag = BLACK;
					Links( uncle ).m_Tag = BLACK;
					Links( grandparent ).m_Tag = RED;
					index = grandparent;
				}
				else
				{
					if ( index == RightChild( parent ) )
					{
						index = parent;
						RotateLeft( index );
						parent = Parent( index );
						grandparent = Parent( parent );
					}
					Links( parent ).m_Tag = BLACK;
					Links( grandparent ).m_Tag = RED;
					RotateRight( grandparent );
				}
			}
			else
			{
				I uncle = LeftChild( grandparent );
				if ( Tag( uncle ) == RED )
				{
					Links( parent ).m_Tag = BLACK;
					Links( uncle ).m_Tag = BLACK;
					Links( grandparent ).m_Tag = RED;
					index = grandparent;
				}
				else
				{
					if ( index == LeftChild( parent ) )
					{
						index = parent;
						RotateRight( index );
						parent = Parent( index );
						grandparent = Parent( parent );
					}
					Links( parent ).m_Tag = BLACK;
					Links( grandparent ).m_Tag = RED;
					RotateLeft( grandparent );
				}
			}
		}
		Links( m_Root ).m_Tag = BLACK;
	}

	void RemoveRebalance( I elem )
	{
		while ( elem != m_Root && Tag( elem ) == BLACK )
		{
			I parent = Parent( elem );
			if ( elem == LeftChild( parent ) )
			{
				I sibling = RightChild( parent );
				if ( Tag( sibling ) == RED )
				{
					SetColor( sibling, BLACK );
					SetColor( parent, RED );
					RotateLeft( parent );
					sibling = RightChild( parent );
				}
				if ( Tag( LeftChild( sibling ) ) == BLACK && Tag( RightChild( sibling ) ) == BLACK )
				{
					SetColor( sibling, RED );
					elem = parent;
				}
				else
				{
					if ( Tag( RightChild( sibling ) ) == BLACK )
					{
						SetColor( LeftChild( sibling ), BLACK );
						SetColor( sibling, RED );
						RotateRight( sibling );
						sibling = RightChild( parent );
					}
					SetColor( sibling, Tag( parent ) );
					SetColor( parent, BLACK );
					SetColor( RightChild( sibling ), BLACK );
					RotateLeft( parent );
					elem = m_Root;
				}
			}
			else
			{
				I sibling = LeftChild( parent );
				if ( Tag( sibling ) == RED )
				{
					SetColor( sibling, BLACK );
					SetColor( parent, RED );
					RotateRight( parent );
					sibling = LeftChild( parent );
				}
				if ( Tag( RightChild( sibling ) ) == BLACK && Tag( LeftChild( sibling ) ) == BLACK )
				{
					SetColor( sibling, RED );
					elem = parent;
				}
				else
				{
					if ( Tag( LeftChild( sibling ) ) == BLACK )
					{
						SetColor( RightChild( sibling ), BLACK );
						SetColor( sibling, RED );
						RotateLeft( sibling );
						sibling = LeftChild( parent );
					}
					SetColor( sibling, Tag( parent ) );
					SetColor( parent, BLACK );
					SetColor( LeftChild( sibling ), BLACK );
					RotateRight( parent );
					elem = m_Root;
				}
			}
		}
		SetColor( elem, BLACK );
	}

	L m_LessFunc;
	CUtlMemory<Node_t> m_Elements;
	I m_Root;
	I m_NumElements;
	I m_FirstFree;
	I m_TotalElements;
	Node_t* m_pElements;
};
}

template <class T> class CDefOps
{
public:
	static bool LessFunc( const T& lhs, const T& rhs )
	{
		return lhs < rhs;
	}
};

using CallResultTree =
	SteamHTTPTypes::CUtlRBTree<SteamAPICall_t, int, bool ( * )( const SteamAPICall_t&, const SteamAPICall_t& )>;

static_assert( sizeof( CallResultTree ) == 36 );

class DownloadManager;

template <class T, class P> class CMultipleCallResults : public CCallbackBase
{
public:
	using func_t = void ( T::* )( P*, bool );

	CMultipleCallResults( T* object, func_t function )
		: m_mapAPICalls( CDefOps<SteamAPICall_t>::LessFunc ), m_pObj( object ), m_Func( function )
	{
		m_nCallbackFlags = 0;
		m_iCallback = P::k_iCallback;
	}

	~CMultipleCallResults()
	{
		for ( int index = 0; index < m_mapAPICalls.TotalElements(); ++index )
		{
			if ( m_mapAPICalls.IsValidIndex( index ) )
				SteamAPI_UnregisterCallResult( this, m_mapAPICalls.Element( index ) );
		}
	}

	void Track( SteamAPICall_t call )
	{
		m_mapAPICalls.Insert( call, true );
		SteamAPI_RegisterCallResult( this, call );
	}

	void Run( void* param ) override;
	void Run( void* param, bool ioFailure, SteamAPICall_t call ) override;
	int GetCallbackSizeBytes() override;

private:
	CallResultTree m_mapAPICalls;
	T* m_pObj;
	func_t m_Func;
};

class DownloadManager
{
public:
	typedef CUtlVector<char*> StrVector;

	DownloadManager();
	~DownloadManager();

	void OnHTTPRequestCompleted( HTTPRequestCompleted_t* callback, bool ioFailure );
	bool HasMapBeenDownloadedFromServer( const char* serverMapName );
	void ClearDownloads();
	void MarkMapAsDownloadedFromServer( const char* serverMapName );
	void Reset();
	void Queue( const char* baseURL, const char* gamePath );
	void CheckActiveDownload();
	void StartNewDownload();
	void Update();

	CMultipleCallResults<DownloadManager, HTTPRequestCompleted_t> m_HTTPRequestCompleted;
	CUtlVector<RequestContext*> m_queuedRequests;
	CUtlLinkedList<RequestContext*, unsigned short> m_activeRequest;
	int m_lastPercent;
	int m_totalRequests;
	StrVector m_downloadedMaps;
};

static DownloadManager TheDownloadManager;

static char* DL_SharedVarArgs( const char* format, ... )
{
	static char string[4][1024];
	static int curstring = 0;

	curstring = ( curstring + 1 ) % 4;
	va_list arguments;
	va_start( arguments, format );
	_vsnprintf( string[curstring], sizeof( string[curstring] ), format, arguments );
	va_end( arguments );
	return string[curstring];
}

template <class T, class P> void CMultipleCallResults<T, P>::Run( void* param )
{
	( m_pObj->*m_Func )( static_cast<P*>( param ), false );
}

template <class T, class P> void CMultipleCallResults<T, P>::Run( void* param, bool ioFailure, SteamAPICall_t call )
{
	int elem = m_mapAPICalls.Find( call );
	if ( elem == m_mapAPICalls.InvalidIndex() )
		return;
	m_mapAPICalls.Unlink( elem );
	m_mapAPICalls.Free( elem );
	( m_pObj->*m_Func )( static_cast<P*>( param ), ioFailure );
}

template <class T, class P> int CMultipleCallResults<T, P>::GetCallbackSizeBytes()
{
	return sizeof( P );
}

DownloadManager::DownloadManager()
	: m_HTTPRequestCompleted( this, &DownloadManager::OnHTTPRequestCompleted ), m_lastPercent( 0 ), m_totalRequests( 0 )
{
}

DownloadManager::~DownloadManager()
{
	Reset();
	ClearDownloads();
}

void DownloadManager::OnHTTPRequestCompleted( HTTPRequestCompleted_t* callback, bool ioFailure )
{
	if ( callback == nullptr || ioFailure )
		return;

	unsigned short index = static_cast<unsigned short>( callback->m_ulContextValue );
	if ( !m_activeRequest.IsValidIndex( index ) )
		return;

	RequestContext* element = m_activeRequest.Element( index );
	uint32 bodySize = 0;
	if ( callback->m_bRequestSuccessful
		 && ( callback->m_eStatusCode == k_EHTTPStatusCode200OK
			  || callback->m_eStatusCode == k_EHTTPStatusCode304NotModified )
		 && SteamHTTP()->GetHTTPResponseBodySize( callback->m_hRequest, &bodySize ) && bodySize != 0 )
	{
		CUtlBuffer buffer( 0, static_cast<int>( bodySize ), false );
		SteamHTTP()->GetHTTPResponseBodyData( callback->m_hRequest, static_cast<uint8*>( buffer.Base() ), bodySize );
		if ( FS_FileExists( element->gamePath ) == 0 )
		{
			char filedir[260];
			std::strncpy( filedir, element->gamePath, sizeof( filedir ) );
			COM_FixSlashes( filedir );
#if defined( _WIN32 )
			char* slash = std::strrchr( filedir, '\\' );
#else
			char* slash = std::strrchr( filedir, '/' );
#endif
			if ( slash != nullptr )
			{
				*slash = 0;
				FS_CreateDirHierarchy( filedir, "GAMEDOWNLOAD" );
			}
			else
			{
				FS_CreateDirHierarchy( com_empty_string, "GAMEDOWNLOAD" );
			}
			FileHandle_t file = FS_OpenPathID( element->gamePath, "wb", "GAMEDOWNLOAD" );
			if ( file != nullptr )
			{
				FS_Write( buffer.Base(), static_cast<int>( bodySize ), 1, file );
				FS_Close( file );
				Con_DPrintf( "Saved %s%s to disk\n", element->baseURL, element->gamePath );
			}
		}
		element->error = HTTP_ERROR_NONE;
		element->status = HTTP_DONE;
		SteamHTTP()->ReleaseHTTPRequest( callback->m_hRequest );
	}
	else
	{
		Con_DPrintf( "Failed HTTP download of %s%s.\n", element->baseURL, element->gamePath );
		element->error = HTTP_ERROR_API;
		element->status = HTTP_ERROR;
	}
}

bool DownloadManager::HasMapBeenDownloadedFromServer( const char* serverMapName )
{
	if ( serverMapName == nullptr )
		return false;
	for ( int index = 0; index < m_downloadedMaps.Count(); ++index )
	{
		if ( m_downloadedMaps[index] != nullptr && _stricmp( serverMapName, m_downloadedMaps[index] ) == 0 )
			return true;
	}
	return false;
}

void DownloadManager::ClearDownloads()
{
	for ( int index = 0; index < m_downloadedMaps.Count(); ++index )
		delete[] m_downloadedMaps[index];
	m_downloadedMaps.RemoveAll();
}

void DownloadManager::MarkMapAsDownloadedFromServer( const char* serverMapName )
{
	if ( serverMapName == nullptr )
		return;
	if ( HasMapBeenDownloadedFromServer( serverMapName ) )
		return;

	std::size_t length = std::strlen( serverMapName );
	char* copy = new char[length + 1];
	std::strcpy( copy, serverMapName );
	m_downloadedMaps.AddToTail( copy );
}

void DownloadManager::Reset()
{
	if ( m_activeRequest.Count() != 0 )
	{
		unsigned short index = m_activeRequest.Head();
		while ( index != m_activeRequest.InvalidIndex() )
		{
			unsigned short next = m_activeRequest.Next( index );
			RequestContext* element = m_activeRequest.Element( index );
			Con_DPrintf( "Aborting download of %s\n", element->gamePath );
			SteamHTTP()->ReleaseHTTPRequest( element->hOpenResource );
			delete element;
			index = next;
		}
		m_activeRequest.RemoveAll();
		StopLoadingProgressBar();
	}

	for ( int index = 0; index < m_queuedRequests.Count(); ++index )
		delete m_queuedRequests[index];
	m_queuedRequests.RemoveAll();
	m_lastPercent = 0;
	m_totalRequests = 0;
}

static void FillRequest( RequestContext* request, const char* baseURL, const char* gamePath )
{
	std::memset( request, 0, sizeof( *request ) );
	request->status = HTTP_CONNECTING;
	_snprintf( request->baseURL, sizeof( request->baseURL ), "%s", baseURL );
	_snprintf( request->gamePath, sizeof( request->gamePath ), "%s", gamePath );
	_snprintf( 
		request->serverURL, sizeof( request->serverURL ), "%s", NET_AdrToString( cls.netchan.remote_address ) );
}

void DownloadManager::Queue( const char* baseURL, const char* gamePath )
{
	if ( std::strcmp( cls.servername, "local" ) == 0 )
		return;

	++m_totalRequests;
	RequestContext* request = new RequestContext;
	m_queuedRequests.AddToTail( request );
	FillRequest( request, baseURL, gamePath );
}

void CL_QueueHTTPDownload( char* filename )
{
	if ( std::strcmp( cls.servername, "local" ) == 0 )
		return;

	++TheDownloadManager.m_totalRequests;
	RequestContext* request = new RequestContext;
	TheDownloadManager.m_queuedRequests.AddToTail( request );
	FillRequest( request, cl.downloadUrl, filename );
}

void DownloadManager::CheckActiveDownload()
{
	CUtlVector<int> finished;
	unsigned short index = m_activeRequest.Head();
	bool completedRequest = false;
	float percentDone = 0.0f;
	float totalPercent = 0.0f;

	while ( index != m_activeRequest.InvalidIndex() )
	{
		RequestContext* element = m_activeRequest.Element( index );
		unsigned short next = m_activeRequest.Next( index );
		HTTPStatus status = element->status;
		if ( status == HTTP_DONE )
		{
			if ( element->hOpenResource != 0 )
			{
				SetSecondaryProgressBarText( element->gamePath );
				finished.AddToTail( index );
			}
			completedRequest = true;
		}
		else if ( status == HTTP_ERROR )
		{
			if ( element != nullptr )
				finished.AddToTail( index );
			completedRequest = true;
		}
		else if ( status == HTTP_FETCH )
		{
			SetSecondaryProgressBarText( element->gamePath );
			if ( element->hOpenResource != 0 )
			{
				float newPercent = 0.0f;
				SteamHTTP()->GetHTTPDownloadProgressPct( element->hOpenResource, &newPercent );
				totalPercent += 1.0f;
				percentDone += 0.01f * newPercent;
			}
		}
		index = next;
	}

	if ( totalPercent > 0.0f )
		percentDone = percentDone / totalPercent * 100.0f;

	if ( percentDone != static_cast<float>( m_lastPercent ) )
	{
		m_lastPercent = static_cast<int>( percentDone );
		SetSecondaryProgressBar( static_cast<float>( m_lastPercent ) * 0.01f );
	}

	for ( int finishedIndex = 0; finishedIndex < finished.Count(); ++finishedIndex )
	{
		unsigned short activeIndex = static_cast<unsigned short>( finished[finishedIndex] );
		delete m_activeRequest.Element( activeIndex );
		m_activeRequest.Remove( activeIndex );
	}

	if ( completedRequest && m_activeRequest.Count() == 0 && m_queuedRequests.Count() == 0 )
	{
		StopLoadingProgressBar();
		Cbuf_AddText( "retry\n" );
	}
}

void DownloadManager::StartNewDownload()
{
	if ( m_queuedRequests.Count() == 0 || SteamHTTP() == nullptr )
		return;

	while ( m_queuedRequests.Count() != 0 && m_activeRequest.Count() <= 4 )
	{
		RequestContext* request = m_queuedRequests[0];
		unsigned short index = m_activeRequest.AddToTail( request );
		m_queuedRequests.Remove( 0 );

		ContinueLoadingProgressBar( "Http", m_totalRequests - m_queuedRequests.Count(), 0.0f );
		SetLoadingProgressBarStatusText( "#GameUI_VerifyingAndDownloading" );
		SetSecondaryProgressBarText( request->gamePath );
		SetSecondaryProgressBar( 0.0f );
		Con_DPrintf( "Requesting HTTP download of %s%s.\n", request->baseURL, request->gamePath );
		m_lastPercent = 0;

		char fullURL[2048];
		_snprintf( fullURL, sizeof( fullURL ), "%s%s", request->baseURL, request->gamePath );
		fullURL[sizeof( fullURL ) - 1] = 0;
		for ( char* cursor = fullURL; *cursor != 0; ++cursor )
		{
			if ( *cursor == '\\' )
				*cursor = '/';
		}

		request->hOpenResource = SteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, fullURL );
		SteamHTTP()->SetHTTPRequestHeaderValue( request->hOpenResource, "Cache-Control", "no-cache" );
		SteamHTTP()->SetHTTPRequestHeaderValue( request->hOpenResource, "User-Agent", "Half-Life" );
		SteamHTTP()->SetHTTPRequestContextValue( request->hOpenResource, index );
		SteamAPICall_t call = 0;
		if ( SteamHTTP()->SendHTTPRequest( request->hOpenResource, &call ) )
		{
			m_HTTPRequestCompleted.Track( call );
			request->status = HTTP_FETCH;
		}
		else
		{
			SteamHTTP()->ReleaseHTTPRequest( request->hOpenResource );
			request->error = HTTP_ERROR_API;
			request->status = HTTP_ERROR;
		}
	}
}

void CL_HTTPUpdate()
{
	TheDownloadManager.CheckActiveDownload();
	TheDownloadManager.StartNewDownload();
}

void DownloadManager::Update()
{
	CheckActiveDownload();
	StartNewDownload();
}

void CL_HTTPStop_f()
{
	TheDownloadManager.Reset();
}

void CL_HTTPCancel_f()
{
	TheDownloadManager.Reset();
	TheDownloadManager.ClearDownloads();
}

int CL_GetDownloadQueueSize()
{
	return TheDownloadManager.m_queuedRequests.Count();
}

int CL_CanUseHTTPDownload()
{
	if ( cl.downloadUrl[0] == 0 )
		return 0;
	if ( TheDownloadManager.HasMapBeenDownloadedFromServer(
			 DL_SharedVarArgs( "%s:%s", cls.servername, cl.levelname ) ) )
		return 0;
	return 1;
}

void CL_MarkMapAsUsingHTTPDownload()
{
	TheDownloadManager.MarkMapAsDownloadedFromServer( DL_SharedVarArgs( "%s:%s", cls.servername, cl.levelname ) );
}
