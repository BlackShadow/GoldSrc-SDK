#include "DemoEditDialog.h"

#include "LabeledCommandComboBox.h"
#include "Button.h"
#include "Label.h"
#include "TextEntry.h"
#include "controls.h"
#include "vgui/ISurface.h"
#include "../HLTV/common/common.h"
#include "../HLTV/DemoPlayer/private/common/IEngineWrapper.h"
#include "../HLTV/DemoPlayer/private/HLTV/common/IDemoPlayer.h"

#include <stdio.h>
#include <string.h>

extern "C" const char* DRC_CMD_NAME[16];

using namespace vgui2;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDemoEditDialog::CDemoEditDialog( Panel* parent, const char* name, IEngineWrapper* engine, IDemoPlayer* demoPlayer )
	: Frame( parent, name, true, false )
{
	SetHDProportional( true );
	m_DemoPlayer = demoPlayer;
	m_Engine = engine;
	m_ChangingData = false;
	SetBounds( 0, 0, GetProportionalScaledValue( 336 ), GetProportionalScaledValue( 380 ) );
	SetSizeable( false );
	surface()->CreatePopup( GetVPanel(), false, true, false, true, true );
	SetTitle( "#GameUI_EditDirectorCommand", true );
	m_TypeBox = new CLabeledCommandComboBox( this, "TypeBox" );
	for ( int type = DRC_CMD_CAMERA; type <= DRC_CMD_CAMPATH; type++ )
		m_TypeBox->AddItem( DRC_CMD_NAME[type], "" );
	m_TypeBox->SetInitialItem( 0 );
	m_TimeStamp = new TextEntry( this, "TimeEntry" );
	new Button( this, "OkButton", "#GameUI_OK" );
	new Button( this, "CancelButton", "#GameUI_Cancel" );
	new Button( this, "GetTimeButton", "#GameUI_GetTime" );
	m_GetViewButton = new Button( this, "GetViewButton", "#GameUI_GetView" );
	int offset = GetProportionalScaledValue( 96 );
	for ( int i = 0; i < MAX_DEMO_DATA_FIELDS; i++ )
	{
		char fieldName[16];
		sprintf( fieldName, "DataLabel%i", i );
		m_DataLabels[i] = new Label( this, fieldName, "" );
		m_DataLabels[i]->SetBounds( GetProportionalScaledValue( 10 ), offset,
			GetProportionalScaledValue( 96 ), GetProportionalScaledValue( 24 ) );
		sprintf( fieldName, "DataEntry%i", i );
		m_DataEntries[i] = new TextEntry( this, fieldName );
		m_DataEntries[i]->SetBounds( GetProportionalScaledValue( 104 ), offset,
			GetProportionalScaledValue( 224 ), GetProportionalScaledValue( 24 ) );
		offset += GetProportionalScaledValue( 30 );
	}
	LoadControlSettings( "Resource/DemoEditDialog.res", NULL );
	m_DrcCmd.Clear();
	OnUpdate();
}

CDemoEditDialog::~CDemoEditDialog()
{
}

void CDemoEditDialog::SetCommand( DirectorCmd* command )
{
	m_DrcCmd.Copy( command );
	OnUpdate();
}

void CDemoEditDialog::OnCommand( const char* command )
{
	if ( !strcmp( command, "ok" ) )
		OnOK();
	else if ( !strcmp( command, "getview" ) )
		OnGetView();
	else if ( !strcmp( command, "gettime" ) )
		OnGetTime();
	Frame::OnCommand( command );
}

void CDemoEditDialog::OnOK()
{
	if ( m_DemoPlayer )
	{
		UpdateDrcCmd(); // transfer data from text field to m_DrcCmd
		if ( m_DrcCmd.m_Index > 0 )
			m_DemoPlayer->RemoveCommand( m_DrcCmd.m_Index ); // remove old command first
		m_DemoPlayer->AddCommand( &m_DrcCmd );
	}
	Frame::OnCommand( "Close" );
}

void CDemoEditDialog::OnGetView()
{
	if ( !m_Engine )
		return;
	vec3_t origin, angles;
	float fov;
	int entity;
	if ( m_DrcCmd.GetType() == DRC_CMD_CAMERA )
		m_DrcCmd.GetCameraData( origin, angles, fov, entity );
	else if ( m_DrcCmd.GetType() == DRC_CMD_CAMPATH )
		m_DrcCmd.GetCamPathData( origin, angles, fov, entity );
	else
		return;
	m_Engine->GetViewOrigin( origin );
	m_Engine->GetViewAngles( angles );
	if ( m_DrcCmd.GetType() == DRC_CMD_CAMERA )
		m_DrcCmd.SetCameraData( origin, angles, fov, entity );
	else if ( m_DrcCmd.GetType() == DRC_CMD_CAMPATH )
		m_DrcCmd.SetCamPathData( origin, angles, fov, entity );
	OnUpdate();
}

void CDemoEditDialog::OnGetTime()
{
	if ( m_DemoPlayer )
	{
		float time = (float)m_DemoPlayer->GetWorldTime();
		m_DrcCmd.SetTime( time );
		OnUpdate();
	}
}

void CDemoEditDialog::ClearDataFields()
{
	for ( int i = 0; i < MAX_DEMO_DATA_FIELDS; i++ )
	{
		m_DataLabels[i]->SetText( "" );
		m_DataLabels[i]->SetVisible( false );
		m_DataEntries[i]->SetText( "" );
		m_DataEntries[i]->SetVisible( false );
	}
}

void CDemoEditDialog::SetDataField( int index, char* label, char* text )
{
	m_DataLabels[index]->SetText( label );
	m_DataLabels[index]->SetVisible( true );
	m_DataEntries[index]->SetText( text );
	m_DataEntries[index]->SetVisible( true );
}

void CDemoEditDialog::SetDataFieldf( int index, char* label, float value )
{
	m_DataLabels[index]->SetText( label );
	m_DataLabels[index]->SetVisible( true );
	m_DataEntries[index]->SetText( COM_VarArgs( "%.1f", value ) );
	m_DataEntries[index]->SetVisible( true );
}

void CDemoEditDialog::SetDataFieldi( int index, char* label, int value )
{
	m_DataLabels[index]->SetText( label );
	m_DataLabels[index]->SetVisible( true );
	m_DataEntries[index]->SetText( COM_VarArgs( "%d", value ) );
	m_DataEntries[index]->SetVisible( true );
}

char* CDemoEditDialog::GetDataFields( int index )
{
	static char text[1024];
	m_DataEntries[index]->GetText( text, sizeof( text ) );
	return text;
}

int CDemoEditDialog::GetDataFieldx( int index )
{
	char text[1024];
	unsigned int value = 0;
	m_DataEntries[index]->GetText( text, sizeof( text ) );
	(void)sscanf( text, "%x", &value );
	return (int)value;
}

int CDemoEditDialog::GetDataFieldi( int index )
{
	char text[1024];
	int value = 0;
	m_DataEntries[index]->GetText( text, sizeof( text ) );
	(void)sscanf( text, "%d", &value );
	return value;
}

float CDemoEditDialog::GetDataFieldf( int index )
{
	char text[1024];
	float value = 0.0f;
	m_DataEntries[index]->GetText( text, sizeof( text ) );
	(void)sscanf( text, "%f", &value );
	return value;
}

void CDemoEditDialog::GetDataField3f( int index, float* first, float* second, float* third )
{
	char text[1024];
	m_DataEntries[index]->GetText( text, sizeof( text ) );
	(void)sscanf( text, "%f %f %f", first, second, third );
}

void CDemoEditDialog::GetDataField2f( int index, float* first, float* second )
{
	char text[1024];
	m_DataEntries[index]->GetText( text, sizeof( text ) );
	(void)sscanf( text, "%f %f", first, second );
}

void CDemoEditDialog::OnTypeChanged()
{
	if ( m_ChangingData )
		return;	// don't react on own changes done by code, only user input
	int index = m_DrcCmd.m_Index;
	// keep command time & index
	float time = m_DrcCmd.m_Time;
	m_DrcCmd.Clear();
	m_DrcCmd.m_Index = index;
	m_DrcCmd.m_Time = time;
	char type[256];
	m_TypeBox->GetText( type, sizeof( type ) );
	for ( int i = DRC_CMD_NONE; i <= DRC_CMD_CAMPATH; i++ )
	{
		if ( !strcmp( type, DRC_CMD_NAME[i] ) )
		{
			if ( i == m_DrcCmd.m_Type )
				return;
			m_DrcCmd.m_Type = i;
			break;
		}
	}
	vec3_t position = { 0.0f, 0.0f, 0.0f };
	switch ( m_DrcCmd.m_Type )
	{
	case DRC_CMD_EVENT:
		m_DrcCmd.SetEventData( 1, 0, 0 );
		break;
	case DRC_CMD_MODE:
		m_DrcCmd.SetModeData( 4 );
		break;
	case DRC_CMD_CAMERA:
		m_DrcCmd.SetCameraData( position, position, 90.0f, 0 );
		break;
	case DRC_CMD_TIMESCALE:
		m_DrcCmd.SetTimeScaleData( 1.0f );
		break;
	case DRC_CMD_MESSAGE:
		position[0] = 0.5f;
		position[1] = 0.9f;
		m_DrcCmd.SetMessageData( 0, 0xFF, position, 1.0f, 1.0f, 5.0f, 0.0f, "Add your message here" );
		break;
	case DRC_CMD_SOUND:
		m_DrcCmd.SetSoundData( "barney/heybuddy.wav", 1.0f );
		break;
	case DRC_CMD_STATUS:
		m_DrcCmd.SetStatusData( 0, 0, 0 );
		break;
	case DRC_CMD_BANNER:
		m_DrcCmd.SetBannerData( "gfx/temp/file.tga" );
		break;
	case DRC_CMD_STUFFTEXT:
		m_DrcCmd.SetStuffTextData( "echo Enter command here" );
		break;
	case DRC_CMD_CHASE:
		m_DrcCmd.SetChaseData( 1, 0, 96.0f, 0 );
		break;
	case DRC_CMD_INEYE:
		m_DrcCmd.SetInEyeData( 1 );
		break;
	case DRC_CMD_MAP:
		m_DrcCmd.SetMapData( 1, 90.0f, 128.0f );
		break;
	case DRC_CMD_CAMPATH:
		m_DrcCmd.SetCamPathData( position, position, 90.0f, 0 );
		break;
	}
	OnUpdate();
}

void CDemoEditDialog::UpdateDrcCmd()
{
	char text[1204];
	m_TimeStamp->GetText( text, sizeof( text ) );
	for ( char* cursor = text; *cursor; cursor++ )
	{
		if ( *cursor == ':' )
			*cursor = ' ';
	}
	int minutes = 0, seconds = 0, hundredths = 0;
	(void)sscanf( text, "%d %d %d", &minutes, &seconds, &hundredths );
#ifdef _WIN32
	float time = (float)( seconds + 60 * minutes ) + (float)hundredths / 100.0f;
#else
	float time = (float)( (double)( seconds + 60 * minutes ) + (double)hundredths / 100.0 );
#endif
	m_DrcCmd.SetTime( time );
	vec3_t position = { 0.0f, 0.0f, 0.0f }, angles = { 0.0f, 0.0f, 0.0f };
	int first, second, flags;
	float fov, fadein, fadeout, holdtime, fxtime, angle, distance;
	char* name;
	switch ( m_DrcCmd.GetType() )
	{
	case DRC_CMD_EVENT:
		first = GetDataFieldi( 0 );
		second = GetDataFieldi( 1 ); // Target 2"
		flags = GetDataFieldi( 2 ); // Flags
		m_DrcCmd.SetEventData( first, second, flags );
		break;
	case DRC_CMD_MODE:
		m_DrcCmd.SetModeData( GetDataFieldi( 0 ) );
		break;
	case DRC_CMD_CAMERA:
		GetDataField3f( 0, &position[0], &position[1], &position[2] ); // Position
		GetDataField3f( 1, &angles[0], &angles[1], &angles[2] ); // View Angle
		fov = GetDataFieldf( 2 ); // FOV
		first = GetDataFieldi( 3 ); // Flags
		m_DrcCmd.SetCameraData( position, angles, fov, first );
		break;
	case DRC_CMD_TIMESCALE:
		m_DrcCmd.SetTimeScaleData( GetDataFieldf( 0 ) );
		break;
	case DRC_CMD_MESSAGE:
		first = GetDataFieldi( 0 );
		second = GetDataFieldx( 1 ); // Color
		GetDataField2f( 2, &position[0], &position[1] ); // Position
		fadein = GetDataFieldf( 3 ); // Fade In
		fadeout = GetDataFieldf( 4 ); // Fade Out"
		holdtime = GetDataFieldf( 5 ); // Hold Time
		fxtime = GetDataFieldf( 6 ); // FX Time
		m_DrcCmd.SetMessageData( first, (unsigned int)second, position, fadein, fadeout, holdtime,
			fxtime, GetDataFields( 7 ) );
		break;
	case DRC_CMD_SOUND:
		name = GetDataFields( 0 );
		fov = GetDataFieldf( 1 ); // Volume
		m_DrcCmd.SetSoundData( name, fov );
		break;
	case DRC_CMD_STATUS:
		m_DrcCmd.SetStatusData( 0, 0, 0 );
		break;
	case DRC_CMD_BANNER:
		m_DrcCmd.SetBannerData( GetDataFields( 0 ) );
		break;
	case DRC_CMD_STUFFTEXT:
		m_DrcCmd.SetStuffTextData( GetDataFields( 0 ) );
		break;
	case DRC_CMD_CHASE:
		first = GetDataFieldi( 0 );
		second = GetDataFieldi( 1 );
		distance = GetDataFieldf( 2 );
		flags = GetDataFieldi( 3 );
		m_DrcCmd.SetChaseData( first, second, distance, flags );
		break;
	case DRC_CMD_INEYE:
		m_DrcCmd.SetInEyeData( GetDataFieldi( 0 ) );
		break;
	case DRC_CMD_MAP:
		first = GetDataFieldi( 0 );
		angle = GetDataFieldf( 1 );
		distance = GetDataFieldf( 2 );
		m_DrcCmd.SetMapData( first, angle, distance );
		break;
	case DRC_CMD_CAMPATH:
		GetDataField3f( 0, &position[0], &position[1], &position[2] );
		GetDataField3f( 1, &angles[0], &angles[1], &angles[2] );
		fov = GetDataFieldf( 2 );
		flags = GetDataFieldi( 3 );
		m_DrcCmd.SetCamPathData( position, angles, fov, flags );
		break;
	}
}

void CDemoEditDialog::OnUpdate()
{
	m_ChangingData = true;
	m_GetViewButton->SetVisible( false );
	ClearDataFields();	//  prophylactic
	int type = m_DrcCmd.GetType();
	float time = m_DrcCmd.GetTime();
	int seconds = (int)time;
	m_TypeBox->SetInitialItem( type - DRC_CMD_CAMERA );
	float hundredths = time * 100.0f;
	char text[1024];
	sprintf( text, "%03u:%02u:%02u", (unsigned int)( seconds / 60 ),
		(unsigned int)( seconds % 60 ), (unsigned int)( (int)hundredths % 100 ) );
	m_TimeStamp->SetText( text );
	vec3_t position, angles;
	int first, second, flags;
	float fov, fadein, fadeout, holdtime, fxtime;
	switch ( type )
	{
	case DRC_CMD_EVENT:
		m_DrcCmd.GetEventData( first, second, flags );
		SetDataFieldi( 0, "Target 1", first );
		SetDataFieldi( 1, "Target 2", second );
		SetDataFieldi( 2, "Flags", flags );
		break;
	case DRC_CMD_MODE:
		m_DrcCmd.GetModeData( first );
		SetDataField( 0, "View Mode", COM_VarArgs( "%i", first ) );
		break;
	case DRC_CMD_CAMERA:
		m_DrcCmd.GetCameraData( position, angles, fov, first );
		SetDataField( 0, "Position", COM_VarArgs( "%.1f %.1f %.1f", position[0], position[1], position[2] ) );
		SetDataField( 1, "View Angle", COM_VarArgs( "%.1f %.1f %.1f", angles[0], angles[1], angles[2] ) );
		SetDataFieldf( 2, "FOV", fov );
		SetDataFieldi( 3, "Target", first );
		m_GetViewButton->SetVisible( true );
		break;
	case DRC_CMD_TIMESCALE:
		m_DrcCmd.GetTimeScaleData( fov );
		SetDataField( 0, "Time Scale", COM_VarArgs( "%.2f", fov ) );
		break;
	case DRC_CMD_MESSAGE:
		m_DrcCmd.GetMessageData( first, second, position, fadein, fadeout, holdtime, fxtime, text );
		SetDataFieldi( 0, "Effect", first );
		SetDataField( 1, "Color", COM_VarArgs( "%x", (unsigned int)second ) );
		SetDataField( 2, "Position", COM_VarArgs( "%.2f %.2f", position[0], position[1] ) );
		SetDataFieldf( 3, "Fade In", fadein );
		SetDataFieldf( 4, "Fade Out", fadeout );
		SetDataFieldf( 5, "Hold Time", holdtime );
		SetDataFieldf( 6, "FX Time", fxtime );
		SetDataField( 7, "Text", text );
		break;
	case DRC_CMD_SOUND:
		m_DrcCmd.GetSoundData( text, fov );
		SetDataField( 0, "Name", text );
		SetDataField( 1, "Volume", COM_VarArgs( "%.2f", fov ) );
		break;
	case DRC_CMD_STATUS:
		m_DrcCmd.GetStatusData( first, second, flags );
		break;
	case DRC_CMD_BANNER:
		m_DrcCmd.GetBannerData( text );
		SetDataField( 0, "File", text );
		break;
	case DRC_CMD_STUFFTEXT:
		m_DrcCmd.GetStuffTextData( text );
		SetDataField( 0, "Command", text );
		break;
	case DRC_CMD_CHASE:
		m_DrcCmd.GetChaseData( first, second, fov, flags );
		SetDataFieldi( 0, "Target 1", first );
		SetDataFieldi( 1, "Target 2", second );
		SetDataFieldf( 2, "Distance", fov );
		SetDataFieldi( 3, "Flags", flags );
		break;
	case DRC_CMD_INEYE:
		m_DrcCmd.GetInEyeData( first );
		SetDataFieldi( 0, "Player", first );
		break;
	case DRC_CMD_MAP:
		m_DrcCmd.GetMapData( first, fov, fadein );
		SetDataFieldi( 0, "Player", first );
		SetDataFieldf( 1, "Angle", fov );
		SetDataFieldf( 2, "Distance", fadein );
		break;
	case DRC_CMD_CAMPATH:
		m_DrcCmd.GetCamPathData( position, angles, fov, flags );
		SetDataField( 0, "Position", COM_VarArgs( "%.1f %.1f %.1f", position[0], position[1], position[2] ) );
		SetDataField( 1, "View Angle", COM_VarArgs( "%.1f %.1f %.1f", angles[0], angles[1], angles[2] ) );
		SetDataFieldf( 2, "FOV", fov );
		SetDataFieldi( 3, "Flags", flags );
		m_GetViewButton->SetVisible( true );
		break;
	}
	m_ChangingData = false;
}
