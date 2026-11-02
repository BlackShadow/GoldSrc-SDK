#pragma once

#include "a2d.h"

DWORD	__cdecl Mixer_Resample16Mono( float* pOutput, a3d_mix_channel_t* pChannel,
		a3d_mix_voice_t* pVoice, DWORD cFrames );
DWORD	__cdecl Mixer_ResampleChannel( float* pOutput, a3d_mix_channel_t* pChannel,
		a3d_mix_voice_t* pVoice, DWORD cFrames );

void	__cdecl Mixer_ReadQuad8Mono( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] );
void	__cdecl Mixer_ReadQuad8MonoLoop( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] );
void	__cdecl Mixer_ReadQuad8MonoTail( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] );
void	__cdecl Mixer_ReadQuad8Stereo( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] );
void	__cdecl Mixer_ReadQuad8StereoLoop( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] );
void	__cdecl Mixer_ReadQuad8StereoTail( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] );
void	__cdecl Mixer_ReadQuad16Stereo( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] );
void	__cdecl Mixer_ReadQuad16StereoLoop( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] );
void	__cdecl Mixer_ReadQuad16StereoTail( const BYTE* pSamples, LONG iSample, LONG cRemaining, double out[4] );
