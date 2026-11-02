#include "pm_defs.h"
#include "server.h"
#include "../common/event_flags.h"

void EV_SV_Playback( int flags, int clientindex, unsigned short eventindex, float delay, float* origin, float* angles,
	float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
const char* TraceTexture( edict_t* pTextureEntity, const float* v1, const float* v2 );

void PM_SV_PlaybackEventFull( int flags, int clientindex, unsigned short eventindex, float delay, float* origin,
	float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 )
{
	EV_SV_Playback( flags | FEV_NOTHOST, clientindex, eventindex, delay, origin, angles, fparam1, fparam2, iparam1,
		iparam2, bparam1, bparam2 );
}

void PM_SV_PlaySound( int channel, const char* sample, float volume, float attenuation, int fFlags, int pitch )
{
	SV_StartSound( 1, host_client->edict, channel, sample, (int)( volume * 255 ), attenuation, fFlags, pitch );
}

const char* PM_SV_TraceTexture( int ground, float* vstart, float* vend )
{
	physent_t* pe;
	edict_t* ent;

	if ( ground < 0 || ground >= pmove->numphysent )
		return NULL;

	pe = &pmove->physents[ground];
	if ( !pe->model || pe->info < 0 || pe->info >= sv.max_edicts )
		return NULL;

	ent = &sv.edicts[pe->info];
	if ( !ent )
		return NULL;

	return TraceTexture( ent, vstart, vend );
}
