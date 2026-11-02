#pragma once

#include "Slider.h"

namespace vgui2
{
class TextEntry;
}

class LabelSliderCombo : public vgui2::Slider
{
public:
	LabelSliderCombo( vgui2::Panel* parent, const char* sliderName, const char* textEntryName );
	~LabelSliderCombo() override;

	void SetVisible( bool state ) override;
	void SetValue( int value, bool triggerChangeMessage = true ) override;

protected:
	void OnCursorMoved( int x, int y ) override;

private:
	void UpdateTextEntry( bool triggerChangeMessage );

	vgui2::TextEntry*	m_pTextEntry;
	int					m_currentValueShown;
};
