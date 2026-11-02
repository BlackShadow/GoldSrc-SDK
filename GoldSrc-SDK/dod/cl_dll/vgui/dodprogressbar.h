#ifndef DODPROGRESSBAR_H
#define DODPROGRESSBAR_H

#include "ProgressBar.h"

namespace vgui2
{
class DoDProgressBar : public ProgressBar
{
public:
	typedef ProgressBar BaseClass;
	DoDProgressBar( Panel* parent, const char* panelName );
	void ApplySchemeSettings( IScheme* scheme ) override;
};
}

#endif
