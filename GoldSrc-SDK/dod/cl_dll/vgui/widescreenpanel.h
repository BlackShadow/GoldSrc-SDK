#ifndef WIDESCREENPANEL_H
#define WIDESCREENPANEL_H

#include "Frame.h"

namespace vgui2 { class Label; }

class CWidescreenPanel : public vgui2::Frame
{
public:
	typedef vgui2::Frame BaseClass;
	CWidescreenPanel( vgui2::Panel* parent );
	void Update();
	void ReadSubtitleFile();
	void setSubtitle( int subtitle );
private:
	vgui2::Label* m_pWideLabel;
	bool m_bSubtitlesLoaded;
	float m_flSubtitleChangeTime;
	int m_iNumSubtitles;
	int m_iSubtitleIndeces[64];
	char m_szSubtitleText[64][1024];
};

#endif
