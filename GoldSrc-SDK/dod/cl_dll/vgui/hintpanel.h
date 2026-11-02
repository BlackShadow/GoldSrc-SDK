#ifndef HINTPANEL_H
#define HINTPANEL_H

#include "Frame.h"

namespace vgui2 { class Image; }

class CHintPanel : public vgui2::Frame
{
public:
	typedef vgui2::Frame BaseClass;
	CHintPanel( vgui2::Panel* parent );
	void ShowHintMessage( int team, const char* text );
	void SetVisibleImagePanel( const char* name, bool visible );
	void Paint() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void OnTick() override;
private:
	vgui2::Image* m_pImages[4];
	float m_fShowTime;
};

#endif
