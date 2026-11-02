#include "IDemoPlayer.h"
#include "SystemWrapper.h"
#include "common.h"
#include "conprint.h"
#include "vgui_intwrap2.h"

static IDemoPlayer* s_DemoPlayer;
static int s_DemoPlayerMode;

void DemoPlayer_SetActive( int state )
{
	s_DemoPlayerMode = state;
}

int DemoPlayer_IsActive( void )
{
	return s_DemoPlayer && s_DemoPlayerMode;
}

int DemoPlayer_LoadGame( char* filename )
{
	return s_DemoPlayer && s_DemoPlayer->LoadGame( filename );
}

void DemoPlayer_SetEditMode( int state )
{
	if ( s_DemoPlayer )
		s_DemoPlayer->SetEditMode( state != 0 );
}

int DemoPlayer_ReadDemoMessage( byte* buffer, int size )
{
	if ( s_DemoPlayer )
		return s_DemoPlayer->ReadDemoMessage( buffer, size );

	return 0;
}

void DemoPlayer_GetDemoViewInfo( ref_params_t* parameters, float* view, int* viewmodel )
{
	if ( s_DemoPlayer )
		s_DemoPlayer->GetDemoViewInfo( parameters, view, viewmodel );
}

void DemoPlayer_ReadNetchanState( int* incomingSequence, int* incomingAcknowledged, int* incomingReliableAcknowledged,
	int* incomingReliableSequence, int* outgoingSequence, int* reliableSequence, int* lastReliableSequence )
{
	if ( s_DemoPlayer )
	{
		s_DemoPlayer->ReadNetchanState( incomingSequence, incomingAcknowledged, incomingReliableAcknowledged,
			incomingReliableSequence, outgoingSequence, reliableSequence, lastReliableSequence );
	}
}

void DemoPlayer_Stop( void )
{
	if ( s_DemoPlayer )
		s_DemoPlayer->Stop();
}

void DemoPlayer_ShowGUI( void )
{
	if ( s_DemoPlayer )
		VGuiWrap2_ShowDemoPlayer();
}

int DemoPlayer_Init( void )
{
	ISystemModule* module = gSystemWrapper.GetModule( "demoplayer001", "demoplayer", NULL );

	s_DemoPlayerMode = 0;
	s_DemoPlayer = (IDemoPlayer*)module;

	if ( module )
	{
		s_DemoPlayer->SetEditMode( COM_CheckParm( "-demoedit" ) != 0 );
		return 1;
	}

	Con_DPrintf( "Failed to load demo player module.\n" );
	return 0;
}
