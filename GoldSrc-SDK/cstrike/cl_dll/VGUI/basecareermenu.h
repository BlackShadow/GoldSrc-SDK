#pragma once

#include "vgui/IScheme.h"
#include "Frame.h"

class CCSCareerBaseMenu : public vgui2::Frame
{
public:
	CCSCareerBaseMenu( vgui2::Panel* parent, const char* name, bool showTaskBar );
	~CCSCareerBaseMenu() override;
	void Activate() override;
	void ActivateBuildMode() override;
	void GetOffset( int& x, int& y ) { x = m_offsetX; y = m_offsetY; }
	void UpdateScheme();

protected:
	int	m_offsetX;
	int	m_offsetY;
};

int		GetAlternateProportionalValueFromScaled( int scaledValue );
void	RepositionControl( vgui2::Panel* panel );
