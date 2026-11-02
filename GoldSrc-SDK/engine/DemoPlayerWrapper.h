#pragma once

#include "IDemoPlayer.h"

int		DemoPlayer_Init();
int		DemoPlayer_IsActive();
void	DemoPlayer_SetActive( int state );
void	DemoPlayer_SetEditMode( int state );
int		DemoPlayer_LoadGame( char* filename );
void	DemoPlayer_Stop();
void	DemoPlayer_ShowGUI();
int		DemoPlayer_ReadDemoMessage( byte* buffer, int size );
void	DemoPlayer_GetDemoViewInfo( ref_params_t* parameters, float* view, int* viewmodel );
void	DemoPlayer_ReadNetchanState( int* incomingSequence, int* incomingAcknowledged,
			int* incomingReliableAcknowledged, int* incomingReliableSequence, int* outgoingSequence, int* reliableSequence,
			int* lastReliableSequence );
