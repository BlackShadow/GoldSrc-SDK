#include "hud.h"
#include "cl_util.h"
#include "vgui_StatsMenu.h"
#include "parsemsg.h"

#include <VGUI_TextImage.h>

char* sCTFStatsSelection[4] = { "Both", "BlackMesa", "OpposingForce", "Individual" };
char* sLocalisedStatsTeams[4] =
{
	"#CTFTitleStats_Both", "#CTFTitleStats_BlackMesa", "#CTFTitleStats_OpposingForce", "#CTFTitleStats_Individual"
};

typedef struct
{
	unsigned char bChunksRead;
	char szMVP[32];
	char szMostKills[32];
	char szMostCTF[32];
	char szMostOff[32];
	char szMostDef[32];
	char szMostSnipe[32];
	char szMostBarnacle[32];
	char szMostDeaths[32];
	char szMostSuicides[32];
	char szMostDamage[32];
	char szMostAccel[32];
	char szMostBackpack[32];
	char szMostHealth[32];
	char szMostShield[32];
	char szMostJump[32];
	int iMVPVal;
	int iKillsVal;
	int iCTFVal;
	int iOffVal;
	int iDefVal;
	int iSnipeVal;
	int iBarnacleVal;
	int iDeathsVal;
	int iSuicidesVal;
	int iDamageVal;
	int iAccelVal;
	int iBackpackVal;
	int iHealthVal;
	int iShieldVal;
	int iJumpVal;
} team_stat_info_t;

static int g_iWinningTeam;
static team_stat_info_t g_TeamStatInfo[3];
static char szStatsBuf[4096];

class CMenuHandler_SaveStats : public ActionSignal
{
public:
	void actionPerformed( Panel* ) override
	{
		gViewPort->SaveStatsMenu();
	}
};

CStatsMenuPanel::CStatsMenuPanel( int transparency, int remove, int x, int y, int wide, int tall ) :
	CMenuPanel( transparency, remove, x, y, wide, tall )
{
	memset( m_pClassImages, 0, sizeof( m_pClassImages ) );
	CSchemeManager* schemes = gViewPort->GetSchemeManager();
	SchemeHandle_t titleScheme = schemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t textScheme = schemes->getSchemeHandle( "Briefing Text" );
	SchemeHandle_t statsScheme = schemes->getSchemeHandle( "CommandMenu Text" );
	int r, g, b, a;
	Label* title = new Label( "", XRES( 40 ), YRES( 32 ) );
	title->setParent( this );
	title->setFont( schemes->getFont( titleScheme ) );
	schemes->getFgColor( titleScheme, r, g, b, a );
	title->setFgColor( r, g, b, a );
	schemes->getBgColor( titleScheme, r, g, b, a );
	title->setBgColor( r, g, b, a );
	title->setContentAlignment( Label::a_west );
	title->setText( CHudTextMessage::BufferedLocaliseTextString( "#CTFTitle_EndGameStats" ) );
	m_pScrollPanel = new CTFScrollPanel( XRES( 40 ), YRES( 80 ), XRES( 600 ), YRES( 312 ) );
	m_pScrollPanel->setParent( this );
	m_pScrollPanel->setScrollBarAutoVisible( false, false );
	m_pScrollPanel->setScrollBarVisible( true, true );
	m_pScrollPanel->setBorder( new LineBorder( Color( 0, 112, 0, 0 ) ) );
	m_pScrollPanel->validate();
	int clientWide = m_pScrollPanel->getClient()->getWide();
	m_pScrollPanel->setScrollBarAutoVisible( false, true );
	m_pScrollPanel->setScrollBarVisible( false, false );
	m_pScrollPanel->validate();
	for ( int i = 0; i < 4; i++ )
	{
		char text[256];
		strcpy( text, CHudTextMessage::BufferedLocaliseTextString( sLocalisedStatsTeams[i] ) );
		m_pButtons[i] = new CommandButton( text, ( XRES( 124 ) + YRES( 8 ) ) * i + XRES( 40 ),
			YRES( 400 ), XRES( 124 ), YRES( 24 ), true );
		m_pButtons[i]->setBoundKey( '0' + i );
		m_pButtons[i]->setContentAlignment( Label::a_west );
		m_pButtons[i]->addInputSignal( new CHandler_MenuButtonOver( this, i ) );
		m_pButtons[i]->setParent( this );
		m_pClassInfoPanel[i] = new CTransparentPanel( 255, 0, 0, clientWide, YRES( 312 ) );
		m_pClassInfoPanel[i]->setParent( m_pScrollPanel->getClient() );
		int textOffset = XRES( 8 );
		sprintf( text, "#CTFTitleStats_%s", sCTFStatsSelection[i] );
		char* heading = CHudTextMessage::BufferedLocaliseTextString( text );
		Label* infoTitle = new Label( "", textOffset, YRES( 8 ) );
		infoTitle->setFont( schemes->getFont( titleScheme ) );
		infoTitle->setParent( m_pClassInfoPanel[i] );
		schemes->getFgColor( titleScheme, r, g, b, a );
		infoTitle->setFgColor( r, g, b, a );
		schemes->getBgColor( titleScheme, r, g, b, a );
		infoTitle->setBgColor( r, g, b, a );
		infoTitle->setContentAlignment( Label::a_west );
		infoTitle->setText( heading );
		if ( ScreenWidth >= 640 )
		{
			int titleWide, titleTall;
			infoTitle->getTextSize( titleWide, titleTall );
			strcpy( text, sCTFStatsSelection[i == 3 ? 1 : i] );
			m_pClassImages[i] = new CImageLabel( text, textOffset, 2 * titleTall, XRES( 250 ), YRES( 80 ) );
			m_pClassImages[i]->setParent( m_pClassInfoPanel[i] );
			textOffset += m_pClassImages[i]->getImageWide() + XRES( 8 );
		}
		m_pStatsWindow[i] = new TextPanel( i == 3 ? "Spectator" : "No players on this team",
			textOffset, YRES( 80 ), XRES( 250 ), YRES( 80 ) );
		m_pStatsWindow[i]->setParent( m_pClassInfoPanel[i] );
		m_pStatsWindow[i]->setFont( schemes->getFont( statsScheme ) );
		schemes->getFgColor( textScheme, r, g, b, a );
		m_pStatsWindow[i]->setFgColor( r, g, b, a );
		schemes->getBgColor( textScheme, r, g, b, a );
		m_pStatsWindow[i]->setBgColor( r, g, b, a );
		int xx, yy;
		m_pStatsWindow[i]->getPos( xx, yy );
		int infoWide = xx + wide;
		int infoTall = yy + tall;
		if ( m_pClassImages[0] )
		{
			m_pClassImages[0]->getPos( xx, yy );
			if ( infoTall < yy + m_pClassImages[0]->getTall() )
				infoTall = yy + m_pClassImages[0]->getTall();
		}
		m_pClassInfoPanel[i]->setSize( infoWide, infoTall );
	}
	m_pCancelButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Menu_Cancel" ),
		XRES( 500 ), YRES( 40 ), XRES( 124 ), YRES( 24 ), false );
	m_pCancelButton->setParent( this );
	m_pCancelButton->addActionSignal( new CMenuHandler_ScoreStatWindow( 0 ) );
	m_pScoreBoardButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#CTFMenu_ScoreBoard" ),
		XRES( 500 ) - YRES( 8 ) - XRES( 124 ), YRES( 40 ), XRES( 124 ), YRES( 24 ), false );
	m_pScoreBoardButton->setParent( this );
	m_pScoreBoardButton->addActionSignal( new CMenuHandler_ScoreStatWindow( 10 ) );
	m_pSaveButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#CTFMenu_SaveStats" ),
		XRES( 500 ) - ( XRES( 124 ) + YRES( 8 ) + XRES( 124 ) + YRES( 8 ) ),
		YRES( 40 ), XRES( 124 ), YRES( 24 ), false );
	m_pSaveButton->setParent( this );
	m_pSaveButton->addActionSignal( new CMenuHandler_SaveStats );
	SetActiveInfo( 0 );
}

void CStatsMenuPanel::Update()
{
}

void CStatsMenuPanel::Open()
{
	Update();
	CMenuPanel::Open();
}

void CStatsMenuPanel::Initialize()
{
	setVisible( false );
	m_pScrollPanel->setScrollValue( 0, 0 );
}

void CStatsMenuPanel::SetActiveInfo( int info )
{
	for ( int i = 0; i < 4; i++ )
	{
		m_pButtons[i]->setArmed( false );
		m_pClassInfoPanel[i]->setVisible( false );
	}
	if ( (unsigned int)info >= 4 )
		info = 0;
	m_pButtons[info]->setArmed( true );
	m_pClassInfoPanel[info]->setVisible( true );
	m_iCurrentInfo = info;
	m_pScrollPanel->setScrollValue( 0, 0 );
	m_pScrollPanel->validate();
}

bool CStatsMenuPanel::SlotInput( int slot )
{
	if ( (unsigned int)( slot - 1 ) <= 2 && m_pButtons[slot] && m_pButtons[slot]->isVisible() )
	{
		m_pButtons[slot]->fireActionSignal();
		return true;
	}
	return false;
}

void CStatsMenuPanel::Reset()
{
	CMenuPanel::Reset();
	m_iCurrentInfo = 0;
}

void CStatsMenuPanel::SetPlayerImage( const char* image )
{
	if ( m_pClassImages[3] )
		m_pClassImages[3]->setImage( image );
}

void CStatsMenuPanel::SaveStats()
{
}

int CStatsMenuPanel::MsgFunc_StatsInfo( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int team = READ_BYTE();
	g_iWinningTeam = READ_BYTE();
	int players = READ_BYTE();
	int chunk = READ_BYTE();
	if ( team == 0 && m_pClassImages[0] )
		m_pClassImages[0]->setImage( sCTFStatsSelection[g_iWinningTeam] );
	team_stat_info_t& stats = g_TeamStatInfo[team];
	if ( chunk == 0 )
	{
		strcpy( stats.szMVP, READ_STRING() );
		stats.iMVPVal = READ_SHORT();
		strcpy( stats.szMostKills, READ_STRING() );
		stats.iKillsVal = READ_SHORT();
		strcpy( stats.szMostCTF, READ_STRING() );
		stats.iCTFVal = READ_SHORT();
		strcpy( stats.szMostOff, READ_STRING() );
		stats.iOffVal = READ_SHORT();
		strcpy( stats.szMostDef, READ_STRING() );
		stats.iDefVal = READ_SHORT();
		strcpy( stats.szMostSnipe, READ_STRING() );
		stats.iSnipeVal = READ_SHORT();
		strcpy( stats.szMostBarnacle, READ_STRING() );
		stats.iBarnacleVal = READ_SHORT();
	}
	else if ( chunk == 1 )
	{
		strcpy( stats.szMostDeaths, READ_STRING() );
		stats.iDeathsVal = READ_SHORT();
		strcpy( stats.szMostSuicides, READ_STRING() );
		stats.iSuicidesVal = READ_SHORT();
		strcpy( stats.szMostDamage, READ_STRING() );
		stats.iDamageVal = READ_SHORT();
		strcpy( stats.szMostAccel, READ_STRING() );
		stats.iAccelVal = READ_SHORT();
		strcpy( stats.szMostBackpack, READ_STRING() );
		stats.iBackpackVal = READ_SHORT();
		strcpy( stats.szMostHealth, READ_STRING() );
		stats.iHealthVal = READ_SHORT();
		strcpy( stats.szMostShield, READ_STRING() );
		stats.iShieldVal = READ_SHORT();
		strcpy( stats.szMostJump, READ_STRING() );
		stats.iJumpVal = READ_SHORT();
	}
	stats.bChunksRead = (unsigned char)( stats.bChunksRead | ( 1 << chunk ) );
	if ( !( stats.bChunksRead & 3 ) )
		return 0;
	char filename[64];
	sprintf( filename, "stats/stats_%s.ost", sCTFStatsSelection[team] );
	char* file = (char*)gEngfuncs.COM_LoadFile( filename, 5, NULL );
	if ( !file )
		return 0;
	char* text = &szStatsBuf[team * 1024];
	if ( players <= 0 )
		strcpy( text, "No players on this team\n" );
	else
	{
		gViewPort->GetAllPlayersInfo();
		sprintf( text, file,
			stats.szMVP, stats.iMVPVal, stats.szMostKills, stats.iKillsVal,
			stats.szMostCTF, stats.iCTFVal, stats.szMostOff, stats.iOffVal,
			stats.szMostDef, stats.iDefVal, stats.szMostSnipe, stats.iSnipeVal,
			stats.szMostBarnacle, stats.iBarnacleVal, stats.szMostDeaths, stats.iDeathsVal,
			stats.szMostSuicides, stats.iSuicidesVal, stats.szMostDamage, stats.iDamageVal,
			stats.szMostAccel, stats.iAccelVal / 60, stats.iAccelVal % 60,
			stats.szMostBackpack, stats.iBackpackVal / 60, stats.iBackpackVal % 60,
			stats.szMostHealth, stats.iHealthVal / 60, stats.iHealthVal % 60,
			stats.szMostShield, stats.iShieldVal / 60, stats.iShieldVal % 60,
			stats.szMostJump, stats.iJumpVal / 60, stats.iJumpVal % 60 );
	}
	m_pStatsWindow[team]->setText( text );
	int wide, tall;
	m_pStatsWindow[team]->getTextImage()->getTextSize( wide, tall );
	m_pStatsWindow[team]->setSize( wide, tall );
	return 0;
}

int CStatsMenuPanel::MsgFunc_StatsPlayer( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	short player = (short)READ_BYTE();
	int mvp = READ_SHORT();
	int kills = READ_SHORT();
	int captures = READ_SHORT();
	int offense = READ_SHORT();
	int defense = READ_SHORT();
	int snipe = READ_SHORT();
	int barnacle = READ_SHORT();
	int deaths = READ_SHORT();
	int suicides = READ_SHORT();
	int damage = READ_SHORT();
	int accelerator = READ_SHORT();
	int backpack = READ_SHORT();
	int health = READ_SHORT();
	int shield = READ_SHORT();
	int jump = READ_SHORT();
	if ( (unsigned short)( player - 1 ) >= MAX_PLAYERS )
		return 0;
	char filename[64];
	sprintf( filename, "stats/stats_%s.ost", sCTFStatsSelection[3] );
	char* file = (char*)gEngfuncs.COM_LoadFile( filename, 5, NULL );
	if ( !file )
		return 0;
	sprintf( &szStatsBuf[3072], file, mvp, kills, captures, offense, defense, snipe, barnacle,
		deaths, suicides, damage, accelerator / 60, accelerator % 60,
		backpack / 60, backpack % 60, health / 60, health % 60,
		shield / 60, shield % 60, jump / 60, jump % 60 );
	m_pStatsWindow[3]->setText( &szStatsBuf[3072] );
	int wide, tall;
	m_pStatsWindow[3]->getTextImage()->getTextSize( wide, tall );
	m_pStatsWindow[3]->setSize( wide, tall );
	return 0;
}
