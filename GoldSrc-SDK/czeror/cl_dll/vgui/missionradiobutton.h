#ifndef MISSIONRADIOBUTTON_H
#define MISSIONRADIOBUTTON_H

#include "RadioButton.h"

class CMissionRadioButton : public vgui2::RadioButton
{
public:
	CMissionRadioButton( vgui2::Panel* parent, const char* panelName, const char* text );
	~CMissionRadioButton() override;
	void Paint() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;

private:
	int m_iMarlettTall;
};

#endif
