#ifndef VGUI_MINIAPP_H
#define VGUI_MINIAPP_H

#include "VGUI.h"

namespace vgui
{

class Frame;

class VGUIAPI MiniApp
{
public:
	MiniApp();

	virtual void getName( char* name, int nameLen );
	virtual Frame* createInstance() = 0;

protected:
	virtual void setName( const char* name );

private:
	char*	_name;

public:
	MiniApp& operator=( const MiniApp& ) = default;
};

}

#endif
