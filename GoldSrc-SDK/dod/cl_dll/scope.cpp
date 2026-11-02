#include "hud.h"
#include "basetypes.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "dod_shared.h"
#include "r_studioint.h"
#include "triangleapi.h"
#include "demo_api.h"
#include "screenfade.h"
#include "shake.h"
#include "hltv.h"
#include "pm_shared.h"
#include <math.h>

extern engine_studio_api_t IEngineStudio;
extern Vector v_origin;
extern Vector v_angles;
extern float in_fov;
extern float CalcFov( float fov, float width, float height );

DECLARE_MESSAGE( m_Scope, Scope );

int CHudScope::Init()
{
	HOOK_MESSAGE( Scope );
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem( this );
	m_iWeaponId = 0;
	m_bWeaponChanged = false;
	m_nLastWpnId = 0;
	return 1;
}

int CHudScope::VidInit()
{
	int spring = gHUD.GetSpriteIndex( "scope_spring" );
	int k43 = gHUD.GetSpriteIndex( "scope_k43" );
	int binoc = gHUD.GetSpriteIndex( "scope_binoc" );
	int enfield = gHUD.GetSpriteIndex( "scope_enfield" );
	spring_sprite = gHUD.GetSprite( spring );
	spring_model = (model_s *)gEngfuncs.GetSpritePointer( IEngineStudio.IsHardware() ?
		spring_sprite : SPR_Load( "sprites/scope_spring_soft.spr" ) );
	k43_sprite = gHUD.GetSprite( k43 );
	k43_model = (model_s *)gEngfuncs.GetSpritePointer( IEngineStudio.IsHardware() ?
		k43_sprite : SPR_Load( "sprites/scope_k43_soft.spr" ) );
	binoc_sprite = gHUD.GetSprite( binoc );
	binoc_model = (model_s *)gEngfuncs.GetSpritePointer( binoc_sprite );
	enfield_sprite = gHUD.GetSprite( enfield );
	enfield_model = (model_s *)gEngfuncs.GetSpritePointer( IEngineStudio.IsHardware() ?
		enfield_sprite : SPR_Load( "sprites/scope_enfield_soft.spr" ) );
	Reset();
	return 1;
}

int CHudScope::Draw( float )
{
	return 1;
}

void CHudScope::Reset()
{
	if ( m_iWeaponId )
		m_bWeaponChanged = true;
	m_iWeaponId = 0;
}

void CHudScope::SetScope( int id )
{
	if ( m_iWeaponId != id )
		m_bWeaponChanged = true;
	m_iWeaponId = id;
}

int CHudScope::MsgFunc_Scope( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	int id = READ_BYTE();
	if ( id != m_iWeaponId )
		m_bWeaponChanged = true;
	m_iFlags |= HUD_ACTIVE;
	m_iWeaponId = id;
	return 1;
}

void CHudScope::Think()
{
	if ( !m_bWeaponChanged )
		return;
	screenfade_t fade;
	gEngfuncs.pfnGetScreenFade( &fade );
	fade.fader = fade.fadeg = fade.fadeb = 0;
	if ( m_iWeaponId == WEAPON_SCOPEDKAR || m_iWeaponId == WEAPON_SPRING || m_iWeaponId == WEAPON_BINOCULARS ||
		((m_iWeaponId == WEAPON_ENFIELD || m_iWeaponId == WEAPON_FG42) && (g_iWeaponFlags & 1)) )
	{
		fade.fadealpha = 255;
		fade.fadeSpeed = 90;
		fade.fadeEnd = gEngfuncs.GetClientTime();
		fade.fadeReset = gEngfuncs.GetClientTime() + 3;
		fade.fadeFlags = FFADE_OUT;
	}
	else
	{
		fade.fadealpha = 0;
		fade.fadeFlags = FFADE_OUT | FFADE_STAYOUT;
	}
	gEngfuncs.pfnSetScreenFade( &fade );
	m_bWeaponChanged = false;
}

void CHudScope::DrawTriApiScope()
{
	int inputFov = (int)in_fov;
	int fov = gHUD.m_iFOV;
	int weapon = m_iWeaponId;
	Vector view;
	gEngfuncs.GetViewAngles( view );
	if ( g_iUser1 == OBS_IN_EYE )
	{
		m_iWeaponId = WEAPON_SPRING;
		view = v_angles;
	}
	int playback = gEngfuncs.pDemoAPI->IsPlayingback();
	if ( inputFov != fov || !IEngineStudio.IsHardware() || fov < 1 || fov > 89 || playback )
		return;
	if ( !inputFov )
		inputFov = 90;
	if ( !weapon )
		weapon = gHUD.GetCurrentWeaponId();
	model_s *model;
	switch ( weapon )
	{
	case WEAPON_SCOPEDKAR:
		model = k43_model;
		break;
	case WEAPON_SPRING:
		model = spring_model;
		break;
	case WEAPON_FG42:
		if ( !(g_iWeaponFlags & 1) )
			return;
		model = k43_model;
		break;
	case WEAPON_ENFIELD:
		if ( !(g_iWeaponFlags & 1) )
			return;
		model = enfield_model;
		break;
	case WEAPON_BINOCULARS:
		model = binoc_model;
		break;
	default:
		return;
	}
	if ( !model )
		return;
	float verticalFov = CalcFov( (float)inputFov, (float)ScreenWidth, (float)ScreenHeight );
	float width = (float)(tan( inputFov / 360.0 * M_PI ) * 5.0 * 1.1);
	float height = (float)(tan( verticalFov / 360.0 * M_PI ) * 5.0 * 1.1);
	if ( gHUD.m_fPlayerDead )
		view.z = 80;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( view, forward, right, up );
	Vector origin = forward * 5 + v_origin;
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );
	gEngfuncs.pTriAPI->CullFace( TRI_FRONT );
	float left[4] = { -width, 0, 0, -width };
	float bottom[4] = { 0, 0, -height, -height };
	for ( int frame = 0; frame < 4; ++frame )
	{
		if ( !gEngfuncs.pTriAPI->SpriteTexture( model, frame ) )
			return;
		gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1 );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );
		gEngfuncs.pTriAPI->Brightness( 1 );
		gEngfuncs.pTriAPI->TexCoord2f( 0.01f, 0.99f );
		Vector vertex = bottom[frame] * up + left[frame] * right + origin;
		gEngfuncs.pTriAPI->Vertex3fv( vertex );
		gEngfuncs.pTriAPI->Brightness( 1 );
		gEngfuncs.pTriAPI->TexCoord2f( 0.01f, 0.01f );
		vertex = (bottom[frame] + height) * up + left[frame] * right + origin;
		gEngfuncs.pTriAPI->Vertex3fv( vertex );
		gEngfuncs.pTriAPI->Brightness( 1 );
		gEngfuncs.pTriAPI->TexCoord2f( 0.99f, 0.01f );
		vertex = (bottom[frame] + height) * up + (left[frame] + width) * right + origin;
		gEngfuncs.pTriAPI->Vertex3fv( vertex );
		gEngfuncs.pTriAPI->Brightness( 1 );
		gEngfuncs.pTriAPI->TexCoord2f( 0.99f, 0.99f );
		vertex = bottom[frame] * up + (left[frame] + width) * right + origin;
		gEngfuncs.pTriAPI->Vertex3fv( vertex );
		gEngfuncs.pTriAPI->End();
	}
}
