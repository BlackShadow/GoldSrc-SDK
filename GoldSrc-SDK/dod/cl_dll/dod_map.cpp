#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include "com_model.h"
#include <math.h>

extern Vector v_sim_org;
extern Vector v_cl_angles;
extern cvar_t *cl_dmsmallmap;
extern void V_GetChasePos( int target, float *angles, float *origin, float *viewangles );

bool b_turnedoffmap;
int flZoomLevels[3] = { 1, 3, 8 };

int CHudDoDMap::Init()
{
	gHUD.AddHudElem( this );
	m_iFlags |= HUD_ACTIVE;
	m_flSmallMapScale = (float)flZoomLevels[gHUD.GetMinimapZoomLevel()];
	m_flIdealMapScale = m_flSmallMapScale;
	m_flPreviousMapScale = m_flSmallMapScale;
	return 1;
}

int CHudDoDMap::VidInit()
{
	b_turnedoffmap = false;
	m_flIdealMapScale = (float)flZoomLevels[gHUD.GetMinimapZoomLevel()];
	m_flPreviousMapScale = m_flIdealMapScale;
	memset( gHUD.m_Spectator.m_OverviewEntities, 0, sizeof( gHUD.m_Spectator.m_OverviewEntities ) );
	memset( m_ExtraOverviewEntities, 0, sizeof( m_ExtraOverviewEntities ) );
	gHUD.m_Spectator.m_MapSprite = NULL;
	gHUD.m_Spectator.LoadMapSprites();
	gHUD.InitMapBounds();
	return 1;
}

int CHudDoDMap::Draw( float ) { return 1; }
void CHudDoDMap::InitHUDData() {}
void CHudDoDMap::SetMapState( int state ) { gHUD.SetMinimapState( state ); }

float PM_SplineFraction( float value, float scale )
{
	double fraction = (double)scale * value;
	return (float)(3.0 * (fraction * fraction) - fraction * (fraction * fraction + fraction * fraction));
}

bool CHudDoDMap::AddMapEntityToMap( int sprite, double lifetime, Vector origin )
{
	for ( int i = 0; i < 32; ++i )
	{
		if ( !m_ExtraOverviewEntities[i].hSprite )
		{
			m_ExtraOverviewEntities[i].hSprite = sprite;
			m_ExtraOverviewEntities[i].killTime = gEngfuncs.GetClientTime() + lifetime;
			m_ExtraOverviewEntities[i].origin = origin;
			return true;
		}
	}
	return false;
}

void CHudDoDMap::CheckOverviewEntities()
{
	double time = gEngfuncs.GetClientTime();
	for ( int i = 0; i < 128; ++i )
	{
		if ( time > gHUD.m_Spectator.m_OverviewEntities[i].killTime )
			memset( &gHUD.m_Spectator.m_OverviewEntities[i], 0, sizeof( overviewEntity_t ) );
	}
	for ( int i = 0; i < 32; ++i )
	{
		if ( m_ExtraOverviewEntities[i].hSprite && time > m_ExtraOverviewEntities[i].killTime )
			memset( &m_ExtraOverviewEntities[i], 0, sizeof( map_icon_t ) );
	}
}

void CHudDoDMap::DrawOverview()
{
	if ( gHUD.m_iIntermission )
		return;
	double fraction = gEngfuncs.GetClientTime() - m_flZoomTime;
	if ( fraction < 1 )
		m_flSmallMapScale = (float)((3.0 * (fraction * fraction) - fraction * (fraction * fraction + fraction * fraction)) *
			(m_flIdealMapScale - m_flPreviousMapScale) + m_flPreviousMapScale);
	gHUD.m_Spectator.m_mapZoom = gHUD.m_Spectator.m_OverviewData.zoom;
	gHUD.m_Spectator.m_mapOrigin = gHUD.m_Spectator.m_OverviewData.origin;
	DrawOverviewLayer();
	DrawOverviewEntities();
	CheckOverviewEntities();
}

void CHudDoDMap::HandleMapButton()
{
	if ( gHUD.m_iFOV <= 89 )
		return;
	int state = gHUD.GetMinimapState();
	if ( !state )
		gHUD.SetMinimapState( 1 );
	else if ( state == 1 && cl_dmsmallmap->value >= 1 )
		gHUD.SetMinimapState( 2 );
	else
		gHUD.SetMinimapState( 0 );
}

void CHudDoDMap::HandleMapZoomButton()
{
	if ( gHUD.m_iFOV > 89 && gHUD.GetMinimapState() == 2 )
	{
		int zoom = gHUD.ZoomMinimap();
		m_flPreviousMapScale = m_flIdealMapScale;
		m_flIdealMapScale = (float)flZoomLevels[zoom];
		m_flZoomTime = gEngfuncs.GetClientTime();
	}
}

void CHudDoDMap::GetSmallMapOffset( float &x, float &y, float & )
{
	double centerX = v_sim_org.x;
	double centerY = v_sim_org.y;
	if ( gHUD.m_Spectator.m_OverviewData.rotated )
	{
		centerX = (double)gHUD.m_Spectator.m_OverviewData.origin.x + v_sim_org.y -
			gHUD.m_Spectator.m_OverviewData.origin.y;
		centerY = gHUD.m_Spectator.m_OverviewData.origin.y -
			((double)v_sim_org.x - gHUD.m_Spectator.m_OverviewData.origin.x);
	}
	x = (float)(x - centerX);
	y = (float)(y - centerY);
}

void CHudDoDMap::DrawOverviewLayer()
{
	CHudSpectator &spectator = gHUD.m_Spectator;
	model_t *dummy = (model_t *)gEngfuncs.GetSpritePointer( spectator.m_hsprUnkownMap );
	int xTiles = 8, yTiles = 6;
	if ( spectator.m_MapSprite )
	{
		int tiles = (int)sqrt( (double)(spectator.m_MapSprite->numframes / 12) );
		xTiles = 4 * tiles;
		yTiles = 3 * tiles;
	}
	float x = spectator.m_OverviewData.origin.x;
	float y = spectator.m_OverviewData.origin.y;
	float scale = 1;
	float alpha = 1;
	float z = 0;
	if ( gHUD.GetMinimapState() == 2 )
	{
		GetSmallMapOffset( x, y, z );
		scale = m_flSmallMapScale;
		alpha = 0.6f;
	}
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );
	gEngfuncs.pTriAPI->CullFace( TRI_FRONT );
	gEngfuncs.pTriAPI->Color4f( 1, 1, 1, alpha );
	double zoom = spectator.m_OverviewData.zoom;
	double xStep = -(8192.0 / zoom) / xTiles;
	double yStep = -(8192.0 / (1.3333334f * zoom)) / yTiles;
	double rowX = 4096.0 / (1.3333334f * zoom) + x;
	float firstY = (float)(4096.0 / zoom + y);
	float points[32][32][3];
	for ( int row = 0; row <= yTiles && row < 32; ++row )
	{
		double columnY = firstY;
		for ( int column = 0; column <= xTiles && column < 32; ++column )
		{
			points[row][column][0] = (float)(scale * rowX);
			points[row][column][1] = (float)(columnY * scale);
			points[row][column][2] = 0 * scale;
			columnY += yStep;
		}
		rowX += xStep;
	}
	int frame = 0;
	for ( int row = 0; row < yTiles; ++row )
	{
		for ( int column = 0; column < xTiles; ++column )
		{
			gEngfuncs.pTriAPI->SpriteTexture( spectator.m_MapSprite ? spectator.m_MapSprite : dummy,
				spectator.m_MapSprite ? frame : 0 );
			++frame;
			gEngfuncs.pTriAPI->Begin( TRI_QUADS );
			gEngfuncs.pTriAPI->TexCoord2f( 0.01f, 0.01f );
			gEngfuncs.pTriAPI->Vertex3f( points[row][column][0], points[row][column][1], points[row][column][2] );
			gEngfuncs.pTriAPI->TexCoord2f( 0.99f, 0.01f );
			gEngfuncs.pTriAPI->Vertex3f( points[row][column + 1][0], points[row][column + 1][1], points[row][column + 1][2] );
			gEngfuncs.pTriAPI->TexCoord2f( 0.99f, 0.99f );
			gEngfuncs.pTriAPI->Vertex3f( points[row + 1][column + 1][0], points[row + 1][column + 1][1], points[row + 1][column + 1][2] );
			gEngfuncs.pTriAPI->TexCoord2f( 0.01f, 0.99f );
			gEngfuncs.pTriAPI->Vertex3f( points[row + 1][column][0], points[row + 1][column][1], points[row + 1][column][2] );
			gEngfuncs.pTriAPI->End();
		}
	}
}

void CHudDoDMap::DrawOverviewIcon( Vector origin, int sprite, int scale, Vector angles )
{
	Vector forward, right;
	gEngfuncs.pfnAngleVectors( angles, forward, right, NULL );
	gEngfuncs.pTriAPI->CullFace( TRI_FRONT );
	model_t *model = (model_t *)gEngfuncs.GetSpritePointer( sprite );
	for ( int i = 0; i < 3; ++i )
	{
		forward[i] = (float)(forward[i] * (double)scale * 16.0);
		right[i] = (float)(16.0 * (scale * (double)right[i]));
	}
	gEngfuncs.pTriAPI->SpriteTexture( model, 0 );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	const float texture[4][2] = { { 0.01f, 0.01f }, { 0.99f, 0.01f }, { 0.99f, 0.99f }, { 0.01f, 0.99f } };
	for ( int corner = 0; corner < 4; ++corner )
	{
		gEngfuncs.pTriAPI->TexCoord2f( texture[corner][0], texture[corner][1] );
		Vector point;
		for ( int i = 0; i < 3; ++i )
		{
			double value = corner < 2 ? (double)origin[i] + right[i] : (double)origin[i] - right[i];
			point[i] = (float)((corner == 0 || corner == 3) ? value + forward[i] : value - forward[i]);
		}
		gEngfuncs.pTriAPI->Vertex3fv( point );
	}
	gEngfuncs.pTriAPI->End();
}

void CHudDoDMap::DrawOverviewEntities()
{
	CHudSpectator &spectator = gHUD.m_Spectator;
	double time = gEngfuncs.GetClientTime();
	bool smallMap = gHUD.GetMinimapState() == 2;
	float sizeScale = 12;
	gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1 );
	gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
	for ( int i = 0; i < 32; ++i )
	{
		map_icon_t &icon = m_ExtraOverviewEntities[i];
		if ( !icon.hSprite || time > icon.killTime )
			continue;
		Vector origin = icon.origin;
		if ( spectator.m_OverviewData.rotated )
		{
			float x = origin.x;
			origin.x = spectator.m_OverviewData.origin.x + origin.y - spectator.m_OverviewData.origin.y;
			origin.y = spectator.m_OverviewData.origin.y - (x - spectator.m_OverviewData.origin.x);
		}
		Vector angles( 0, 90, 0 );
		if ( smallMap )
		{
			GetSmallMapOffset( origin.x, origin.y, origin.z );
			origin.x *= m_flSmallMapScale;
			origin.y *= m_flSmallMapScale;
			sizeScale = min( 10 * m_flSmallMapScale, 40.0f );
			angles.x = 180;
		}
		else
			sizeScale = 16;
		origin.z = 0;
		DrawOverviewIcon( origin, icon.hSprite, (int)sizeScale, angles );
	}
	for ( int i = 0; i < 128; ++i )
	{
		overviewEntity_t &icon = spectator.m_OverviewEntities[i];
		if ( !icon.hSprite || icon.killTime - 10 > time )
			continue;
		cl_entity_t *entity = icon.entity;
		if ( entity->player && entity->curstate.team != g_iTeamNumber )
			continue;
		Vector angles = !entity->player && entity->baseline.iuser1 == 1 ? Vector( 0, 90, 0 ) : entity->curstate.angles;
		Vector origin = icon.b_dodMapTag ? icon.origin : entity->origin;
		if ( entity->player )
			angles.x = angles.z = 0;
		if ( spectator.m_OverviewData.rotated )
		{
			float x = origin.x;
			origin.x = spectator.m_OverviewData.origin.x + origin.y - spectator.m_OverviewData.origin.y;
			origin.y = spectator.m_OverviewData.origin.y - (x - spectator.m_OverviewData.origin.x);
			if ( entity->player )
				angles.y += 90;
		}
		else if ( entity->player )
			angles.y += 180;
		if ( smallMap )
		{
			GetSmallMapOffset( origin.x, origin.y, origin.z );
			if ( entity->player )
				sizeScale = 40;
			else if ( entity->curstate.entityType == 2 )
				sizeScale = min( m_flSmallMapScale * 10, 40.0f );
			else
				sizeScale = min( m_flSmallMapScale * 15, 45.0f );
			origin.x *= m_flSmallMapScale;
			origin.y *= m_flSmallMapScale;
		}
		origin.z = 0;
		DrawOverviewIcon( origin, icon.hSprite, (int)sizeScale, angles );
		if ( spectator.ShouldSetVoiceIcon( entity->index ) )
			entity->baseline.fuser4 = (float)(time + 1.6f);
		if ( (entity->player && (unsigned int)(entity->index - 1) <= 63 && gHUD.m_StatusBar.m_TargetTalking[entity->index]) ||
			entity->baseline.fuser4 > time )
		{
			gEngfuncs.GetSpritePointer( spectator.m_hsprSpeakerIcon );
			DrawOverviewIcon( origin, spectator.m_hsprSpeakerIcon, smallMap ? 25 : 10, Vector( 0, 90, 0 ) );
		}
	}
	Vector origin = v_sim_org;
	Vector angles( 0.0f, v_cl_angles.y, 0.0f );
	if ( g_iUser1 == 1 )
	{
		Vector chase;
		V_GetChasePos( g_iUser2, NULL, chase, angles );
	}
	angles.x = angles.z = 0;
	if ( spectator.m_OverviewData.rotated )
	{
		float x = origin.x;
		origin.x = spectator.m_OverviewData.origin.x + origin.y - spectator.m_OverviewData.origin.y;
		origin.y = spectator.m_OverviewData.origin.y - (x - spectator.m_OverviewData.origin.x);
		angles.y += 90;
	}
	else
		angles.y += 180;
	int sprite = spectator.m_hsprCameraSpec;
	if ( !gEngfuncs.IsSpectateOnly() )
		sprite = g_iTeamNumber == 2 ? spectator.m_hsprCameraAxis :
			gHUD.m_bBritish ? spectator.m_hsprCameraBrit : spectator.m_hsprCameraAllies;
	if ( sprite )
	{
		if ( smallMap )
			GetSmallMapOffset( origin.x, origin.y, origin.z );
		origin.z = 0;
		DrawOverviewIcon( origin, sprite, smallMap ? 50 : 16, angles );
	}
}
