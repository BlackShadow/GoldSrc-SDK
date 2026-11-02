#ifndef HIGHLIGHTBUTTON_H
#define HIGHLIGHTBUTTON_H

#include "Button.h"

class CHighlightButton : public vgui2::Button
{
public:
	CHighlightButton( vgui2::Panel* parent, const char* panelName, const char* text );
	~CHighlightButton() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	virtual void SetArmedBorder( vgui2::IBorder* border );
	vgui2::IBorder* GetBorder( bool depressed, bool armed, bool selected, bool keyfocus ) override;

private:
	vgui2::IBorder* m_armedBorder;
};

#endif
