#pragma once

#include <windows.h>

#define MIXER_FILTER_DELAY_CAPACITY	34

typedef struct mixer_filter_channel_s
{
	float	previousInput;
	float	previousCrossfeed;
	float	delay[MIXER_FILTER_DELAY_CAPACITY];
	float	responseState;
	float	previousResponseState;
	float	previousResponseOutput;
} mixer_filter_channel_t;

typedef struct mixer_filter_state_s
{
	DWORD					sampleRate;
	DWORD					reserved;
	int						delayIndex;
	float					previousFrames[4];
	DWORD					unusedFilterState[28];
	mixer_filter_channel_t	channels[2];
} mixer_filter_state_t;

void	Mixer_ResetFloatState();
void	__cdecl Mixer_InitializeFilters( void* pUnused, mixer_filter_state_t* pState, DWORD sampleRate );
int		__cdecl Mixer_FilterStereo( float* pSamples, SHORT mode, mixer_filter_state_t* pState, DWORD frameCount );
void	__cdecl Mixer_FloatToPcm16( const float* pInput, short* pOutput, DWORD sampleCount );
