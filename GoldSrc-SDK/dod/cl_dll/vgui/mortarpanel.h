#ifndef MORTARPANEL_H
#define MORTARPANEL_H

#include "Frame.h"

class CMortarPanel : public vgui2::Frame
{
public:
	typedef vgui2::Frame BaseClass;
	CMortarPanel( vgui2::Panel* parent );
	void ShowMortarPanel();
	void HideMortarPanel();
	void PostMortarValue( float value );
	void Paint() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void OnTick() override;
	virtual int KeyInput( int down, int keynum, const char* binding );
	void DrawRect( int x, int y, int wide, int tall );
	void AttackDown();
	void AttackUp();
private:
	float m_fShowTime;
	bool m_bAttack;
	double m_fAimValue;
	double m_fAimVelocity;
	int m_iNumPasses;
	int m_iTexture;
};

#endif
