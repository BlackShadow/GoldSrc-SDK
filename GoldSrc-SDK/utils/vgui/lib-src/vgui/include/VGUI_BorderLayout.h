#ifndef VGUI_BORDERLAYOUT_H
#define VGUI_BORDERLAYOUT_H

#include "VGUI_Layout.h"
#include "VGUI_LayoutInfo.h"

namespace vgui
{

class Panel;

class VGUIAPI BorderLayout : public Layout
{
public:
	enum Alignment
	{
		a_center	= 0,
		a_north		= 1,
		a_south		= 2,
		a_east		= 3,
		a_west		= 4
	};

	BorderLayout( int inset );

	virtual LayoutInfo* createLayoutInfo( Alignment alignment );
	virtual void performLayout( Panel* panel ) override;

private:
	int	_inset;

public:
	BorderLayout& operator=( const BorderLayout& ) = default;
};

class BorderLayoutInfo : public LayoutInfo
{
public:
	virtual LayoutInfo* getThis() override;

	BorderLayout::Alignment	_alignment;

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

}

#endif
