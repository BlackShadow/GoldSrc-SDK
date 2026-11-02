#pragma once

#include "vgui/IScheme.h"
#include "Frame.h"
#include <vector>

namespace vgui2
{
class Label;
}

class CCSCreditsMenu : public vgui2::Frame
{
public:
	CCSCreditsMenu( vgui2::Panel* parent );
	~CCSCreditsMenu() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void Activate() override;
	void OnThink() override;
	void OnCommand( const char* command ) override;
	static void SetDisconnect( bool disconnectAfter );

protected:
	void PerformLayout() override;
	void ReadCreditsFile();
	vgui2::Panel*				m_panel;
	std::vector<vgui2::Label*>	m_labelVector;
	float						m_panelSpeed;
	float						m_panelOffscreenStart;
	float						m_panelHideTime;
	float						m_startTime;
	static bool					m_shouldDisconnect;
};

void	ShowCreditsMenu( bool disconnectAfter );
