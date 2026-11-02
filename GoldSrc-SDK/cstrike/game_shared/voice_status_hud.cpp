//========= Copyright 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// There are hud.h's coming out of the woodwork so this ensures that we get the right one.
#include "voice_status_hud.h"
#include "vgui_controls/Controls.h"
#include "game_controls/iviewport.h"
#include "vgui/ILocalize.h"
#include "vgui/IImage.h"
#include "vgui/IScheme.h"
#include "entity_types.h"
#include "shared_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

extern int cam_thirdperson;



// ---------------------------------------------------------------------- //
// The voice manager for the client.
// ---------------------------------------------------------------------- //
CVoiceStatusHud g_VoiceStatusHud;

IVoiceHud* GetClientVoiceHud()
{
	return &g_VoiceStatusHud;
}

CVoiceLabel::CVoiceLabel()
{
	m_pLabel = new VoiceVGUILabel( NULL, "VoiceLabel", "" );
	m_pLabel->SetParent( gViewPortInterface->GetViewPortPanel() );
	m_pLabel->SetProportional( true );
	m_pLabel->SetScheme( "ClientScheme" );
	m_pLabel->MakeReadyForUse();
	m_clientindex = -1;
	m_locationString = NULL;
	m_playerName = NULL;
}

CVoiceLabel::~CVoiceLabel()
{
	m_pLabel->MarkForDeletion();
}

void CVoiceLabel::GetContentSize( int& width, int& height )
{
	m_pLabel->GetContentSize( width, height );
	if ( height < 32 )
		height = 32;
}

void CVoiceLabel::SetBounds( int x, int y )
{
	m_pLabel->SetPos( x, y );
	int width, height;
	m_pLabel->GetContentSize( width, height );
	m_pLabel->SetSize( width, height );
}

void CVoiceLabel::VoiceVGUILabel::ApplySchemeSettings( vgui2::IScheme* scheme )
{
	Label::ApplySchemeSettings( scheme );
	SetTextImageIndex( 1 );
	SetImagePreOffset( 1, 2 );
	SetImageAtIndex( 0, vgui2::scheme()->GetImage( "gfx/vgui/speaker4", false ), 1 );
}



// ---------------------------------------------------------------------- //
// CVoiceLabel.
// ---------------------------------------------------------------------- //
void CVoiceLabel::SetLocation( const char* location )
{
	if ( !location || !*location )
	{
		if ( m_locationString )
		{
			delete[] m_locationString;
			m_locationString = NULL;
			RebuildLabelText();
		}
		return;
	}
	const wchar_t* localized = vgui2::localize()->Find( location );
	if ( localized )
	{
		// localized version
		if ( m_locationString && wcscmp( localized, m_locationString ) )
		{
			delete[] m_locationString;
			m_locationString = NULL;
		}
		if ( !m_locationString )
		{
			m_locationString = CloneWString( localized );
			RebuildLabelText();
		}
	}
	else
	{
		wchar_t* temporary = new wchar_t[strlen( location ) + 1];
		vgui2::localize()->ConvertANSIToUnicode( location, temporary, sizeof( temporary ) );
		if ( m_locationString && wcscmp( temporary, m_locationString ) )
		{
			delete[] m_locationString;
			m_locationString = NULL;
		}
		if ( !m_locationString )
		{
			m_locationString = CloneWString( temporary );
			RebuildLabelText();
		}
		delete[] temporary;
	}
}

void CVoiceLabel::SetPlayerName( const char* name )
{
	if ( m_playerName )
	{
		delete[] m_playerName;
		m_playerName = NULL;
	}
	if ( name )
		m_playerName = CloneString( name );
	RebuildLabelText();
}

void CVoiceLabel::RebuildLabelText()
{
	wchar_t text[512] = L"";
	if ( m_playerName )
	{
		wchar_t playerName[512] = L"";
		vgui2::localize()->ConvertANSIToUnicode( m_playerName, playerName, sizeof( playerName ) );
		const wchar_t* format = L"%ls   ";
		if ( m_locationString )
		{
			format = vgui2::localize()->Find( "#Voice_Location" );
			if ( !format )
				format = L"%ls/%ls   ";
		}
#ifdef _WIN32
		_snwprintf( text, 512, format, playerName, m_locationString );
#else
		swprintf( text, 512, format, playerName, m_locationString );
#endif
	}
	m_pLabel->SetText( text );
}

#ifdef _MSC_VER



// ---------------------------------------------------------------------- //
// CVoiceStatus.
// ---------------------------------------------------------------------- //

#pragma warning( suppress : 26495 )
#endif
CVoiceStatusHud::CVoiceStatusHud()
{
}

CVoiceStatusHud::~CVoiceStatusHud()
{
}

int CVoiceStatusHud::Init( IVoiceStatusHelper* helper, IVoiceStatus* status )
{
	m_VoiceHeadModel = 0;
	m_pHelper = helper;
	m_pStatus = status;
	gHUD.AddHudElem( this );
	m_iFlags = HUD_ACTIVE;
	m_pLocalPlayerTalkIcon = new vgui2::ImagePanel( NULL, "LocalPlayerIcon" );
	m_pLocalPlayerTalkIcon->SetParent( gViewPortInterface->GetViewPortPanel() );
	m_pLocalPlayerTalkIcon->SetVisible( false );
	m_pLocalPlayerTalkIcon->SetImage( vgui2::scheme()->GetImage( "gfx/vgui/icntlk_pl", false ) );
	return 1;
}

int CVoiceStatusHud::VidInit()
{
	// Figure out the voice head model height.
	m_VoiceHeadModelHeight = 45;
	char* file = (char*)gEngfuncs.COM_LoadFile( "scripts/voicemodel.txt", 5, NULL );
	if ( file )
	{
		char token[4096];
		gEngfuncs.COM_ParseFile( file, token );
		if ( token[0] >= '0' && token[0] <= '9' )
			m_VoiceHeadModelHeight = (float)atof( token );
		gEngfuncs.COM_FreeFile( file );
	}
	m_VoiceHeadModel = gEngfuncs.pfnSPR_Load( "sprites/voiceicon.spr" );
	return 1;
}

void CVoiceStatusHud::CreateEntities()
{
	if ( !m_VoiceHeadModel )
		return;
	cl_entity_s* localPlayer = gEngfuncs.GetLocalPlayer();
	int modelIndex = 0;
	for ( int player = 0; player < VOICE_MAX_PLAYERS; ++player )
	{
		if ( m_pStatus->GetSpeakerStatus( player ) != CVoiceStatus::VOICE_TALKING )
			continue;
		cl_entity_s* client = gEngfuncs.GetEntityByIndex( player + 1 );
		if ( !client || client->curstate.messagenum < localPlayer->curstate.messagenum )
			continue;

		// Don't show an icon for dead or spectating players (ie: invisible entities).
		if ( client->curstate.effects & EF_NODRAW )
			continue;

		// Don't show an icon for the local player unless we're in thirdperson mode.
		if ( client == localPlayer && !cam_thirdperson )
			continue;
		cl_entity_s* entity = &m_VoiceHeadModels[modelIndex++];
		memset( entity, 0, sizeof( *entity ) );
		entity->curstate.rendermode = kRenderTransAdd;
		entity->curstate.renderamt = 255;
		entity->baseline.renderamt = 255;
		entity->curstate.renderfx = kRenderFxNoDissipation;
		entity->curstate.framerate = 1;
		entity->curstate.frame = 0;
		entity->model = (model_s*)gEngfuncs.GetSpritePointer( m_VoiceHeadModel );
		entity->angles[0] = entity->angles[1] = entity->angles[2] = 0;
		entity->curstate.scale = 0.5f;
		entity->origin[0] = 0;
		entity->origin[1] = 0;
		entity->origin[2] = 45;
		entity->origin[0] += client->origin[0];
		entity->origin[1] += client->origin[1];
		entity->origin[2] += client->origin[2];
		gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, entity );
	}
}

void CVoiceStatusHud::UpdateLocation( int player, const char* location )
{
	if ( player - 1 < 0 )
		return;
	CVoiceLabel* label = FindVoiceLabel( player - 1 );
	if ( !label )
		return;
	label->SetLocation( location );
	RepositionLabels();
}

CVoiceLabel* CVoiceStatusHud::FindVoiceLabel( int player )
{
	for ( int index = 0; index < m_Labels.Count(); ++index )
	{
		if ( m_Labels[index]->GetClientIndex() == player )
			return m_Labels[index];
	}
	return NULL;
}

CVoiceLabel* CVoiceStatusHud::GetFreeVoiceLabel()
{
	CVoiceLabel* label = FindVoiceLabel( -1 );
	if ( !label )
	{
		label = new CVoiceLabel;
		m_Labels.AddToTail( label );
	}
	return label;
}

void CVoiceStatusHud::RepositionLabels()
{
	int y = gHUD.m_scrinfo.iHeight / 2;
	for ( int index = 0; index < m_Labels.Count(); ++index )
	{
		CVoiceLabel* label = m_Labels[index];
		int width, height;
		label->GetContentSize( width, height );
		label->SetBounds( gHUD.m_scrinfo.iWidth - width - 8, y );
		y += height + 2;
	}
}

void CVoiceStatusHud::UpdateSpeakerStatus( int player, bool talking )
{
	if ( player == -2 )  // this is the local player
	{
		if ( talking )
		{
			vgui2::IImage* image = m_pLocalPlayerTalkIcon->GetImage();
			if ( image )
			{
				int width, height;
				image->GetContentSize( width, height );
				m_pLocalPlayerTalkIcon->SetPos( gHUD.m_scrinfo.iWidth - width - 10,
					gHUD.m_scrinfo.iHeight - m_pHelper->GetAckIconHeight() - height );
				m_pLocalPlayerTalkIcon->SetVisible( true );
			}
		}
		else
		{
			m_pLocalPlayerTalkIcon->SetVisible( false );
		}
	}
	else if ( player >= 0 && player <= 64 )
	{
		if ( player == 0 )
			return;
		CVoiceLabel* label = FindVoiceLabel( player - 1 );
		if ( talking )
		{
			// If we don't have a label for this guy yet, then create one.
			if ( !label )
			{
				label = GetFreeVoiceLabel();
				if ( label )
				{
					// Get the name from the engine.
					hud_player_info_t info;
					memset( &info, 0, sizeof( info ) );
					gEngfuncs.pfnGetPlayerInfo( player, &info );
					int color[3];
					m_pHelper->GetPlayerTextColor( player, color );
					label->SetFgColor( Color( 255, 255, 255, 255 ) );
					label->SetBgColor( Color( color[0], color[1], color[2], 180 ) );
					label->SetPlayerName( info.name );
					label->SetLocation( m_pHelper->GetPlayerLocation( player ) );
					label->SetClientIndex( player - 1 );
					if ( !m_pHelper || m_pHelper->CanShowSpeakerLabels() )
						label->SetVisible( true );
				}
			}
		}
		else if ( label )
		{
			label->SetVisible( false );
			label->SetClientIndex( -1 );
		}
	}
	RepositionLabels();
}
