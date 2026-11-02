#include <string.h>
#include "hud.h"
#include "cl_util.h"
#include "hud_iga.h"
#include "game_controls/iviewport.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Controls.h"
#include "Panel.h"
#include "PHandle.h"

#include "buildslave_assert.h"
#define ASSERT_FILE BUILDSLAVE_PATH( L"cstrike\\cl_dll\\hud_iga.cpp" )

class CIGAPanel;

struct IGATexture_t
{
	IGATexture_t()
	{
		m_nTextureId = -1;
		m_bValid = false;
		m_bShown = true;
		m_iInfo1 = 0;
		m_iInfo2 = 0;
		m_iInfo0 = 0;
		m_iWide = 0;
		m_iTall = 0;
		rgba = NULL;
		m_iUnused = 0;
	}

	int				m_nTextureId;
	bool			m_bValid;
	bool			m_bShown;
	int				m_iInfo1;
	int				m_iInfo2;
	int				m_iInfo0;
	int				m_iWide;
	int				m_iTall;
	unsigned char*	rgba;
	vgui2::PHandle	m_hPanel;
	int				m_iUnused;
};

class CEngineInGameAdvertising : public IEngineInGameAdvertising, public IClientInGameAdvertisingInternal
{
public:
	CEngineInGameAdvertising()
	{
		m_bInitialized = false;
		m_pClientIGA = NULL;
	}

	void Init( CreateInterfaceFn factory ) override;
	void Shutdown() override;
	void SetTexture( int space, int info0, int info1, int info2, int wide, int tall, const unsigned char* rgba ) override;

	vgui2::Panel* CreatePanel( int space, vgui2::Panel* parent ) override;
	void UpdatePanel( bool visible, int space ) override;
	bool IsPanelReady( int space, bool* shown ) override;
	void Reset() override;

private:
	friend class CIGAPanel;

	static int SpaceToIndex( int space )
	{
		switch ( space )
		{
		case IGA_SCOREBOARD:
		default:
			return 0;
		case IGA_SPECTATOR:
			return 1;
		}
	}

	bool							m_bInitialized;
	IClientDLLInGameAdvertising*	m_pClientIGA;
	IGATexture_t					m_Textures[IGA_NUM_SPACES];
};

static CEngineInGameAdvertising g_EngineInGameAdvertising;
IClientInGameAdvertisingInternal* g_pClientIGA = &g_EngineInGameAdvertising;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CEngineInGameAdvertising, IEngineInGameAdvertising, ENGINE_INGAMEADVERTISING_INTERFACE_VERSION, g_EngineInGameAdvertising );

class CIGAPanel : public vgui2::Panel
{
	DECLARE_CLASS_SIMPLE_NAMED( CIGAPanel, vgui2::Panel, "vgui::Panel" );

public:
	CIGAPanel( vgui2::Panel* parent, int space, int textureId ) : BaseClass( parent, "IGAPanel" )
	{
		m_iSpace = space;
		m_nTextureId = textureId;

		SetPaintEnabled( false );
		SetPaintBackgroundEnabled( true );

		if ( m_iSpace == IGA_SCOREBOARD )
			MakePopup( true, false );

		SetKeyBoardInputEnabled( false );
		SetMouseInputEnabled( false );

		vgui2::ivgui()->AddTickSignal( GetVPanel(), 0 );
	}

	void OnSizeChanged( int newWide, int newTall ) override;
	void OnTick() override;
	void PaintBackground() override;

private:
	int m_iSpace;
	int m_nTextureId;
};

void CEngineInGameAdvertising::Init( CreateInterfaceFn factory )
{
	m_pClientIGA = (IClientDLLInGameAdvertising*)factory( CLIENTDLL_INGAMEADVERTISING_INTERFACE_VERSION, NULL );
	if ( m_pClientIGA )
		m_bInitialized = true;
}

void CEngineInGameAdvertising::Shutdown()
{
	for ( int i = 0; i < IGA_NUM_SPACES; i++ )
	{
		delete[] m_Textures[i].rgba;
		m_Textures[i].rgba = NULL;
	}

	if ( m_bInitialized )
		m_bInitialized = false;
}

void CEngineInGameAdvertising::SetTexture( int space, int info0, int info1, int info2, int wide, int tall, const unsigned char* rgba )
{
	int index = space & ~IGA_SPACE_HIDDEN;

	if ( (unsigned int)index >= IGA_NUM_SPACES || wide < 0 || tall < 0 || ( wide && tall > 0x7fffffff / 4 / wide ) )
		return;

	if ( m_bInitialized )
	{
		IGATexture_t& tex = m_Textures[index];
		ASSERT_AT( tex.m_nTextureId != -1, 363 );

		if ( tex.rgba )
		{
			delete[] tex.rgba;
			tex.rgba = NULL;
		}

		tex.rgba = new unsigned char[wide * tall * 4];
		ASSERT_AT( tex.rgba, 374 );
		memcpy( tex.rgba, rgba, wide * tall * 4 );

		tex.m_bValid = true;
		tex.m_iInfo1 = info1;
		tex.m_iInfo0 = info0;
		tex.m_iWide = wide;
		tex.m_bShown = ( space & IGA_SPACE_HIDDEN ) == 0;
		tex.m_iInfo2 = info2;
		tex.m_iTall = tall;
	}
}

vgui2::Panel* CEngineInGameAdvertising::CreatePanel( int space, vgui2::Panel* parent )
{
	if ( !m_bInitialized )
		return NULL;

	IGATexture_t& tex = m_Textures[SpaceToIndex( space )];

	if ( tex.m_nTextureId == -1 )
		tex.m_nTextureId = vgui2::surface()->CreateNewTextureID( true );

	CIGAPanel* panel = new CIGAPanel( parent, space, tex.m_nTextureId );
	tex.m_hPanel = panel;
	return panel;
}

bool CEngineInGameAdvertising::IsPanelReady( int space, bool* shown )
{
	if ( !m_bInitialized )
		return false;

	IGATexture_t& tex = m_Textures[SpaceToIndex( space )];
	*shown = tex.m_bShown;

	return tex.m_hPanel.Get() && tex.m_nTextureId != -1 && tex.m_bValid;
}

void CEngineInGameAdvertising::UpdatePanel( bool visible, int space )
{
	if ( !m_bInitialized )
		return;

	int index = SpaceToIndex( space );
	IGATexture_t& tex = m_Textures[index];
	if ( tex.m_nTextureId == -1 || !tex.m_bValid )
		return;

	int screenWide, screenTall;
	vgui2::surface()->GetScreenSize( screenWide, screenTall );

	int wide, tall;
	tex.m_hPanel.Get()->GetSize( wide, tall );

	float coverage = (float)( wide * tall ) * 100.0f / (float)( screenWide * screenTall );
	if ( !tex.m_bShown )
		coverage = 100.0f;

	IGAImpression_t impression;
	impression.bVisible = visible;
	impression.iSpace = index;
	impression.iInfo1 = tex.m_iInfo1;
	impression.iInfo2 = tex.m_iInfo2;
	impression.iInfo0 = tex.m_iInfo0;
	impression.flCoverage = coverage;
	m_pClientIGA->ReportImpression( &impression );
}

void CEngineInGameAdvertising::Reset()
{
	if ( m_bInitialized )
		m_pClientIGA->Reset();
}

static float* ComputeResampleWeights( int count, int step, bool normalize )
{
	float* weights = new float[count * 2];
	float scale = normalize ? (float)step / (float)count : 1.0f;
	int divisor = normalize ? count : step;

	memset( weights, 0, count * 2 * sizeof( float ) );

	float* w = weights;
	int position = 0;
	for ( int i = 0; i < count; i++, w += 2 )
	{
		int previous = position;
		position += step;
		if ( position > count )
		{
			position -= count;
			w[0] = (float)( count - previous ) / (float)divisor;
			w[1] = (float)position / (float)divisor;
		}
		else
		{
			w[0] = scale;
			if ( position == count )
			{
				w[1] = -1.0f;
				position = 0;
			}
		}
	}

	return weights;
}

static inline float ResampleWeightAbs( float weight )
{
	return weight > 0.0f ? weight : 0.0f - weight;
}

static void ResampleUp( const unsigned char* src, int srcWide, int srcTall, unsigned char* dst, int dstWide, int dstTall )
{
	float* xWeights = ComputeResampleWeights( dstWide, srcWide, false );
	float* yWeights = ComputeResampleWeights( dstTall, srcTall, false );

	const unsigned char* srcRow = src;
	unsigned char* dstRow = dst;
	const float* yw = yWeights;

	for ( int sy = 0; sy < srcTall; yw += 2 )
	{
		const unsigned char* curRow = srcRow;
		unsigned char* out = dstRow;
		bool blendRow = yw[1] > 0.0001f;

		dstRow += dstWide * 4;
		if ( ResampleWeightAbs( yw[1] ) > 0.0001f )
		{
			sy++;
			srcRow += srcWide * 4;
		}

		const unsigned char* nextRow = srcRow;
		const float* xw = xWeights;
		int pixel = 0;

		for ( int sx = 0; sx < srcWide; xw += 2, out += 4 )
		{
			bool blendColumn = xw[1] > 0.0001f;
			int nextPixel = pixel;
			if ( ResampleWeightAbs( xw[1] ) > 0.0001f )
			{
				sx++;
				nextPixel += 4;
			}

			float color[3];
			float weight = xw[0] * yw[0];
			color[0] = (float)curRow[pixel + 0] * weight;
			color[1] = (float)curRow[pixel + 1] * weight;
			color[2] = (float)curRow[pixel + 2] * weight;

			if ( blendColumn )
			{
				weight = xw[1] * yw[0];
				color[0] += (float)curRow[nextPixel + 0] * weight;
				color[1] += (float)curRow[nextPixel + 1] * weight;
				color[2] += (float)curRow[nextPixel + 2] * weight;
			}

			if ( blendRow )
			{
				weight = xw[0] * yw[1];
				color[0] += (float)nextRow[pixel + 0] * weight;
				color[1] += (float)nextRow[pixel + 1] * weight;
				color[2] += (float)nextRow[pixel + 2] * weight;

				if ( blendColumn )
				{
					weight = xw[1] * yw[1];
					color[0] += (float)nextRow[nextPixel + 0] * weight;
					color[1] += (float)nextRow[nextPixel + 1] * weight;
					color[2] += (float)nextRow[nextPixel + 2] * weight;
				}
			}

			for ( int c = 0; c < 3; c++ )
				out[c] = (unsigned char)(int)color[c];
			out[3] = 255;

			pixel = nextPixel;
		}
	}

	delete[] xWeights;
	delete[] yWeights;
}

static void ResampleDown( const unsigned char* src, int srcWide, int srcTall, unsigned char* dst, int dstWide, int dstTall )
{
	float* xWeights = ComputeResampleWeights( srcWide, dstWide, true );
	float* yWeights = ComputeResampleWeights( srcTall, dstTall, true );

	float* accum = new float[dstWide * 3 * 2];
	float* row = accum;
	float* nextRow = accum + dstWide * 3;
	memset( accum, 0, dstWide * 3 * 2 * sizeof( float ) );

	const unsigned char* srcRow = src;
	const float* yw = yWeights;

	for ( int dy = 0; dy < dstTall; yw += 2 )
	{
		const unsigned char* in = srcRow;
		bool blendRow = yw[1] > 0.0001f;

		srcRow += srcWide * 4;

		const float* xw = xWeights;
		for ( int dx = 0; dx < dstWide; xw += 2, in += 4 )
		{
			float weight = xw[0] * yw[0];
			row[dx * 3 + 0] += (float)in[0] * weight;
			row[dx * 3 + 1] += (float)in[1] * weight;
			row[dx * 3 + 2] += (float)in[2] * weight;

			bool blendColumn = xw[1] > 0.0001f;
			if ( blendColumn )
			{
				weight = xw[1] * yw[0];
				row[dx * 3 + 3] += (float)in[0] * weight;
				row[dx * 3 + 4] += (float)in[1] * weight;
				row[dx * 3 + 5] += (float)in[2] * weight;
			}

			if ( blendRow )
			{
				weight = yw[1] * xw[0];
				nextRow[dx * 3 + 0] += (float)in[0] * weight;
				nextRow[dx * 3 + 1] += (float)in[1] * weight;
				nextRow[dx * 3 + 2] += (float)in[2] * weight;

				if ( blendColumn )
				{
					weight = yw[1] * xw[1];
					nextRow[dx * 3 + 3] += (float)in[0] * weight;
					nextRow[dx * 3 + 4] += (float)in[1] * weight;
					nextRow[dx * 3 + 5] += (float)in[2] * weight;
				}
			}

			if ( ResampleWeightAbs( xw[1] ) > 0.0001f )
				dx++;
		}

		if ( ResampleWeightAbs( yw[1] ) > 0.0001f )
		{
			const float* color = row;
			for ( int x = 0; x < dstWide; x++, color += 3, dst += 4 )
			{
				for ( int c = 0; c < 3; c++ )
					dst[c] = (unsigned char)(int)color[c];
				dst[3] = 255;
			}

			float* done = row;
			row = nextRow;
			nextRow = done;
			memset( done, 0, dstWide * 3 * sizeof( float ) );

			dy++;
		}
	}

	delete[] xWeights;
	delete[] yWeights;
	delete[] accum;
}

void CIGAPanel::OnSizeChanged( int newWide, int newTall )
{
	BaseClass::OnSizeChanged( newWide, newTall );

	for ( int i = 0; i < IGA_NUM_SPACES; i++ )
	{
		IGATexture_t& tex = g_EngineInGameAdvertising.m_Textures[i];
		if ( !tex.m_bValid )
			return;

		ASSERT_AT( tex.rgba, 496 );

		if ( tex.m_hPanel.Get() != this )
			continue;

		if ( newWide == tex.m_iWide && newTall == tex.m_iTall )
		{
			vgui2::surface()->DrawSetTextureRGBA( tex.m_nTextureId, tex.rgba, tex.m_iWide, tex.m_iTall, false, true );
		}
		else
		{
			int size = newWide * newTall * 4;
			unsigned char* rgba = new unsigned char[size];
			memset( rgba, 0, size );

			if ( tex.m_iWide < newWide || tex.m_iTall < newTall )
				ResampleUp( tex.rgba, tex.m_iWide, tex.m_iTall, rgba, newWide, newTall );
			else
				ResampleDown( tex.rgba, tex.m_iWide, tex.m_iTall, rgba, newWide, newTall );

			vgui2::surface()->DrawSetTextureRGBA( tex.m_nTextureId, rgba, newWide, newTall, false, true );
			delete[] rgba;
		}
		return;
	}
}

void CIGAPanel::OnTick()
{
	BaseClass::OnTick();

	bool visible = IsVisible();
	switch ( m_iSpace )
	{
	case IGA_SCOREBOARD:
	default:
		visible = gViewPortInterface && gViewPortInterface->IsScoreBoardVisible() && !gViewPortInterface->IsSpectatorGUIVisible();
		break;
	case IGA_SPECTATOR:
		visible = gViewPortInterface && gViewPortInterface->IsSpectatorGUIVisible();
		break;
	}

	g_pClientIGA->UpdatePanel( visible, m_iSpace );
}

void CIGAPanel::PaintBackground()
{
	int wide, tall;
	GetSize( wide, tall );

	if ( m_nTextureId == -1 )
		return;

	for ( int i = 0; i < IGA_NUM_SPACES; i++ )
	{
		IGATexture_t& tex = g_EngineInGameAdvertising.m_Textures[i];
		if ( tex.m_hPanel.Get() != this )
			continue;

		if ( tex.m_bShown )
		{
			vgui2::surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );
			vgui2::surface()->DrawSetTexture( m_nTextureId );
			vgui2::surface()->DrawTexturedRect( 0, 0, wide, tall );
		}
		return;
	}
}
