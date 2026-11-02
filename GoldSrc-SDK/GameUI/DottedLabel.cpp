#include "DottedLabel.h"

#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"

CDottedBgLabel::CDottedBgLabel( vgui2::Panel* parent, const char* name, const char* text,
		int dash, int gap, int corner ) : vgui2::Label( parent, name, text )
{
	m_dash = dash ? dash : 2;
	m_gap = gap ? gap : 2;
	m_corner = corner ? corner : 6;
	m_color = vgui2::scheme()->GetIScheme( GetScheme() )->GetColor( "BorderBright", m_color );
}

CDottedBgLabel::~CDottedBgLabel()
{
}

void CDottedBgLabel::PaintBackground()
{
	vgui2::Panel::PaintBackground();
	int x, y, wide, tall;
	GetBounds( x, y, wide, tall );
	vgui2::surface()->DrawSetColor( m_color );
	DrawDashedLine( 0, 0, wide, 1, m_dash, m_gap );
	DrawDashedLine( 0, 0, 1, tall, m_dash, m_gap );
	DrawDashedLine( wide - 1, 0, wide, tall, m_dash, m_gap );
	DrawDashedLine( 0, tall - 1, wide, tall, m_dash, m_gap );
	vgui2::surface()->DrawFilledRect( 0, 0, m_corner, 1 );
	vgui2::surface()->DrawFilledRect( 0, 0, 1, m_corner );
	vgui2::surface()->DrawFilledRect( wide - m_corner, 0, wide, 1 );
	vgui2::surface()->DrawFilledRect( wide - 1, 0, wide, m_corner );
	vgui2::surface()->DrawFilledRect( 0, tall - 1, m_corner, tall );
	vgui2::surface()->DrawFilledRect( 0, tall - m_corner, 1, tall );
	vgui2::surface()->DrawFilledRect( wide - m_corner, tall - 1, wide, tall );
	vgui2::surface()->DrawFilledRect( wide - 1, tall - m_corner, wide, tall );
}
