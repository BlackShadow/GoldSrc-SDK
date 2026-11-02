#include "hud.h"
#include "widescreenpanel.h"
#include "Label.h"
#include <stdio.h>
#include <string.h>

using namespace vgui2;

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CWidescreenPanel::CWidescreenPanel( Panel* parent ) : BaseClass( parent, "WidescreenPanel", true )
{
	m_pWideLabel = NULL;
	m_flSubtitleChangeTime = 0;
	m_bSubtitlesLoaded = false;
	m_iNumSubtitles = -1;
}

void CWidescreenPanel::Update()
{
}

void CWidescreenPanel::ReadSubtitleFile()
{
	const char* level = gEngfuncs.pfnGetLevelName();
	if ( !level )
		return;
	char path[284];
	int length = snprintf( path, sizeof( path ), "dod/%s", level );
	if ( length < 0 || length >= (int)sizeof( path ) )
		return;
	char* extension = strchr( path, '.' );
	if ( !extension )
		return;
	*extension = 0;
	if ( strlen( path ) + sizeof( "_subtitles.txt" ) > sizeof( path ) )
		return;
	strcat( path, "_subtitles.txt" );
	FILE* file = fopen( path, "r" );
	if ( !file )
		return;
	m_iNumSubtitles = 0;
	int subtitle = 0;
	while ( subtitle < 64 )
	{
		m_iSubtitleIndeces[subtitle] = 0;
		memset( m_szSubtitleText[subtitle], 0, sizeof( m_szSubtitleText[subtitle] ) );
		if ( fscanf( file, "%i %1023[^\n]", &m_iSubtitleIndeces[subtitle], m_szSubtitleText[subtitle] ) != 2 )
			break;
		++subtitle;
	}
	m_iNumSubtitles = subtitle;
	fclose( file );
}

void CWidescreenPanel::setSubtitle( int subtitle )
{
	if ( m_iNumSubtitles < 0 )
		ReadSubtitleFile();
	for ( int index = 0; index < m_iNumSubtitles; ++index )
	{
		if ( m_iSubtitleIndeces[index] == subtitle )
		{
			if ( m_pWideLabel )
				m_pWideLabel->SetText( m_szSubtitleText[index] );
			m_flSubtitleChangeTime = gEngfuncs.GetClientTime() + 7;
			break;
		}
	}
}
