#pragma once

#include <windows.h>

#define A3D_MIX_MAX_FRAMES		4096
#define A3D_MIX_FILTER_TAPS		50
#define A3D_MIX_HISTORY_FRAMES	52
#define A3D_MIX_STEREO			0x02
#define A3D_MIX_16BIT				0x10
#define A3D_MIX_LOOPING			0x40

#define A3D_MIXER_RATE_SHIFT			15
#define A3D_MIXER_MAX_RATE_MULTIPLIER	10

typedef struct a3d_mix_input_s
{
	BYTE*	pSamples;
	BYTE*	pEnd;
	DWORD	cFrames;
	BYTE*	pCurrent;
	DWORD	dwFlags;
} a3d_mix_input_t;

typedef struct a3d_mix_channel_s a3d_mix_channel_t;
typedef struct a3d_mix_voice_s a3d_mix_voice_t;
typedef void ( __cdecl* a3d_mix_read_pair_t )( const BYTE*, LONG, double[2] );
typedef void ( __cdecl* a3d_mix_read_quad_t )( const BYTE*, LONG, LONG, double[4] );
typedef DWORD ( __cdecl* a3d_mix_channel_kernel_t )( float*, a3d_mix_channel_t*, a3d_mix_voice_t*, DWORD );

struct a3d_mix_channel_s
{
	LONG	lGainCurrent;
	LONG	lGainTarget;
	LONG	lDelayCurrent;
	LONG	lDelayTarget;
	DWORD	dwPhaseFraction;
	LONG	iSourceSample;
	LONG	cFilterTaps;
	float	coefficients[A3D_MIX_FILTER_TAPS];
	float	targetCoefficients[A3D_MIX_FILTER_TAPS];
	float	coefficientSteps[A3D_MIX_FILTER_TAPS];
	float	history[A3D_MIX_HISTORY_FRAMES];
	LONG	lGainBase;
	LONG	lGainSpatial;
};

struct a3d_mix_voice_s
{
	const BYTE*					pSamples;
	LONG						cSourceBytes;
	LONG						lRateCurrent;
	LONG						lRateRequested;
	LONG						lRateTarget;
	BOOL						bLooping;
	LONG						iSourceByte;
	DWORD						dwSampleRate;
	DWORD						dwReserved;
	DWORD						cSampleShift;
	a3d_mix_read_pair_t			pfnReadPair;
	a3d_mix_read_pair_t			pfnReadPairLoop;
	a3d_mix_channel_kernel_t		pfnMixChannel;
	a3d_mix_read_quad_t			pfnReadQuad;
	a3d_mix_read_quad_t			pfnReadQuadLoop;
	a3d_mix_read_quad_t			pfnReadQuadTail;
	a3d_mix_channel_kernel_t		pfnResampleChannel;
	a3d_mix_channel_t			channels[2];
};

DWORD	__cdecl AR2DSetFrequency( a3d_mix_voice_t* pVoice, DWORD dwFrequency );
SHORT	__cdecl AR2Dinit( a3d_mix_voice_t* pVoice, a3d_mix_input_t* pInput, WORD wSampleRate );
SHORT	__cdecl AR2Dsetposition( a3d_mix_voice_t* pVoice, a3d_mix_input_t* pInput, DWORD dwPosition );
SHORT	__cdecl AR2Dproc( a3d_mix_input_t* pInput, float* pOutput, a3d_mix_voice_t* pVoice,
		DWORD cFrames, BOOL bFilter );
LONG	__cdecl Mixer_PrepareChannelRate( a3d_mix_channel_t* pChannel, a3d_mix_voice_t* pVoice,
		DWORD cFrames, LONG* plRateStep );
void	__cdecl Mixer_SetImpulseResponse( a3d_mix_channel_t* pChannel, const SHORT* pCoefficients, DWORD cTaps );
