#include "basefilesystem.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <csignal>
#ifndef _WIN32
#include <strings.h>
#include <alloca.h>
#include <fcntl.h>
#include <sys/mount.h>
#endif
#ifndef _WIN32
extern "C" const int** __ctype_tolower_loc();
#endif

extern bool s_bShowDiag;
extern const char* s_pszDbgPathMatch;

#ifndef _WIN32

static char* GetSteamContentPath()
{
	char szContentLink[FS_MAX_PATH_LENGTH];
	char* home;
	char* resolved;

	home = getenv( "HOME" );
	snprintf( szContentLink, FS_MAX_PATH_LENGTH, "%s/.steam/steam", home );
	szContentLink[FS_MAX_PATH_LENGTH - 1] = 0;
	resolved = (char*)malloc( FS_MAX_PATH_LENGTH + 1 );
	*resolved = 0;
	if ( realpath( szContentLink, resolved ) != NULL )
	{
		strcat( resolved, "/" );
	}
	else
	{
		resolved[0] = 0;
		resolved[1] = '/';
	}

	return resolved;
}

static char Descend( char* pPath, unsigned int nStartIdx, bool bAllowBasenameMismatch, unsigned int nLevel )
{
	unsigned int cursor;
	char* end;
	char saved;
	unsigned int nNextSlash;
	int accessResult;
	unsigned int componentAt;
	char* component;
	unsigned int componentLen;
	char found;

	if ( s_bShowDiag )
	{
		const char* flag;

		flag = "false ";
		if ( bAllowBasenameMismatch )
		{
			flag = "true";
		}

		fprintf( stderr, "(%zu) Descend: %s, (%s), %s\n", nLevel, pPath, &pPath[nStartIdx], flag );
	}

	cursor = nStartIdx + 1;
	end = &pPath[cursor];
	saved = *end;

	if ( saved == 0 )
	{
		return 1;
	}

	if ( saved == '/' )
	{
		nNextSlash = cursor;
	}
	else
	{
		unsigned int scan;
		char ch;

		scan = cursor;
		do
		{
			ch = pPath[++scan];
			end = &pPath[scan];
		}
		while ( ch != 0 && ch != '/' );

		saved = pPath[scan];
		nNextSlash = scan;
	}

	*end = 0;
	accessResult = access( pPath, 0 );
	*end = saved;
	if ( accessResult == 0
		&& ( saved != '/' || Descend( pPath, nNextSlash, bAllowBasenameMismatch, nLevel + 1 ) != 0 ) )
	{
		return 1;
	}

	CDirPtr spDir;

	if ( nStartIdx != 0 )
	{
		char* boundary;
		char boundaryChar;

		boundary = &pPath[nStartIdx];
		boundaryChar = *boundary;
		*boundary = 0;
		spDir.m_pDir = opendir( pPath );
		*boundary = boundaryChar;
		componentAt = nStartIdx + 1;
	}
	else if ( *pPath == '/' )
	{
		componentAt = 1;
		spDir.m_pDir = opendir( "/" );
	}
	else
	{
		componentAt = 0;
		spDir.m_pDir = opendir( "." );
	}

	errno = 0;
	component = &pPath[componentAt];
	componentLen = nNextSlash - componentAt;
	found = 0;
	if ( spDir.m_pDir != NULL )
	{
		struct dirent* ent;

		ent = readdir( spDir.m_pDir );
		if ( ent != NULL )
		{
			char* componentEnd;

			componentEnd = &pPath[nNextSlash];
			do
			{
				unsigned char endChar;

				endChar = (unsigned char)*componentEnd;
				if ( s_bShowDiag )
				{
					*componentEnd = 0;
					fprintf( stderr, "\t(%zu) comparing %s with %s\n", nLevel, ent->d_name, component );
				}

				*componentEnd = 0;
				if ( strcasecmp( component, ent->d_name ) == 0 )
				{
					int sameBytes;

					sameBytes = strcmp( component, ent->d_name );
					*componentEnd = (char)endChar;
					if ( sameBytes != 0 )
					{
						char letter;

						letter = ent->d_name[0];
						if ( letter != 0 )
						{
							char* dst;
							const char* src;

							dst = component;
							src = ent->d_name;
							do
							{
								if ( letter == '/' )
								{
									break;
								}

								++dst;
								++src;
								*( dst - 1 ) = letter;
								letter = *src;
							}
							while ( *src != 0 );
						}

						if ( saved != '/'
							|| Descend( pPath, nNextSlash, bAllowBasenameMismatch, nLevel + 1 ) != 0 )
						{
							found = 1;
							break;
						}
					}
				}
				else
				{
					*componentEnd = (char)endChar;
				}

				ent = readdir( spDir.m_pDir );
			}
			while ( ent != NULL );
		}
	}

	if ( found == 0 )
	{
		unsigned char more;

		more = (unsigned char)( saved == '/' );
		if ( s_bShowDiag && saved == '/' )
		{
			char* componentEnd;
			char endChar;
			size_t compBytes;
			char* compCopy;
			char first;
			size_t parentBytes;
			char* parentCopy;

			componentEnd = component + componentLen;
			endChar = *componentEnd;
			*componentEnd = 0;
			compBytes = strlen( component ) + 1;
			compCopy = (char*)alloca( compBytes );
			memcpy( compCopy, component, compBytes );
			*componentEnd = endChar;
			first = *component;
			*component = 0;
			parentBytes = strlen( pPath ) + 1;
			parentCopy = (char*)alloca( parentBytes );
			memcpy( parentCopy, pPath, parentBytes );
			*component = first;
			fprintf( stderr, "(%zu) readdir failed to find '%s' in '%s'\n", nLevel, compCopy, parentCopy );
		}

		found = (char)( (unsigned char)bAllowBasenameMismatch > more );
	}

	if ( spDir.m_pDir != NULL )
	{
		closedir( spDir.m_pDir );
	}

	return found;
}

int ResolveCaseInsensitivePath( const char* pszIn, char** ppszOut, bool bAllowBasenameMismatch,
		char* pszOutBuf, unsigned int OutBufLen )
{
	char* out;
	static char* pszSteamPath = NULL;
	static size_t nSteamPathLen = 0;
	char* steam;
	size_t n;
	char* component;
	size_t nStartIdx;

	out = pszOutBuf;
	if ( strlen( pszIn ) >= OutBufLen )
	{
		out = strdup( pszIn );
		if ( out == NULL )
		{
			return 3;
		}
	}
	else
	{
		strncpy( pszOutBuf, pszIn, OutBufLen );

		if ( pszOutBuf == NULL )
		{
			return 3;
		}
	}

	steam = pszSteamPath;
	if ( pszSteamPath != NULL )
	{
		n = nSteamPathLen;
	}
	else
	{
		steam = GetSteamContentPath();
		pszSteamPath = steam;
		n = strlen( steam );
		nSteamPathLen = n;
	}

	if ( strncasecmp( out, steam, n ) == 0 )
	{
		memcpy( out, steam, n );

		nStartIdx = n - 1;
		component = out + nStartIdx;
	}
	else
	{
		component = out;
		nStartIdx = 0;
	}

	if ( *component != 0 )
	{
		char* p;

		p = component;
		do
		{
			int folded;

			if ( *p == '/' )
			{
				component = p + 1;
			}

			folded = (signed char)*p;
			if ( (unsigned int)( folded + 128 ) <= FS_CTYPE_TABLE_RANGE )
			{
				*p = (char)( *__ctype_tolower_loc() )[folded];
			}
			else
			{
				*p = (char)folded;
			}

			++p;
		}
		while ( *p != 0 );
	}

	if ( access( out, 0 ) == 0 )
	{
		*ppszOut = out;
		if ( s_bShowDiag )
		{
			fprintf( stderr, "Lowered '%s' -> '%s'\n", pszIn, out );
		}

		return 1;
	}

	if ( bAllowBasenameMismatch && *component != 0 )
	{
		const char* src;
		char* dst;

		src = pszIn + ( component - out );
		dst = component;
		do
		{
			++src;
			++dst;
			*( dst - 1 ) = *( src - 1 );
		}
		while ( *dst != 0 );
	}

	if ( s_pszDbgPathMatch != NULL && strcasestr( s_pszDbgPathMatch, pszIn ) != NULL )
	{
		if ( s_bShowDiag )
		{
			fprintf( stderr, "Breaking '%s' in '%s'\n", pszIn, s_pszDbgPathMatch );
		}

		std::raise( SIGTRAP );
	}

	if ( Descend( out, (unsigned int)nStartIdx, bAllowBasenameMismatch, 0 ) == 0 )
	{
		if ( s_bShowDiag )
		{
			fprintf( stderr, "Unmatched %s\n", pszIn );
		}

		return 3;
	}

	*ppszOut = out;
	if ( s_bShowDiag )
	{
		fprintf( stderr, "Matched '%s' -> '%s'\n", pszIn, out );
	}

	return 2;
}

extern "C" FILE* fopen64( const char* path, const char* mode );

const char* s_pszDbgPathMatch;
bool s_bShowDiag;

int ResolveCaseInsensitivePath( const char* pszIn, char** ppszOut, bool bAllowBasenameMismatch,
		char* pszOutBuf, size_t OutBufLen );

static void RefreshPathMatchDiag()
{
	static const char* s_pszDbgPathMatchOnce = getenv( "DBG_PATHMATCH" );

	s_pszDbgPathMatch = s_pszDbgPathMatchOnce;
	s_bShowDiag = s_pszDbgPathMatch != NULL;
}

PathMod_t pathmatch( const char* pszIn, char** ppszOut, bool bAllowBasenameMismatch,
		char* pszOutBuf, size_t OutBufLen )
{
	RefreshPathMatchDiag();
	*ppszOut = NULL;
	if ( access( pszIn, 0 ) != 0 )
	{
		return (PathMod_t)ResolveCaseInsensitivePath( pszIn, ppszOut, bAllowBasenameMismatch, pszOutBuf, OutBufLen );
	}

	return kPathUnchanged;
}

extern "C" char* SteamRealPath( char* szOrigPath, char* szFinalPath, unsigned int uiFinalPathCapacity )
{
	char* szMatchPath;
	char szRealPath[FS_MAX_PATH_LENGTH];
	char PathMatchBuf[FS_MAX_PATH_LENGTH];
	char* szResolved;

	if ( uiFinalPathCapacity - 1 > ( FS_MAX_PATH_LENGTH - 1 ) || szFinalPath == NULL )
	{
		return NULL;
	}

	RefreshPathMatchDiag();
	szMatchPath = NULL;
	if ( access( szOrigPath, 0 ) != 0
		&& ResolveCaseInsensitivePath( szOrigPath, &szMatchPath, true, PathMatchBuf,
			sizeof( PathMatchBuf ) ) == kPathFailed )
	{
		return NULL;
	}

	if ( szMatchPath == NULL )
	{
		szMatchPath = szOrigPath;
	}

	szResolved = realpath( szMatchPath, szRealPath );
	if ( szResolved != szRealPath )
	{
		return NULL;
	}

	strncpy( szFinalPath, szResolved, uiFinalPathCapacity );
	return szFinalPath;
}

namespace
{
	class MatchedPath
	{
	public:
		MatchedPath( const char* path, bool allowNewBasename )
			: m_Original( path ), m_Matched( NULL )
		{
			pathmatch( path, &m_Matched, allowNewBasename, m_Buffer, sizeof( m_Buffer ) );
		}

		~MatchedPath()
		{
			if ( m_Matched != NULL && m_Matched != m_Original && m_Matched != m_Buffer )
			{
				free( m_Matched );
			}
		}

		const char* Get() const
		{
			return m_Matched == NULL ? m_Original : m_Matched;
		}

		MatchedPath( const MatchedPath& ) = delete;
		MatchedPath& operator=( const MatchedPath& ) = delete;

	private:
		const char*	m_Original;
		char*		m_Matched;
		char		m_Buffer[FS_PATH_MATCH_BUFFER_SIZE];
	};
}

extern "C" int __xstat( int version, const char* path, struct stat* buffer );
extern "C" int __lxstat( int version, const char* path, struct stat* buffer );
extern "C" int __xstat64( int version, const char* path, struct stat* buffer );
extern "C" int __lxstat64( int version, const char* path, struct stat* buffer );

extern "C" FILE* __wrap_freopen( const char* path, const char* mode, FILE* stream )
{
	MatchedPath matched( path, strpbrk( mode, "wa+" ) != NULL );
	return freopen( matched.Get(), mode, stream );
}

extern "C" FILE* __wrap_fopen( const char* path, const char* mode )
{
	MatchedPath matched( path, strpbrk( mode, "wa+" ) != NULL );
	return fopen( matched.Get(), mode );
}

extern "C" FILE* __wrap_fopen64( const char* path, const char* mode )
{
	MatchedPath matched( path, strpbrk( mode, "wa+" ) != NULL );
	return fopen64( matched.Get(), mode );
}

extern "C" int __wrap_open( const char* path, int flags, mode_t mode )
{
	MatchedPath matched( path, ( flags & O_ACCMODE ) != O_RDONLY );
	return open( matched.Get(), flags, mode );
}

extern "C" int __wrap_open64( const char* path, int flags, mode_t mode )
{
	MatchedPath matched( path, ( flags & O_ACCMODE ) != O_RDONLY );
	return open64( matched.Get(), flags, mode );
}

extern "C" int __wrap_creat( const char* path, mode_t mode )
{
	return __wrap_open( path, O_WRONLY | O_CREAT | O_TRUNC, mode );
}

extern "C" int __wrap_access( const char* path, int mode )
{
	MatchedPath matched( path, false );
	return access( matched.Get(), mode );
}

extern "C" int __wrap_stat( const char* path, struct stat* buffer )
{
	MatchedPath matched( path, false );
	return stat( matched.Get(), buffer );
}

extern "C" int __wrap_lstat( const char* path, struct stat* buffer )
{
	MatchedPath matched( path, false );
	return lstat( matched.Get(), buffer );
}

extern "C" int __wrap_scandir( const char* path, dirent*** entries,
		int ( *filter )( const dirent* ), int ( *compare )( const dirent**, const dirent** ) )
{
	MatchedPath matched( path, false );
	return scandir( matched.Get(), entries, filter, compare );
}

extern "C" DIR* __wrap_opendir( const char* path )
{
	MatchedPath matched( path, false );
	return opendir( matched.Get() );
}

extern "C" int __wrap___xstat( int version, const char* path, struct stat* buffer )
{
	MatchedPath matched( path, false );
	return __xstat( version, matched.Get(), buffer );
}

extern "C" int __wrap___lxstat( int version, const char* path, struct stat* buffer )
{
	MatchedPath matched( path, false );
	return __lxstat( version, matched.Get(), buffer );
}

extern "C" int __wrap___xstat64( int version, const char* path, struct stat* buffer )
{
	MatchedPath matched( path, false );
	return __xstat64( version, matched.Get(), buffer );
}

extern "C" int __wrap___lxstat64( int version, const char* path, struct stat* buffer )
{
	MatchedPath matched( path, false );
	return __lxstat64( version, matched.Get(), buffer );
}

extern "C" int __wrap_chmod( const char* path, mode_t mode )
{
	MatchedPath matched( path, false );
	return chmod( matched.Get(), mode );
}

extern "C" int __wrap_chown( const char* path, uid_t owner, gid_t group )
{
	MatchedPath matched( path, false );
	return chown( matched.Get(), owner, group );
}

extern "C" int __wrap_lchown( const char* path, uid_t owner, gid_t group )
{
	MatchedPath matched( path, false );
	return lchown( matched.Get(), owner, group );
}

extern "C" int __wrap_symlink( const char* oldPath, const char* newPath )
{
	MatchedPath destination( newPath, true );
	MatchedPath source( oldPath, false );
	return symlink( source.Get(), destination.Get() );
}

extern "C" int __wrap_link( const char* oldPath, const char* newPath )
{
	MatchedPath destination( newPath, true );
	MatchedPath source( oldPath, false );
	return link( source.Get(), destination.Get() );
}

extern "C" int __wrap_mknod( const char* path, mode_t mode, dev_t device )
{
	MatchedPath matched( path, true );
	return mknod( matched.Get(), mode, device );
}

extern "C" int __wrap_mount( const char* sourcePath, const char* targetPath,
		const char* filesystemType, unsigned int flags, const void* data )
{
	MatchedPath target( targetPath, false );
	MatchedPath source( sourcePath, false );
	return mount( source.Get(), target.Get(), filesystemType, flags, data );
}

extern "C" int __wrap_unlink( const char* path )
{
	MatchedPath matched( path, false );
	return unlink( matched.Get() );
}

extern "C" int __wrap_mkfifo( const char* path, mode_t mode )
{
	MatchedPath matched( path, true );
	return mkfifo( matched.Get(), mode );
}

extern "C" int __wrap_rename( const char* oldPath, const char* newPath )
{
	MatchedPath destination( newPath, true );
	MatchedPath source( oldPath, false );
	return rename( source.Get(), destination.Get() );
}

extern "C" int __wrap_utime( const char* path, const utimbuf* times )
{
	MatchedPath matched( path, false );
	return utime( matched.Get(), times );
}

extern "C" int __wrap_utimes( const char* path, const timeval* times )
{
	MatchedPath matched( path, false );
	return utimes( matched.Get(), times );
}

extern "C" int __wrap_mkdir( const char* path, mode_t mode )
{
	MatchedPath matched( path, true );
	return mkdir( matched.Get(), mode );
}

#endif
