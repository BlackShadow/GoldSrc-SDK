#include "a2d.h"
#include "mixer_filters.h"
#include "mixer_resample.h"
#include "WallEdge.h"

#include <stdio.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <xmmintrin.h>

#define A3D_MIX_IMPULSE_SCALE	3.0518509476e-05

#define MIXER_PCM8_MIDPOINT		128
#define MIXER_PCM8_TO_PCM16_SCALE	256
#define MIXER_PHASE_FRACTION_BITS	16
#define MIXER_PHASE_FRACTION_MASK	0xffffu
#define MIXER_RESAMPLE_PHASE_BITS	7
#define MIXER_RESAMPLE_PHASES		( 1 << MIXER_RESAMPLE_PHASE_BITS )

static float mixSamples[A3D_MIX_MAX_FRAMES + A3D_MIX_HISTORY_FRAMES];

static __forceinline void Mixer_ReadPair8Mono( const BYTE* pSamples, LONG iSample, double samples[2] )
{
	samples[0] = ( (int)pSamples[iSample] - MIXER_PCM8_MIDPOINT ) * MIXER_PCM8_TO_PCM16_SCALE;
	samples[1] = ( (int)pSamples[iSample + 1] - MIXER_PCM8_MIDPOINT ) * MIXER_PCM8_TO_PCM16_SCALE;
}

static __forceinline void Mixer_ReadPair8MonoLoop( const BYTE* pSamples, LONG iSample, double samples[2] )
{
	samples[0] = ( (int)pSamples[iSample + 1] - MIXER_PCM8_MIDPOINT ) * MIXER_PCM8_TO_PCM16_SCALE;
	samples[1] = ( (int)pSamples[0] - MIXER_PCM8_MIDPOINT ) * MIXER_PCM8_TO_PCM16_SCALE;
}

static __forceinline void Mixer_ReadPair8Stereo( const BYTE* pSamples, LONG iSample, double samples[2] )
{
	int halfSample = 0;
	for ( int i = 0; i < 2; i++ )
	{
		int sum = 0;
		for ( int channel = 0; channel < 2; channel++ )
		{
			halfSample = ( (int)pSamples[( iSample + i ) * 2 + channel] - MIXER_PCM8_MIDPOINT ) *
				( MIXER_PCM8_TO_PCM16_SCALE / 2 ) + (BYTE)halfSample / 2;
			sum += halfSample;
		}
		samples[i] = (SHORT)sum;
	}
}

static __forceinline void Mixer_ReadPair8StereoLoop( const BYTE* pSamples, LONG iSample, double samples[2] )
{
	int halfSample = 0;
	for ( int i = 0; i < 2; i++ )
	{
		int sum = 0;
		LONG sourceSample = i == 0 ? iSample : 0;
		for ( int channel = 0; channel < 2; channel++ )
		{
			halfSample = ( (int)pSamples[sourceSample * 2 + channel] - MIXER_PCM8_MIDPOINT ) *
				( MIXER_PCM8_TO_PCM16_SCALE / 2 ) + (BYTE)halfSample / 2;
			sum += halfSample;
		}
		samples[i] = (SHORT)sum;
	}
}

static __forceinline void Mixer_ReadPair16Stereo( const BYTE* pSamples, LONG iSample, double samples[2] )
{
	const SHORT* pWords = (const SHORT*)pSamples;
	samples[0] = ( (double)pWords[iSample * 2] + pWords[iSample * 2 + 1] ) * 0.5;
	samples[1] = ( (double)pWords[iSample * 2 + 2] + pWords[iSample * 2 + 3] ) * 0.5;
}

static __forceinline void Mixer_ReadPair16StereoLoop( const BYTE* pSamples, LONG iSample, double samples[2] )
{
	const SHORT* pWords = (const SHORT*)pSamples;
	samples[0] = ( (double)pWords[iSample * 2] + pWords[iSample * 2 + 1] ) * 0.5;
	samples[1] = ( (double)pWords[0] + pWords[1] ) * 0.5;
}

LONG __cdecl Mixer_PrepareChannelRate( a3d_mix_channel_t* pChannel, a3d_mix_voice_t* pVoice,
		DWORD cFrames, LONG* plRateStep )
{
	LONG difference = (LONG)( (DWORD)pVoice->lRateTarget - pVoice->lRateCurrent );
	*plRateStep = difference / (LONG)cFrames;
	DWORD targetDelay = Mixer_MultiplyFixed16( (DWORD)pVoice->lRateTarget, (DWORD)pChannel->lDelayTarget );
	LONG delayDifference = (LONG)( (DWORD)pChannel->lDelayCurrent - targetDelay );
	LONG delayStep = delayDifference / (LONG)cFrames;
	pChannel->lDelayCurrent = (LONG)( (DWORD)pChannel->lDelayCurrent - (DWORD)delayStep * cFrames );
	LONG rate = (LONG)( (DWORD)pVoice->lRateCurrent + delayStep );
	if ( rate < 0 )
		__debugbreak();
	return rate;
}

static void Mixer_UpdateRate( a3d_mix_voice_t* pVoice, DWORD cFrames )
{
	LONG difference = (LONG)( (DWORD)pVoice->lRateTarget - pVoice->lRateCurrent );
	LONG step = difference / (LONG)cFrames;
	pVoice->lRateCurrent = (LONG)( (DWORD)pVoice->lRateCurrent + (DWORD)step * cFrames );
	LONG maximum = (LONG)( (DWORD)pVoice->lRateCurrent * A3D_MIXER_MAX_RATE_MULTIPLIER );
	pVoice->lRateTarget = pVoice->lRateRequested;
	if ( pVoice->lRateTarget > maximum )
		pVoice->lRateTarget = maximum;
}

static void smUpdateA3DTrackers( a3d_mix_voice_t* pVoice, DWORD cFrames )
{
	DWORD position = (DWORD)( pVoice->channels[0].lDelayCurrent >> 8 ) *
		(DWORD)( pVoice->lRateTarget >> 8 );
	position = ( position >> MIXER_PHASE_FRACTION_BITS ) + pVoice->channels[0].iSourceSample;
	position <<= pVoice->cSampleShift;
	if ( (LONG)position >= pVoice->cSourceBytes )
	{
		if ( pVoice->bLooping )
			position -= pVoice->cSourceBytes;
		else
			position = pVoice->cSourceBytes;
	}
	pVoice->iSourceByte = (LONG)position;
	Mixer_UpdateRate( pVoice, cFrames );
}

static DWORD Mixer_Mix16Mono( float* pOutput, a3d_mix_channel_t* pChannel,
		a3d_mix_voice_t* pVoice, DWORD cFrames )
{
	LONG lastSample = (LONG)( (DWORD)pVoice->cSourceBytes >> pVoice->cSampleShift );
	if ( !pVoice->bLooping )
		lastSample -= (DWORD)pChannel->lDelayTarget >> MIXER_PHASE_FRACTION_BITS;
	lastSample--;
	LONG rateStep;
	DWORD rate = Mixer_PrepareChannelRate( pChannel, pVoice, cFrames, &rateStep );
	Mixer_ResetFloatState();
	double gain = (double)pChannel->lGainCurrent * ( 1.0 / 65536.0 );
	double gainStep = ( (double)pChannel->lGainTarget - pChannel->lGainCurrent ) *
		( 1.0 / 65536.0 ) / (LONG)cFrames;
	gain += gainStep;
	pChannel->lGainCurrent = pChannel->lGainTarget;
	LONG sample = pChannel->iSourceSample;
	DWORD phase = pChannel->dwPhaseFraction;
	const SHORT* pSamples = (const SHORT*)pVoice->pSamples;
	DWORD remaining = cFrames;
	bool first = true;
	while ( remaining )
	{
		bool boundary = sample == lastSample;
		bool pastEnd = first ? sample > lastSample : (DWORD)sample > (DWORD)lastSample;
		if ( pastEnd )
		{
			if ( !pVoice->bLooping )
				break;
			sample -= lastSample + 1;
			boundary = false;
			if ( (DWORD)sample > (DWORD)lastSample )
				sample = (LONG)( (DWORD)sample % (DWORD)( lastSample + 1 ) );
			boundary = sample == lastSample;
		}
		double current = pSamples[sample];
		double next = boundary ? pSamples[0] : pSamples[sample + 1];
		double phaseGain = ( (double)(LONG)phase * ( 1.0 / 65536.0 ) ) * gain;
		double contribution = current * gain + ( next - current ) * phaseGain;
		gain += gainStep;
		DWORD phaseSum = ( phase & MIXER_PHASE_FRACTION_MASK ) + ( rate & MIXER_PHASE_FRACTION_MASK );
		phase = ( phase & ~MIXER_PHASE_FRACTION_MASK ) | ( phaseSum & MIXER_PHASE_FRACTION_MASK );
		sample = (LONG)( (DWORD)sample + ( rate >> MIXER_PHASE_FRACTION_BITS ) +
			( phaseSum >> MIXER_PHASE_FRACTION_BITS ) );
		rate += rateStep;
		*pOutput = (float)( contribution + *pOutput );
		pOutput += 2;
		remaining--;
		first = false;
	}
	if ( sample > lastSample + 1 )
		sample = lastSample + 1;
	pChannel->iSourceSample = sample;
	pChannel->dwPhaseFraction = phase;
	return remaining;
}

static DWORD Mixer_MixChannel( float* pOutput, a3d_mix_channel_t* pChannel,
		a3d_mix_voice_t* pVoice, DWORD cFrames )
{
	LONG lastSample = (LONG)( (DWORD)pVoice->cSourceBytes >> pVoice->cSampleShift );
	if ( !pVoice->bLooping )
	{
		DWORD delay = (DWORD)( pChannel->lDelayTarget >> 8 ) * (DWORD)( pVoice->lRateTarget >> 8 );
		lastSample -= delay >> MIXER_PHASE_FRACTION_BITS;
	}
	lastSample--;
	LONG rateStep;
	DWORD rate = Mixer_PrepareChannelRate( pChannel, pVoice, cFrames, &rateStep );
	Mixer_ResetFloatState();
	double gain = (double)pChannel->lGainCurrent * ( 1.0 / 65536.0 );
	double gainStep = ( (double)pChannel->lGainTarget - pChannel->lGainCurrent ) *
		( 1.0 / 65536.0 ) / (LONG)cFrames;
	gain += gainStep;
	pChannel->lGainCurrent = pChannel->lGainTarget;
	LONG sample = pChannel->iSourceSample;
	DWORD phase = pChannel->dwPhaseFraction;
	DWORD remaining = cFrames;
	bool first = true;
	while ( remaining )
	{
		bool boundary = sample == lastSample;
		bool pastEnd = first ? sample > lastSample : (DWORD)sample > (DWORD)lastSample;
		if ( pastEnd )
		{
			if ( !pVoice->bLooping )
				break;
			sample -= lastSample + 1;
			boundary = false;
			if ( (DWORD)sample > (DWORD)lastSample )
				sample = (LONG)( (DWORD)sample % (DWORD)( lastSample + 1 ) );
			boundary = sample == lastSample;
		}
		double pair[2];
		if ( pVoice->pfnReadPair == Mixer_ReadPair16Stereo )
		{
			if ( boundary )
				Mixer_ReadPair16StereoLoop( pVoice->pSamples, sample, pair );
			else
				Mixer_ReadPair16Stereo( pVoice->pSamples, sample, pair );
		}
		else if ( pVoice->pfnReadPair == Mixer_ReadPair8Stereo )
		{
			if ( boundary )
				Mixer_ReadPair8StereoLoop( pVoice->pSamples, sample, pair );
			else
				Mixer_ReadPair8Stereo( pVoice->pSamples, sample, pair );
		}
		else
		{
			if ( boundary )
				Mixer_ReadPair8MonoLoop( pVoice->pSamples, sample, pair );
			else
				Mixer_ReadPair8Mono( pVoice->pSamples, sample, pair );
		}
		double phaseGain = ( (double)(LONG)phase * ( 1.0 / 65536.0 ) ) * gain;
		double contribution = pair[0] * gain + ( pair[1] - pair[0] ) * phaseGain;
		gain += gainStep;
		DWORD phaseSum = ( phase & MIXER_PHASE_FRACTION_MASK ) + ( rate & MIXER_PHASE_FRACTION_MASK );
		phase = ( phase & ~MIXER_PHASE_FRACTION_MASK ) | ( phaseSum & MIXER_PHASE_FRACTION_MASK );
		sample = (LONG)( (DWORD)sample + ( rate >> MIXER_PHASE_FRACTION_BITS ) +
			( phaseSum >> MIXER_PHASE_FRACTION_BITS ) );
		rate += rateStep;
		*pOutput = (float)( contribution + *pOutput );
		pOutput += 2;
		remaining--;
		first = false;
	}
	if ( sample > lastSample + 1 )
		sample = lastSample + 1;
	pChannel->iSourceSample = sample;
	pChannel->dwPhaseFraction = phase;
	return remaining;
}

static void smA3Dsrc( a3d_mix_voice_t* pVoice, a3d_mix_channel_t* pChannel,
		float* pOutput, DWORD cFrames )
{
	DWORD remaining = cFrames;
	if ( pVoice->cSourceBytes != 0 && pVoice->iSourceByte < pVoice->cSourceBytes )
		remaining = pVoice->pfnResampleChannel( pOutput, pChannel, pVoice, cFrames );
	if ( remaining )
		memset( pOutput + cFrames - remaining, 0, remaining * sizeof( float ) );
	Mixer_ResetFloatState();
}

void __cdecl Mixer_SetImpulseResponse( a3d_mix_channel_t* pChannel,
		const SHORT* pCoefficients, DWORD cTaps )
{
	pChannel->cFilterTaps = (LONG)cTaps;
	Mixer_ResetFloatState();
	const SHORT* pCoefficient = pCoefficients + cTaps;
	float* pTarget = pChannel->targetCoefficients;
	do
	{
		*pTarget++ = (float)( (double)*--pCoefficient * A3D_MIX_IMPULSE_SCALE );
	}
	while ( --cTaps );
}

static void smConvolveA3D( a3d_mix_channel_t* pChannel, const float* pSamples,
		float* pOutput, DWORD cFrames )
{
	if ( !cFrames )
	{
		Mixer_ResetFloatState();
		return;
	}
	Mixer_ResetFloatState();
	if ( pChannel->lGainCurrent == 0 && pChannel->lGainTarget == 0 )
		return;
	LONG taps = pChannel->cFilterTaps;
	if ( !taps )
		return;
	LONG steps = ( (LONG)cFrames + taps - 1 ) / taps;
	double gain = (double)pChannel->lGainTarget * ( 1.0 / 65536.0 );
	double stepScale = 1.0 / steps;
	for ( LONG i = 0; i < taps; i++ )
	{
		pChannel->coefficientSteps[i] = (float)( ( (double)pChannel->targetCoefficients[i] * gain -
			pChannel->coefficients[i] ) * stepScale );
	}
	Mixer_ResetFloatState();
	LONG coefficient = 0;
	for ( DWORD frame = 0; frame < cFrames; frame++ )
	{
		double sample = pOutput[frame * 2];
		for ( LONG i = 0; i < taps; i++ )
			sample += (double)pSamples[frame + i] * pChannel->coefficients[i];
		pOutput[frame * 2] = (float)sample;
		if ( frame + 1 != cFrames )
		{
			pChannel->coefficients[coefficient] += pChannel->coefficientSteps[coefficient];
			coefficient++;
			if ( coefficient >= taps )
				coefficient = 0;
		}
	}
	Mixer_ResetFloatState();
}

static void AR2DProcEar( a3d_mix_voice_t* pVoice, a3d_mix_channel_t* pChannel,
		DWORD cFrames, float* pOutput )
{
	DWORD history = (DWORD)pChannel->cFilterTaps;
	memcpy( mixSamples, pChannel->history, history * sizeof( float ) );
	smA3Dsrc( pVoice, pChannel, mixSamples + history, cFrames );
	memcpy( pChannel->history, mixSamples + cFrames, history * sizeof( float ) );
	smConvolveA3D( pChannel, mixSamples, pOutput, cFrames );
}

static void smInit( a3d_mix_voice_t* pVoice, DWORD dwFlags, WORD wSampleRate,
		const BYTE* pSamples, DWORD cSourceBytes )
{
	memset( pVoice, 0, sizeof( *pVoice ) );
	pVoice->pSamples = pSamples;
	pVoice->cSourceBytes = (LONG)cSourceBytes;
	pVoice->dwSampleRate = wSampleRate;
	pVoice->pfnMixChannel = Mixer_MixChannel;
	pVoice->pfnResampleChannel = Mixer_ResampleChannel;
	if ( dwFlags & A3D_MIX_STEREO )
	{
		pVoice->cSampleShift++;
		if ( dwFlags & A3D_MIX_16BIT )
		{
			pVoice->pfnReadPair = Mixer_ReadPair16Stereo;
			pVoice->pfnReadPairLoop = Mixer_ReadPair16StereoLoop;
			pVoice->pfnReadQuad = Mixer_ReadQuad16Stereo;
			pVoice->pfnReadQuadLoop = Mixer_ReadQuad16StereoLoop;
			pVoice->pfnReadQuadTail = Mixer_ReadQuad16StereoTail;
			pVoice->cSampleShift++;
		}
		else
		{
			pVoice->pfnReadPair = Mixer_ReadPair8Stereo;
			pVoice->pfnReadPairLoop = Mixer_ReadPair8StereoLoop;
			pVoice->pfnReadQuad = Mixer_ReadQuad8Stereo;
			pVoice->pfnReadQuadLoop = Mixer_ReadQuad8StereoLoop;
			pVoice->pfnReadQuadTail = Mixer_ReadQuad8StereoTail;
		}
	}
	else if ( dwFlags & A3D_MIX_16BIT )
	{
		pVoice->cSampleShift++;
		pVoice->pfnMixChannel = Mixer_Mix16Mono;
		pVoice->pfnResampleChannel = Mixer_Resample16Mono;
	}
	else
	{
		pVoice->pfnReadPair = Mixer_ReadPair8Mono;
		pVoice->pfnReadPairLoop = Mixer_ReadPair8MonoLoop;
		pVoice->pfnReadQuad = Mixer_ReadQuad8Mono;
		pVoice->pfnReadQuadLoop = Mixer_ReadQuad8MonoLoop;
		pVoice->pfnReadQuadTail = Mixer_ReadQuad8MonoTail;
	}
}

DWORD __cdecl AR2DSetFrequency( a3d_mix_voice_t* pVoice, DWORD dwFrequency )
{
	DWORD dwRate = ( dwFrequency << A3D_MIXER_RATE_SHIFT ) / ( pVoice->dwSampleRate >> 1 );
	DWORD dwCurrent = (DWORD)pVoice->lRateCurrent;
	pVoice->lRateRequested = (LONG)dwRate;
	if ( dwCurrent == 0 )
	{
		pVoice->lRateCurrent = (LONG)dwRate;
		pVoice->lRateTarget = (LONG)dwRate;
	}
	else
	{
		DWORD dwLimit = dwCurrent * A3D_MIXER_MAX_RATE_MULTIPLIER;
		if ( dwRate > dwLimit )
			pVoice->lRateTarget = (LONG)dwLimit;
		else
			pVoice->lRateTarget = (LONG)dwRate;
	}

	return dwRate;
}

SHORT __cdecl AR2Dsetposition( a3d_mix_voice_t* pVoice, a3d_mix_input_t* pInput, DWORD dwPosition )
{
	pVoice->iSourceByte = (LONG)dwPosition;
	pVoice->channels[0].iSourceSample = (LONG)dwPosition;
	pVoice->channels[0].lDelayCurrent = 0;
	pVoice->channels[1].iSourceSample = (LONG)dwPosition;
	pVoice->channels[1].lDelayCurrent = 0;
	pInput->pCurrent = pInput->pSamples + dwPosition;
	pVoice->channels[0].dwPhaseFraction = 0;
	pVoice->channels[1].dwPhaseFraction = 0;
	return 0;
}

SHORT __cdecl AR2Dinit( a3d_mix_voice_t* pVoice, a3d_mix_input_t* pInput, WORD wSampleRate )
{
	DWORD cSourceBytes = pInput->cFrames;
	if ( pInput->dwFlags & A3D_MIX_STEREO )
		cSourceBytes <<= 1;
	if ( pInput->dwFlags & A3D_MIX_16BIT )
		cSourceBytes <<= 1;
	smInit( pVoice, pInput->dwFlags, wSampleRate, pInput->pSamples, cSourceBytes );
	AR2Dsetposition( pVoice, pInput, (DWORD)( pInput->pCurrent - pInput->pSamples ) );
	if ( wSampleRate == 11 )
		pVoice->dwSampleRate = 11025;
	else if ( wSampleRate == 22 )
		pVoice->dwSampleRate = 22050;
	else
		pVoice->dwSampleRate = 44100;
	return 0;
}

static void smA2D( a3d_mix_voice_t* pVoice, float* pOutput, DWORD cFrames )
{
	if ( pVoice->cSourceBytes == 0 || pVoice->iSourceByte >= pVoice->cSourceBytes )
		return;
	pVoice->pfnMixChannel( pOutput, &pVoice->channels[0], pVoice, cFrames );
	pVoice->pfnMixChannel( pOutput + 1, &pVoice->channels[1], pVoice, cFrames );
	smUpdateA3DTrackers( pVoice, cFrames );
}

SHORT __cdecl AR2Dproc( a3d_mix_input_t* pInput, float* pOutput, a3d_mix_voice_t* pVoice,
		DWORD cFrames, BOOL bFilter )
{
	if ( cFrames > A3D_MIX_MAX_FRAMES )
	{
		printf( "numsamples of %d is too high!\n", (int)cFrames );
		return -1;
	}
	if ( !cFrames )
		return 2;
	pVoice->bLooping = pInput->dwFlags & A3D_MIX_LOOPING;
	if ( !bFilter )
	{
		pVoice->channels[0].lGainTarget = (LONG)( ( (DWORD)pVoice->channels[0].lGainBase *
			pVoice->channels[0].lGainSpatial ) >> 15 );
		pVoice->channels[1].lGainTarget = (LONG)( ( (DWORD)pVoice->channels[1].lGainBase *
			pVoice->channels[1].lGainSpatial ) >> 15 );
		smA2D( pVoice, pOutput, cFrames );
	}
	else
	{
		pVoice->channels[0].lGainTarget = pVoice->channels[0].lGainBase;
		pVoice->channels[1].lGainTarget = pVoice->channels[1].lGainBase;
		AR2DProcEar( pVoice, &pVoice->channels[0], cFrames, pOutput );
		AR2DProcEar( pVoice, &pVoice->channels[1], cFrames, pOutput + 1 );
		smUpdateA3DTrackers( pVoice, cFrames );
	}
	pInput->pCurrent = pInput->pSamples + pVoice->iSourceByte;
	return 0;
}

#define MIXER_FILTER_HISTORY_SAMPLES	4
#define MIXER_FILTER_ALTERNATE_MODE	(-3)

typedef struct mixer_response_coefficients_s
{
	float	inputGain;
	float	secondPreviousInputGain;
	float	previousInputGain;
	float	secondPreviousStateGain;
	float	previousStateGain;
	float	stateGain;
	float	previousStateOutputGain;
	float	previousOutputGain;
} mixer_response_coefficients_t;

typedef struct mixer_filter_coefficients_s
{
	int							delayFrames;
	float						inputGain;
	float						previousInputGain;
	float						previousOutputGain;
	mixer_response_coefficients_t	response;
} mixer_filter_coefficients_t;

static const mixer_filter_coefficients_t filter22kHz =
{
	5, 0.24488053f, -0.022858365f, -0.77053136f,
	{
		0.93972594f, 0.5445418f, 0.66765344f, 0.6754662f,
		0.7714774f, 0.7120579f, -0.6432081f, -0.9355754f
	}
};

static const mixer_filter_coefficients_t filter44kHz =
{
	10, 0.20249031f, -0.089571826f, -0.8836024f,
	{
		0.991302f, 0.763146f, -0.933744f, 0.82577f,
		-0.96173f, 1.0f, -0.95349f, -0.969237f
	}
};

void Mixer_ResetFloatState()
{
	unsigned int control = _mm_getcsr();
	_fpreset();
	unsigned int x87Control;
	_controlfp_s( &x87Control, _PC_64, _MCW_PC );
	_mm_setcsr( control );
}

void __cdecl Mixer_InitializeFilters( void* pUnused, mixer_filter_state_t* pState, DWORD sampleRate )
{
	(void)pUnused;
	memset( pState, 0, sizeof( *pState ) );
	pState->sampleRate = sampleRate;
}

static void __stdcall Mixer_Crossfeed( DWORD frameCount, mixer_filter_state_t* pState,
		const mixer_filter_coefficients_t* pCoefficients, float* pSamples )
{
	Mixer_ResetFloatState();
	double previousLeft = pState->channels[0].previousInput;
	double previousRight = pState->channels[1].previousInput;
	double crossfeedLeft = pState->channels[0].previousCrossfeed;
	double crossfeedRight = pState->channels[1].previousCrossfeed;
	int delayIndex = pState->delayIndex;
	if ( delayIndex >= pCoefficients->delayFrames )
		delayIndex = 0;

	do
	{
		double left = pSamples[0];
		double right = pSamples[1];
		double nextLeft = ( previousLeft * pCoefficients->previousInputGain +
			left * pCoefficients->inputGain ) - crossfeedLeft * pCoefficients->previousOutputGain;
		double nextRight = ( right * pCoefficients->inputGain +
			previousRight * pCoefficients->previousInputGain ) - crossfeedRight * pCoefficients->previousOutputGain;

		pSamples[0] = (float)( left - pState->channels[1].delay[delayIndex] );
		pSamples[1] = (float)( right - pState->channels[0].delay[delayIndex] );
		pState->channels[0].delay[delayIndex] = (float)nextLeft;
		pState->channels[1].delay[delayIndex] = (float)nextRight;
		previousLeft = left;
		previousRight = right;
		crossfeedLeft = nextLeft;
		crossfeedRight = nextRight;
		pSamples += 2;
		if ( ++delayIndex >= pCoefficients->delayFrames )
			delayIndex = 0;
	} while ( --frameCount );

	pState->channels[1].previousCrossfeed = (float)crossfeedRight;
	pState->channels[0].previousCrossfeed = (float)crossfeedLeft;
	pState->channels[1].previousInput = (float)previousRight;
	pState->channels[0].previousInput = (float)previousLeft;
	pState->delayIndex = delayIndex;
}

static void Mixer_FilterResponseChannel( float* pSamples, mixer_filter_channel_t* pChannel,
		const mixer_response_coefficients_t* pCoefficients, DWORD frameCount )
{
	double state = pChannel->responseState;
	double previousState = pChannel->previousResponseState;
	double output = pChannel->previousResponseOutput;

	do
	{
		double nextState = ( ( (double)pSamples[-4] * pCoefficients->secondPreviousInputGain -
			( state * pCoefficients->previousStateGain + previousState * pCoefficients->secondPreviousStateGain ) ) +
			(double)pSamples[-2] * pCoefficients->previousInputGain ) +
			(double)pSamples[0] * pCoefficients->inputGain;
		output = ( state * pCoefficients->previousStateOutputGain - output * pCoefficients->previousOutputGain ) +
			nextState * pCoefficients->stateGain;
		previousState = state;
		state = nextState;
		pSamples[-MIXER_FILTER_HISTORY_SAMPLES] = (float)output;
		pSamples += 2;
	} while ( --frameCount );

	pChannel->previousResponseOutput = (float)output;
	pChannel->previousResponseState = (float)previousState;
	pChannel->responseState = (float)state;
}

static void __stdcall Mixer_FilterResponse( DWORD frameCount, mixer_filter_state_t* pState,
		const mixer_response_coefficients_t* pCoefficients, float* pSamples )
{
	Mixer_ResetFloatState();
	float* pHistory = pSamples - MIXER_FILTER_HISTORY_SAMPLES;
	memcpy( pHistory, pState->previousFrames, sizeof( pState->previousFrames ) );
	memcpy( pState->previousFrames, pHistory + frameCount * 2, sizeof( pState->previousFrames ) );
	Mixer_FilterResponseChannel( pSamples, &pState->channels[0], pCoefficients, frameCount );
	Mixer_FilterResponseChannel( pSamples + 1, &pState->channels[1], pCoefficients, frameCount );
}

int __cdecl Mixer_FilterStereo( float* pSamples, SHORT mode, mixer_filter_state_t* pState, DWORD frameCount )
{
	if ( frameCount && mode )
	{
		static const mixer_filter_coefficients_t* const presets[4] =
		{
			&filter22kHz, &filter22kHz, &filter44kHz, &filter44kHz
		};
		int preset = mode == MIXER_FILTER_ALTERNATE_MODE ? 1 : 0;
		if ( pState->sampleRate == 44 )
			preset += 2;

		Mixer_Crossfeed( frameCount, pState, presets[preset], pSamples );
		Mixer_FilterResponse( frameCount, pState, &presets[preset]->response, pSamples );
	}
	Mixer_ResetFloatState();
	return 0;
}

void __cdecl Mixer_FloatToPcm16( const float* pInput, short* pOutput, DWORD sampleCount )
{
	if ( !sampleCount )
		return;

	Mixer_ResetFloatState();
	while ( sampleCount )
	{
		DWORD count = sampleCount & 1 ? 1 : sampleCount & 2 ? 2 : 4;
		long samples[4];
		for ( DWORD i = 0; i < count; i++ )
		{
			double sample = pInput[i];
			if ( sample >= LONG_MIN && sample <= LONG_MAX )
				samples[i] = lrint( sample );
			else
				samples[i] = LONG_MIN;
		}
		for ( DWORD i = 0; i < count; i++ )
		{
			if ( samples[i] > SHRT_MAX )
				pOutput[i] = SHRT_MAX;
			else if ( samples[i] < SHRT_MIN )
				pOutput[i] = SHRT_MIN;
			else
				pOutput[i] = (short)samples[i];
		}
		pInput += count;
		pOutput += count;
		sampleCount -= count;
	}
}

static const float resampleCoefficients[MIXER_RESAMPLE_PHASES][4] =
{
	{ 0.0f, 0.999023f, 0.0f, 0.0f },
	{ -0.002930f, 0.995605f, 0.007813f, -0.001465f },
	{ -0.005829f, 0.992096f, 0.015717f, -0.002960f },
	{ -0.008637f, 0.988403f, 0.023713f, -0.004456f },
	{ -0.011383f, 0.984619f, 0.031739f, -0.005951f },
	{ -0.014069f, 0.980682f, 0.039857f, -0.007447f },
	{ -0.016694f, 0.976653f, 0.048006f, -0.008942f },
	{ -0.019257f, 0.972472f, 0.056246f, -0.010437f },
	{ -0.021729f, 0.968169f, 0.064516f, -0.011902f },
	{ -0.024171f, 0.963713f, 0.072878f, -0.013398f },
	{ -0.026521f, 0.959166f, 0.081271f, -0.014893f },
	{ -0.028840f, 0.954497f, 0.089724f, -0.016358f },
	{ -0.031068f, 0.949705f, 0.098239f, -0.017823f },
	{ -0.033265f, 0.944792f, 0.106784f, -0.019288f },
	{ -0.035371f, 0.939756f, 0.115390f, -0.020753f },
	{ -0.037446f, 0.934629f, 0.124058f, -0.022217f },
	{ -0.039430f, 0.929350f, 0.132756f, -0.023652f },
	{ -0.041353f, 0.923978f, 0.141484f, -0.025086f },
	{ -0.043245f, 0.918516f, 0.150273f, -0.026521f },
	{ -0.045045f, 0.912900f, 0.159124f, -0.027924f },
	{ -0.046815f, 0.907193f, 0.167974f, -0.029359f },
	{ -0.048524f, 0.901395f, 0.176885f, -0.030763f },
	{ -0.050142f, 0.895474f, 0.185858f, -0.032136f },
	{ -0.051729f, 0.889462f, 0.194830f, -0.033509f },
	{ -0.053255f, 0.883358f, 0.203833f, -0.034883f },
	{ -0.054720f, 0.877132f, 0.212897f, -0.036256f },
	{ -0.056154f, 0.870815f, 0.221961f, -0.037599f },
	{ -0.057497f, 0.864376f, 0.231056f, -0.038911f },
	{ -0.058809f, 0.857875f, 0.240181f, -0.040223f },
	{ -0.060060f, 0.851253f, 0.249336f, -0.041536f },
	{ -0.061251f, 0.844539f, 0.258522f, -0.042817f },
	{ -0.062380f, 0.837764f, 0.267739f, -0.044069f },
	{ -0.063448f, 0.830866f, 0.276955f, -0.045320f },
	{ -0.064486f, 0.823878f, 0.286172f, -0.046541f },
	{ -0.065462f, 0.816828f, 0.295450f, -0.047761f },
	{ -0.066408f, 0.809687f, 0.304697f, -0.048952f },
	{ -0.067263f, 0.802454f, 0.314005f, -0.050142f },
	{ -0.068087f, 0.795129f, 0.323283f, -0.051302f },
	{ -0.068850f, 0.787744f, 0.332591f, -0.052431f },
	{ -0.069582f, 0.780267f, 0.341899f, -0.053560f },
	{ -0.070254f, 0.772698f, 0.351238f, -0.054659f },
	{ -0.070894f, 0.765069f, 0.360546f, -0.055727f },
	{ -0.071444f, 0.757378f, 0.369884f, -0.056764f },
	{ -0.071993f, 0.749596f, 0.379192f, -0.057802f },
	{ -0.072451f, 0.741752f, 0.388531f, -0.058779f },
	{ -0.072909f, 0.733848f, 0.397870f, -0.059755f },
	{ -0.073275f, 0.725852f, 0.407178f, -0.060732f },
	{ -0.073611f, 0.717795f, 0.416486f, -0.061647f },
	{ -0.073916f, 0.709677f, 0.425794f, -0.062532f },
	{ -0.074160f, 0.701498f, 0.435102f, -0.063417f },
	{ -0.074374f, 0.693258f, 0.444411f, -0.064272f },
	{ -0.074526f, 0.684957f, 0.453658f, -0.065065f },
	{ -0.074648f, 0.676595f, 0.462935f, -0.065859f },
	{ -0.074709f, 0.668203f, 0.472182f, -0.066622f },
	{ -0.074740f, 0.659719f, 0.481399f, -0.067354f },
	{ -0.074740f, 0.651204f, 0.490616f, -0.068056f },
	{ -0.074679f, 0.642628f, 0.499802f, -0.068728f },
	{ -0.074587f, 0.633992f, 0.508988f, -0.069338f },
	{ -0.074465f, 0.625324f, 0.518113f, -0.069948f },
	{ -0.074282f, 0.616596f, 0.527238f, -0.070528f },
	{ -0.074099f, 0.607837f, 0.536332f, -0.071047f },
	{ -0.073824f, 0.599048f, 0.545396f, -0.071566f },
	{ -0.073550f, 0.590197f, 0.554430f, -0.072024f },
	{ -0.073214f, 0.581317f, 0.563402f, -0.072451f },
	{ -0.072878f, 0.572375f, 0.572375f, -0.072878f },
	{ -0.072451f, 0.563402f, 0.581317f, -0.073214f },
	{ -0.072024f, 0.554430f, 0.590197f, -0.073550f },
	{ -0.071566f, 0.545396f, 0.599048f, -0.073824f },
	{ -0.071047f, 0.536332f, 0.607837f, -0.074099f },
	{ -0.070528f, 0.527238f, 0.616596f, -0.074282f },
	{ -0.069948f, 0.518113f, 0.625324f, -0.074465f },
	{ -0.069338f, 0.508988f, 0.633992f, -0.074587f },
	{ -0.068728f, 0.499802f, 0.642628f, -0.074679f },
	{ -0.068056f, 0.490616f, 0.651204f, -0.074740f },
	{ -0.067354f, 0.481399f, 0.659719f, -0.074740f },
	{ -0.066622f, 0.472182f, 0.668203f, -0.074709f },
	{ -0.065859f, 0.462935f, 0.676595f, -0.074648f },
	{ -0.065065f, 0.453658f, 0.684957f, -0.074526f },
	{ -0.064272f, 0.444411f, 0.693258f, -0.074374f },
	{ -0.063417f, 0.435102f, 0.701498f, -0.074160f },
	{ -0.062532f, 0.425794f, 0.709677f, -0.073916f },
	{ -0.061647f, 0.416486f, 0.717795f, -0.073611f },
	{ -0.060732f, 0.407178f, 0.725852f, -0.073275f },
	{ -0.059755f, 0.397870f, 0.733848f, -0.072909f },
	{ -0.058779f, 0.388531f, 0.741752f, -0.072451f },
	{ -0.057802f, 0.379192f, 0.749596f, -0.071993f },
	{ -0.056764f, 0.369884f, 0.757378f, -0.071444f },
	{ -0.055727f, 0.360546f, 0.765069f, -0.070894f },
	{ -0.054659f, 0.351238f, 0.772698f, -0.070254f },
	{ -0.053560f, 0.341899f, 0.780267f, -0.069582f },
	{ -0.052431f, 0.332591f, 0.787744f, -0.068850f },
	{ -0.051302f, 0.323283f, 0.795129f, -0.068087f },
	{ -0.050142f, 0.314005f, 0.802454f, -0.067263f },
	{ -0.048952f, 0.304697f, 0.809687f, -0.066408f },
	{ -0.047761f, 0.295450f, 0.816828f, -0.065462f },
	{ -0.046541f, 0.286172f, 0.823878f, -0.064486f },
	{ -0.045320f, 0.276955f, 0.830866f, -0.063448f },
	{ -0.044069f, 0.267739f, 0.837764f, -0.062380f },
	{ -0.042817f, 0.258522f, 0.844539f, -0.061251f },
	{ -0.041536f, 0.249336f, 0.851253f, -0.060060f },
	{ -0.040223f, 0.240181f, 0.857875f, -0.058809f },
	{ -0.038911f, 0.231056f, 0.864376f, -0.057497f },
	{ -0.037599f, 0.221961f, 0.870815f, -0.056154f },
	{ -0.036256f, 0.212897f, 0.877132f, -0.054720f },
	{ -0.034883f, 0.203833f, 0.883358f, -0.053255f },
	{ -0.033509f, 0.194830f, 0.889462f, -0.051729f },
	{ -0.032136f, 0.185858f, 0.895474f, -0.050142f },
	{ -0.030763f, 0.176885f, 0.901395f, -0.048524f },
	{ -0.029359f, 0.167974f, 0.907193f, -0.046815f },
	{ -0.027924f, 0.159124f, 0.912900f, -0.045045f },
	{ -0.026521f, 0.150273f, 0.918516f, -0.043245f },
	{ -0.025086f, 0.141484f, 0.923978f, -0.041353f },
	{ -0.023652f, 0.132756f, 0.929350f, -0.039430f },
	{ -0.022217f, 0.124058f, 0.934629f, -0.037446f },
	{ -0.020753f, 0.115390f, 0.939756f, -0.035371f },
	{ -0.019288f, 0.106784f, 0.944792f, -0.033265f },
	{ -0.017823f, 0.098239f, 0.949705f, -0.031068f },
	{ -0.016358f, 0.089724f, 0.954497f, -0.028840f },
	{ -0.014893f, 0.081271f, 0.959166f, -0.026521f },
	{ -0.013398f, 0.072878f, 0.963713f, -0.024171f },
	{ -0.011902f, 0.064516f, 0.968169f, -0.021729f },
	{ -0.010437f, 0.056246f, 0.972472f, -0.019257f },
	{ -0.008942f, 0.048006f, 0.976653f, -0.016694f },
	{ -0.007447f, 0.039857f, 0.980682f, -0.014069f },
	{ -0.005951f, 0.031739f, 0.984619f, -0.011383f },
	{ -0.004456f, 0.023713f, 0.988403f, -0.008637f },
	{ -0.002960f, 0.015717f, 0.992096f, -0.005829f },
	{ -0.001465f, 0.007813f, 0.995605f, -0.002930f }
};

void __cdecl Mixer_ReadQuad8Mono( const BYTE* pSamples, LONG iSample, LONG, double out[4] )
{
	for ( int i = 0; i < 4; i++ )
		out[i] = ( (int)pSamples[iSample + i] - MIXER_PCM8_MIDPOINT ) * MIXER_PCM8_TO_PCM16_SCALE;
}

void __cdecl Mixer_ReadQuad8MonoLoop( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] )
{
	LONG indices[4] = { iSample, 0, 1, 2 };
	if ( cRemaining != 1 )
	{
		indices[1] = iSample + 1;
		indices[2] = cRemaining == 2 ? 0 : iSample + 1;
		indices[3] = cRemaining == 2 ? 1 : 0;
	}
	for ( int i = 0; i < 4; i++ )
		out[i] = ( (int)pSamples[indices[i]] - MIXER_PCM8_MIDPOINT ) * MIXER_PCM8_TO_PCM16_SCALE;
}

void __cdecl Mixer_ReadQuad8MonoTail( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] )
{
	int cRead = cRemaining == 1 ? 1 : cRemaining == 2 ? 2 : 3;
	for ( int i = 0; i < cRead; i++ )
		out[i] = ( (int)pSamples[iSample + i] - MIXER_PCM8_MIDPOINT ) * MIXER_PCM8_TO_PCM16_SCALE;
	for ( int i = cRead; i < 4; i++ )
		out[i] = 0.0;
}

void __cdecl Mixer_ReadQuad8Stereo( const BYTE* pSamples, LONG iSample, LONG, double out[4] )
{
	SHORT previousHalf = 0;
	for ( int i = 0; i < 4; i++ )
	{
		int first = ( (int)pSamples[( iSample + i ) * 2] - MIXER_PCM8_MIDPOINT ) *
			( MIXER_PCM8_TO_PCM16_SCALE / 2 ) + (BYTE)previousHalf / 2;
		previousHalf = (SHORT)first;
		int second = ( (int)pSamples[( iSample + i ) * 2 + 1] - MIXER_PCM8_MIDPOINT ) *
			( MIXER_PCM8_TO_PCM16_SCALE / 2 ) + (BYTE)previousHalf / 2;
		previousHalf = (SHORT)second;
		out[i] = (SHORT)( first + second );
	}
}

void __cdecl Mixer_ReadQuad8StereoLoop( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] )
{
	LONG indices[4] = { iSample, 0, 1, 2 };
	if ( cRemaining != 1 )
	{
		indices[1] = iSample + 1;
		indices[2] = cRemaining == 2 ? 0 : iSample + 2;
		indices[3] = cRemaining == 2 ? 1 : 0;
	}
	SHORT previousHalf = 0;
	for ( int i = 0; i < 4; i++ )
	{
		int first = ( (int)pSamples[indices[i] * 2] - MIXER_PCM8_MIDPOINT ) *
			( MIXER_PCM8_TO_PCM16_SCALE / 2 ) + (BYTE)previousHalf / 2;
		previousHalf = (SHORT)first;
		int second = ( (int)pSamples[indices[i] * 2 + 1] - MIXER_PCM8_MIDPOINT ) *
			( MIXER_PCM8_TO_PCM16_SCALE / 2 ) + (BYTE)previousHalf / 2;
		previousHalf = (SHORT)second;
		out[i] = (SHORT)( first + second );
	}
}

void __cdecl Mixer_ReadQuad8StereoTail( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] )
{
	int cRead = cRemaining == 1 ? 1 : cRemaining == 2 ? 2 : 3;
	SHORT previousHalf = 0;
	for ( int i = 0; i < cRead; i++ )
	{
		int first = ( (int)pSamples[( iSample + i ) * 2] - MIXER_PCM8_MIDPOINT ) *
			( MIXER_PCM8_TO_PCM16_SCALE / 2 ) + (BYTE)previousHalf / 2;
		previousHalf = (SHORT)first;
		int second = ( (int)pSamples[( iSample + i ) * 2 + 1] - MIXER_PCM8_MIDPOINT ) *
			( MIXER_PCM8_TO_PCM16_SCALE / 2 ) + (BYTE)previousHalf / 2;
		previousHalf = (SHORT)second;
		out[i] = (SHORT)( first + second );
	}
	for ( int i = cRead; i < 4; i++ )
		out[i] = 0.0;
}

void __cdecl Mixer_ReadQuad16Stereo( const BYTE* pSamples, LONG iSample, LONG, double out[4] )
{
	const SHORT* pPcm = (const SHORT*)pSamples;
	for ( int i = 0; i < 4; i++ )
		out[i] = ( (int)pPcm[( iSample + i ) * 2] + pPcm[( iSample + i ) * 2 + 1] ) * 0.5;
}

void __cdecl Mixer_ReadQuad16StereoLoop( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] )
{
	const SHORT* pPcm = (const SHORT*)pSamples;
	LONG indices[4] = { iSample, 0, 1, 2 };
	if ( cRemaining != 1 )
	{
		indices[1] = iSample + 1;
		indices[2] = cRemaining == 2 ? 0 : iSample + 2;
		indices[3] = cRemaining == 2 ? 1 : 0;
	}
	for ( int i = 0; i < 4; i++ )
		out[i] = ( (int)pPcm[indices[i] * 2] + pPcm[indices[i] * 2 + 1] ) * 0.5;
}

void __cdecl Mixer_ReadQuad16StereoTail( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] )
{
	const SHORT* pPcm = (const SHORT*)pSamples;
	int cRead = cRemaining == 1 ? 1 : cRemaining == 2 ? 2 : 3;
	for ( int i = 0; i < cRead; i++ )
		out[i] = ( (int)pPcm[( iSample + i ) * 2] + pPcm[( iSample + i ) * 2 + 1] ) * 0.5;
	for ( int i = cRead; i < 4; i++ )
		out[i] = 0.0;
}

DWORD __cdecl Mixer_Resample16Mono( float* pOutput, a3d_mix_channel_t* pChannel,
		a3d_mix_voice_t* pVoice, DWORD cFrames )
{
	DWORD cSamples = (DWORD)pVoice->cSourceBytes >> pVoice->cSampleShift;
	if ( !pVoice->bLooping )
	{
		DWORD delayProduct = (DWORD)( pChannel->lDelayTarget >> 8 ) * (DWORD)( pVoice->lRateTarget >> 8 );
		cSamples -= delayProduct >> MIXER_PHASE_FRACTION_BITS;
	}
	LONG iLastFull = (LONG)( cSamples - 4 );
	LONG rateStep;
	DWORD rate = (DWORD)Mixer_PrepareChannelRate( pChannel, pVoice, cFrames, &rateStep );
	Mixer_ResetFloatState();
	pChannel->lGainCurrent = pChannel->lGainTarget;
	LONG iSample = pChannel->iSourceSample;
	DWORD fraction = pChannel->dwPhaseFraction;
	const SHORT* pSamples = (const SHORT*)pVoice->pSamples;

	for ( ;; )
	{
		double samples[4];
		if ( iSample <= iLastFull )
		{
			for ( int i = 0; i < 4; i++ )
				samples[i] = pSamples[iSample + i];
		}
		else
		{
			DWORD cRemaining = cSamples - (DWORD)iSample;
			if ( pVoice->bLooping )
			{
				if ( !cRemaining )
				{
					iSample = 0;
					for ( int i = 0; i < 4; i++ )
						samples[i] = pSamples[i];
				}
				else if ( cSamples < (DWORD)iSample )
				{
					iSample = (LONG)( (DWORD)iSample - cSamples );
					continue;
				}
				else
				{
					LONG indices[4] = { iSample, 0, 1, 2 };
					if ( cRemaining != 1 )
					{
						indices[1] = iSample + 1;
						indices[2] = cRemaining == 2 ? 0 : iSample + 2;
						indices[3] = cRemaining == 2 ? 1 : 0;
					}
					for ( int i = 0; i < 4; i++ )
						samples[i] = pSamples[indices[i]];
				}
			}
			else
			{
				if ( !cRemaining )
					break;
				if ( cSamples < (DWORD)iSample )
				{
					iSample = (LONG)cSamples;
					break;
				}
				int cRead = cRemaining == 1 ? 1 : cRemaining == 2 ? 2 : 3;
				for ( int i = 0; i < cRead; i++ )
					samples[i] = pSamples[iSample + i];
				for ( int i = cRead; i < 4; i++ )
					samples[i] = 0.0;
			}
		}

		DWORD phase = ( fraction >> ( MIXER_PHASE_FRACTION_BITS - MIXER_RESAMPLE_PHASE_BITS ) ) &
			( MIXER_RESAMPLE_PHASES - 1 );
		const float* pCoefficients = resampleCoefficients[phase];
		*pOutput++ = (float)( ( samples[0] * pCoefficients[0] + samples[1] * pCoefficients[1] ) +
			samples[2] * pCoefficients[2] + samples[3] * pCoefficients[3] );
		DWORD nextFraction = ( fraction & MIXER_PHASE_FRACTION_MASK ) + ( rate & MIXER_PHASE_FRACTION_MASK );
		fraction = ( fraction & ~MIXER_PHASE_FRACTION_MASK ) | ( nextFraction & MIXER_PHASE_FRACTION_MASK );
		iSample = (LONG)( (DWORD)iSample + ( rate >> MIXER_PHASE_FRACTION_BITS ) +
			( nextFraction >> MIXER_PHASE_FRACTION_BITS ) );
		rate += (DWORD)rateStep;
		if ( !--cFrames )
			break;
	}

	pChannel->iSourceSample = iSample;
	pChannel->dwPhaseFraction = fraction;
	return cFrames;
}

DWORD __cdecl Mixer_ResampleChannel( float* pOutput, a3d_mix_channel_t* pChannel,
		a3d_mix_voice_t* pVoice, DWORD cFrames )
{
	a3d_mix_read_quad_t read = pVoice->pfnReadQuad;
	a3d_mix_read_quad_t readLoop = pVoice->pfnReadQuadLoop;
	a3d_mix_read_quad_t readTail = pVoice->pfnReadQuadTail;
	DWORD cSamples = (DWORD)pVoice->cSourceBytes >> pVoice->cSampleShift;
	if ( !pVoice->bLooping )
		cSamples -= (DWORD)pChannel->lDelayTarget >> MIXER_PHASE_FRACTION_BITS;
	LONG iLastFull = (LONG)( cSamples - 4 );
	LONG rateStep;
	DWORD rate = (DWORD)Mixer_PrepareChannelRate( pChannel, pVoice, cFrames, &rateStep );
	Mixer_ResetFloatState();
	pChannel->lGainCurrent = pChannel->lGainTarget;
	LONG iSample = pChannel->iSourceSample;
	DWORD fraction = pChannel->dwPhaseFraction;
	const BYTE* pSamples = pVoice->pSamples;

	for ( ;; )
	{
		double samples[4];
		if ( iSample <= iLastFull )
		{
			read( pSamples, iSample, 4, samples );
		}
		else
		{
			DWORD cRemaining = cSamples - (DWORD)iSample;
			if ( pVoice->bLooping )
			{
				if ( !cRemaining )
				{
					iSample = 0;
					read( pSamples, iSample, 4, samples );
				}
				else if ( cSamples < (DWORD)iSample )
				{
					iSample = (LONG)( (DWORD)iSample - cSamples );
					continue;
				}
				else
				{
					readLoop( pSamples, iSample, (LONG)cRemaining, samples );
				}
			}
			else
			{
				if ( !cRemaining )
					break;
				if ( cSamples < (DWORD)iSample )
				{
					iSample = (LONG)cSamples;
					break;
				}
				readTail( pSamples, iSample, (LONG)cRemaining, samples );
			}
		}

		DWORD phase = ( fraction >> ( MIXER_PHASE_FRACTION_BITS - MIXER_RESAMPLE_PHASE_BITS ) ) &
			( MIXER_RESAMPLE_PHASES - 1 );
		const float* pCoefficients = resampleCoefficients[phase];
		*pOutput++ = (float)( ( samples[0] * pCoefficients[0] + samples[1] * pCoefficients[1] ) +
			samples[2] * pCoefficients[2] + samples[3] * pCoefficients[3] );
		DWORD nextFraction = ( fraction & MIXER_PHASE_FRACTION_MASK ) + ( rate & MIXER_PHASE_FRACTION_MASK );
		fraction = ( fraction & ~MIXER_PHASE_FRACTION_MASK ) | ( nextFraction & MIXER_PHASE_FRACTION_MASK );
		iSample = (LONG)( (DWORD)iSample + ( rate >> MIXER_PHASE_FRACTION_BITS ) +
			( nextFraction >> MIXER_PHASE_FRACTION_BITS ) );
		rate += (DWORD)rateStep;
		if ( !--cFrames )
			break;
	}

	pChannel->iSourceSample = iSample;
	pChannel->dwPhaseFraction = fraction;
	return cFrames;
}
