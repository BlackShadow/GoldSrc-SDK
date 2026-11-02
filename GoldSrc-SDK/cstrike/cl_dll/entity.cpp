// Client side entity management functions

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_types.h"
#include "studio_event.h" // def. of mstudioevent_t
#include "r_efx.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "particleman.h"
#include "../game_shared/voice_status.h"

extern IParticleMan* g_pParticleMan;
void Game_AddObjects();
float g_flGravity;
int iOnTrain[64];

extern "C" int HUD_AddEntity( int type, cl_entity_t* entity, const char* modelName )
{
	if ( (unsigned int)type <= ET_PLAYER && entity->player && iOnTrain[entity->index] )
	{
		entity->origin = entity->curstate.origin;
		entity->angles = entity->curstate.angles;
	}
	// each frame every entity passes this function, so the overview hooks it to filter the overview entities
	// in spectator mode:
	// each frame every entity passes this function, so the overview hooks 
	// it to filter the overview entities

	if ( g_iUser1 )
	{
		gHUD.m_Spectator.AddOverviewEntity( type, entity, modelName );
		if ( g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_pip->value == INSET_IN_EYE )
			return entity->index != g_iUser2;
	}
	return 1;
}

extern "C" void HUD_TxferLocalOverrides( entity_state_t* state, const clientdata_t* client )
{
	state->origin = client->origin;

	// Spectator
	state->iuser1 = client->iuser1;
	state->iuser2 = client->iuser2;

	// Duck prevention
	state->iuser3 = client->iuser3;

	// Fire prevention
	state->iuser4 = client->iuser4;
}

extern "C" void HUD_ProcessPlayerState( entity_state_t* destination, const entity_state_t* source )
{
	iOnTrain[source->number] = source->iuser4;
	destination->origin = source->origin;
	destination->angles = source->angles;
	destination->velocity = source->velocity;
	destination->frame = source->frame;
	destination->modelindex = source->modelindex;
	destination->skin = source->skin;
	destination->effects = source->effects;
	destination->weaponmodel = source->weaponmodel;
	destination->movetype = source->movetype;
	destination->sequence = source->sequence;
	destination->animtime = source->animtime;
	destination->solid = source->solid;
	destination->rendermode = source->rendermode;
	destination->renderamt = source->renderamt;
	destination->rendercolor = source->rendercolor;
	destination->renderfx = source->renderfx;
	destination->framerate = source->framerate;
	destination->body = source->body;
	memcpy( destination->controller, source->controller, sizeof( destination->controller ) );
	memcpy( destination->blending, source->blending, 2 );
	destination->basevelocity = source->basevelocity;
	destination->friction = source->friction;
	destination->gravity = source->gravity;
	destination->gaitsequence = source->gaitsequence;
	destination->spectator = source->spectator;
	destination->usehull = source->usehull;
	destination->playerclass = source->playerclass;
	destination->team = source->team;
	destination->colormap = source->colormap;
	if ( destination->number == gEngfuncs.GetLocalPlayer()->index )
	{
		g_iTeamNumber = g_PlayerExtraInfo[destination->number].teamnumber;
		g_iUser1 = source->iuser1;
		g_iUser2 = source->iuser2;
		g_iUser3 = source->iuser3;
	}
	destination->fuser2 = source->fuser2;
}

extern "C" void HUD_TxferPredictionData( entity_state_t* state, const entity_state_t* previousState,
	clientdata_t* client, const clientdata_t* previousClient, weapon_data_t* weapons, const weapon_data_t* previousWeapons )
{
	state->oldbuttons = previousState->oldbuttons;
	state->flFallVelocity = previousState->flFallVelocity;
	state->iStepLeft = previousState->iStepLeft;
	state->playerclass = previousState->playerclass;
	state->iuser4 = previousState->iuser4;
	client->viewmodel = previousClient->viewmodel;
	client->m_iId = previousClient->m_iId;
	client->ammo_shells = previousClient->ammo_shells;
	client->ammo_nails = previousClient->ammo_nails;
	client->ammo_cells = previousClient->ammo_cells;
	client->ammo_rockets = previousClient->ammo_rockets;
	client->m_flNextAttack = previousClient->m_flNextAttack;
	client->fov = previousClient->fov;
	client->weaponanim = previousClient->weaponanim;
	client->tfstate = previousClient->tfstate;
	client->maxspeed = previousClient->maxspeed;
	client->deadflag = previousClient->deadflag;
	client->iuser1 = previousClient->iuser1;
	client->iuser2 = previousClient->iuser2;
	client->iuser3 = previousClient->iuser3;
	client->iuser4 = previousClient->iuser4;
	if ( gEngfuncs.IsSpectateOnly() )
	{
		client->iuser1 = g_iUser1;
		client->iuser2 = g_iUser2;
		client->iuser3 = g_iUser3;
	}
	client->vuser2 = previousClient->vuser2;
	client->vuser3 = previousClient->vuser3;
	client->vuser4 = previousClient->vuser4;
	memcpy( weapons, previousWeapons, sizeof( weapon_data_t ) * 32 );
}

extern "C" void HUD_CreateEntities()
{

	// Add in any game specific objects
	Game_AddObjects();
	GetClientVoiceHud()->CreateEntities();
}

extern "C" void HUD_StudioEvent( const mstudioevent_t* event, const cl_entity_t* entity )
{
	switch ( event->event )
	{
	case 5001:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float*)(const float*)entity->attachment[0], atoi( event->options ) );
		break;
	case 5002:
		gEngfuncs.pEfxAPI->R_SparkEffect( (float*)(const float*)entity->attachment[0], atoi( event->options ), -100, 100 );
		break;
	case 5004:
		gEngfuncs.pfnPlaySoundByNameAtLocation( (char*)event->options, 1, (float*)(const float*)entity->attachment[0] );
		break;
	case 5011:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float*)(const float*)entity->attachment[1], atoi( event->options ) );
		break;
	case 5021:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float*)(const float*)entity->attachment[2], atoi( event->options ) );
		break;
	case 5031:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float*)(const float*)entity->attachment[3], atoi( event->options ) );
		break;
	}
}

extern "C" void HUD_TempEntUpdate( double frameTime, double clientTime, double gravity,
	TEMPENTITY** freeEntities, TEMPENTITY** activeEntities,
	int ( *addVisibleEntity )( cl_entity_t* ), void ( *playSound )( TEMPENTITY*, float ) )
{
	static int tempEntFrame;
	g_flGravity = gravity;
	if ( g_pParticleMan )
		g_pParticleMan->SetVariables( g_flGravity, gHUD.m_vecAngles );
	if ( !*activeEntities )
		return;

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL 
	// tent, then set this BOOL to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( FALSE, TRUE );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers( -1 );
	tempEntFrame = ( tempEntFrame + 1 ) & 31;
	if ( frameTime <= 0 )
	{
		for ( TEMPENTITY* entity = *activeEntities; entity; entity = entity->next )
			if ( !( entity->flags & FTENT_NOMODEL ) )
				addVisibleEntity( &entity->entity );
		gEngfuncs.pEventAPI->EV_PopPMStates();
		return;
	}
	float gravityStep = -frameTime * gravity;
	float slowGravity = gravityStep * 0.5;
	TEMPENTITY* previous = NULL;
	for ( TEMPENTITY* entity = *activeEntities; entity; )
	{
		TEMPENTITY* next = entity->next;
		float life = entity->die - clientTime;
		if ( life < 0 )
		{
			if ( entity->flags & FTENT_FADEOUT )
			{
				if ( entity->entity.curstate.rendermode == kRenderNormal )
					entity->entity.curstate.rendermode = kRenderTransTexture;
				float amount = entity->entity.baseline.renderamt * ( 1.0 + life * entity->fadeSpeed );
				entity->entity.curstate.renderamt = (int)amount;
			}
			if ( !( entity->flags & FTENT_FADEOUT ) || entity->entity.curstate.renderamt <= 0 )
			{
				entity->next = *freeEntities;
				*freeEntities = entity;
				if ( previous )
					previous->next = next;
				else
					*activeEntities = next;
				entity = next;
				continue;
			}
		}
		entity->entity.prevstate.origin = entity->entity.origin;
		if ( entity->flags & FTENT_SPARKSHOWER )
		{
			// Adjust speed if it's time
			// Scale is next think time
			if ( clientTime > entity->entity.baseline.scale )
			{
				gEngfuncs.pEfxAPI->R_SparkEffect( entity->entity.origin, 8, -200, 200 );
				entity->entity.baseline.framerate -= 0.1;
				if ( entity->entity.baseline.framerate <= 0 )
					entity->die = clientTime;
				else
				{
					entity->die = clientTime + 0.5;
					entity->entity.baseline.scale = clientTime + 0.1;
				}
			}
		}
		else if ( entity->flags & FTENT_PLYRATTACHMENT )
			entity->entity.origin = gEngfuncs.GetEntityByIndex( entity->clientIndex )->origin + entity->tentOffset;
		else if ( entity->flags & FTENT_SINEWAVE )
		{
			entity->x += entity->entity.baseline.origin.x * frameTime;
			entity->y += entity->entity.baseline.origin.y * frameTime;
			float frequency = clientTime * 5.5;
			entity->entity.origin.x = entity->x + sin( clientTime * entity->entity.prevstate.frame + entity->entity.baseline.origin.z ) * ( entity->entity.curstate.framerate * 10.0 );
			entity->entity.origin.y = entity->y + sin( frequency + entity->entity.baseline.origin.z + 0.7 ) * ( entity->entity.curstate.framerate * 8.0 );
			entity->entity.origin.z += entity->entity.baseline.origin.z * frameTime;
		}
		else if ( entity->flags & FTENT_SPIRAL )
		{
			entity->entity.origin.x += entity->entity.baseline.origin.x * frameTime + 8.0 * sin( clientTime * 20.0 + (int)(intptr_t)entity );
			entity->entity.origin.y += entity->entity.baseline.origin.y * frameTime + 4.0 * sin( clientTime * 30.0 + (int)(intptr_t)entity );
			entity->entity.origin.z += entity->entity.baseline.origin.z * frameTime;
		}
		else
		{
			entity->entity.origin.x += entity->entity.baseline.origin.x * frameTime;
			entity->entity.origin.y += entity->entity.baseline.origin.y * frameTime;
			entity->entity.origin.z += entity->entity.baseline.origin.z * frameTime;
		}
		if ( entity->flags & FTENT_SPRANIMATE )
		{
			entity->entity.curstate.frame += frameTime * entity->entity.curstate.framerate;
			if ( entity->entity.curstate.frame >= entity->frameMax )
			{
				entity->entity.curstate.frame -= (int)entity->entity.curstate.frame;
				if ( entity->flags & 0x100000 )
				{
					previous = entity;
					entity = next;
					continue;
				}
				if ( !( entity->flags & FTENT_SPRANIMATELOOP ) )
				{
					entity->die = clientTime;
					previous = entity;
					entity = next;
					continue;
				}
			}
		}
		else if ( entity->flags & FTENT_SPRCYCLE )
		{
			entity->entity.curstate.frame += frameTime * 10.0;
			if ( entity->entity.curstate.frame >= entity->frameMax )
				entity->entity.curstate.frame -= (int)entity->entity.curstate.frame;
		}
		if ( entity->flags & FTENT_ROTATE )
		{
			entity->entity.angles.x += entity->entity.baseline.angles.x * frameTime;
			entity->entity.angles.y += entity->entity.baseline.angles.y * frameTime;
			entity->entity.angles.z += entity->entity.baseline.angles.z * frameTime;
			entity->entity.latched.prevangles = entity->entity.angles;
		}
		if ( ( entity->flags & ( FTENT_COLLIDEALL | FTENT_COLLIDEWORLD ) ) && !( entity->flags & 0x200000 ) )
		{
			pmtrace_t trace;
			float fraction = 1;
			Vector normal;
			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			if ( entity->flags & FTENT_COLLIDEALL )
			{
				gEngfuncs.pEventAPI->EV_PlayerTrace( entity->entity.prevstate.origin, entity->entity.origin, PM_STUDIO_BOX, -1, &trace );
				if ( trace.fraction != 1 && ( !trace.ent || gEngfuncs.pEventAPI->EV_GetPhysent( trace.ent )->info != entity->clientIndex ) )
				{
					normal = trace.plane.normal;
					fraction = trace.fraction;
					if ( entity->hitcallback )
						entity->hitcallback( entity, &trace );
				}
			}
			else
			{
				Vector end = entity->entity.origin;
				if ( entity->flags & 0x100000 )
				{
					Vector velocity = entity->entity.baseline.origin;
					float speed = velocity.Length();
					if ( speed )
						velocity = velocity / speed;
					end = entity->entity.prevstate.origin + velocity * 36;
				}
				gEngfuncs.pEventAPI->EV_PlayerTrace( entity->entity.prevstate.origin, end, PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &trace );
				fraction = trace.fraction;
				if ( fraction != 1 )
				{
					normal = trace.plane.normal;
					if ( entity->flags & FTENT_SPARKSHOWER )
					{
						entity->entity.baseline.origin = entity->entity.baseline.origin * 0.6f;
						if ( entity->entity.baseline.origin.Length() < 10 )
							entity->entity.baseline.framerate = 0;
					}
					if ( entity->hitcallback )
						entity->hitcallback( entity, &trace );
				}
			}
			if ( fraction != 1 )
			{
				entity->entity.origin = entity->entity.prevstate.origin + entity->entity.baseline.origin * (float)( fraction * frameTime );
				float damp = entity->bounceFactor;
				if ( entity->flags & ( FTENT_GRAVITY | FTENT_SLOWGRAVITY ) )
				{
					damp *= 0.5;
					if ( normal.z > 0.9 && entity->entity.baseline.origin.z <= 0 && entity->entity.baseline.origin.z >= gravityStep * 3.0 )
					{
						damp = 0;
						entity->entity.angles.x = entity->entity.angles.z = 0;
						entity->flags &= ~( FTENT_ROTATE | FTENT_GRAVITY | FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD | FTENT_SMOKETRAIL );
					}
				}
				if ( entity->hitSound )
					playSound( entity, damp );
				if ( entity->flags & FTENT_COLLIDEKILL )
				{
					entity->flags &= ~FTENT_FADEOUT;
					entity->die = clientTime;
				}
				else
				{
					// Reflect velocity
					if ( damp != 0 )
					{
						float projection = -2 * DotProduct( entity->entity.baseline.origin, normal );
						entity->entity.baseline.origin = entity->entity.baseline.origin + normal * projection;
						entity->entity.angles.y = -entity->entity.angles.y;
					}
					if ( damp != 1 )
					{
						entity->entity.baseline.origin = entity->entity.baseline.origin * damp;
						if ( !( entity->flags & 0x100000 ) )
							entity->entity.angles = entity->entity.angles * 0.9f;
					}
				}
			}
		}
		if ( ( entity->flags & FTENT_FLICKER ) && entity->entity.curstate.effects == tempEntFrame )
		{
			dlight_t* light = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
			light->origin = entity->entity.origin;
			light->radius = 60;
			light->color.r = 255;
			light->color.g = 120;
			light->color.b = 0;
			light->die = clientTime + 0.01;
		}
		if ( entity->flags & FTENT_SMOKETRAIL )
			gEngfuncs.pEfxAPI->R_RocketTrail( entity->entity.prevstate.origin, entity->entity.origin, 1 );
		if ( !( entity->flags & 0x200000 ) )
		{
			if ( entity->flags & FTENT_GRAVITY )
				entity->entity.baseline.origin.z += gravityStep;
			else if ( entity->flags & FTENT_SLOWGRAVITY )
				entity->entity.baseline.origin.z += slowGravity;
		}
		if ( ( entity->flags & FTENT_CLIENTCUSTOM ) && entity->callback )
			entity->callback( entity, frameTime, clientTime );
		if ( !( entity->flags & FTENT_NOMODEL ) && !addVisibleEntity( &entity->entity ) && !( entity->flags & FTENT_PERSIST ) )
		{
			entity->die = clientTime;
			entity->flags &= ~FTENT_FADEOUT;
		}
		previous = entity;
		entity = next;
	}
	gEngfuncs.pEventAPI->EV_PopPMStates();
}

extern "C" cl_entity_t* HUD_GetUserEntity( int )
{
	return NULL;
}
