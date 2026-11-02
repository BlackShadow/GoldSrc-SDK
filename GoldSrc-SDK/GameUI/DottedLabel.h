#pragma once

#include "Label.h"

class CDottedBgLabel : public vgui2::Label
{
public:
	CDottedBgLabel( vgui2::Panel* parent, const char* name, const char* text, int dash, int gap, int corner );
	~CDottedBgLabel() override;
	void SetColor( Color color ) { m_color = color; }

protected:
	void PaintBackground() override;

private:
	int		m_dash;
	int		m_gap;
	int		m_corner;
	Color	m_color;
};
