#ifndef DODBUTTON_H
#define DODBUTTON_H

#include "Button.h"

namespace vgui2
{
class DoDButton : public Button
{
public:
	typedef Button BaseClass;
	DoDButton( Panel* parent, const char* panelName, const char* text );
	DoDButton( Panel* parent, const char* panelName, const wchar_t* text );
	void SetDefaultColor( Color foreground, Color background ) override;
	void SetArmedColor( Color foreground, Color background ) override;
	void SetDepressedColor( Color foreground, Color background ) override;
	Color GetButtonFgColor() override;
	Color GetButtonBgColor() override;
	void OnCursorEntered() override;
	void SetMouseOverCode( int code )
	{
		m_iMouseOverCode = code;
	}
protected:
	void ApplySchemeSettings( IScheme* scheme ) override;
private:
	Color m_cDefaultFgColor;
	Color m_cDefaultBgColor;
	Color m_cArmedFgColor;
	Color m_cArmedBgColor;
	Color m_cDepressedFgColor;
	Color m_cDepressedBgColor;
	int m_iMouseOverCode;
};
}

#endif
