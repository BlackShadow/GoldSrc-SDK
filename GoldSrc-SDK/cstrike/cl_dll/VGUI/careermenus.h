#pragma once

namespace vgui2
{
class Label;
}

struct CareerMatchStatus
{
	int	ctWins;
	int	tWins;
	int	neededWins;
	int	winBy;
	int	roundResult;
};

extern CareerMatchStatus TheCareerMatchStatus;

void	ShowCareerRoundEndMenu( int ctWins, int tWins, int neededWins, int winBy, int roundResult );
void	ShowCareerMatchEndMenu( int ctWins, int tWins, int neededWins, int winBy, int roundResult );
void	PopulateWinsAndLosses( bool playingCT, vgui2::Label* wins, vgui2::Label* losses );
