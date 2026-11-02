#ifndef DODMOTD_H
#define DODMOTD_H

#include "game_controls/clientmotd.h"

class CDoDMOTD : public CClientMOTD
{
public:
	typedef CClientMOTD BaseClass;
	CDoDMOTD( vgui2::Panel* parent );
	void OnCommand( const char* command ) override;
	void Paint() override;
	vgui2::Panel* CreateControlByName( const char* controlName ) override;
};

#endif
