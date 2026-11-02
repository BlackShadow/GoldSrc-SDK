// Client side entity management functions

#include "hud.h"
#include "cl_util.h"
#include "Exports.h"
#include "entity_types.h"
#include "studio_event.h" // def. of mstudioevent_t
#include "r_efx.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pmtrace.h"
#include "com_model.h"
#include "voice_status.h"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4018)
#endif
#include "tri.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#define FTENT_ENVMODEL		0x00100000
#define FTENT_GRAVITYFACTOR	0x00200000
#define FTENT_LONGTRACE		0x00400000
#define FTENT_SKIPPHYSICS		0x00800000

float g_flGravity;
int g_iAlive;
int g_iDeadFlag;

extern Vector v_angles;
extern cvar_t* cl_bulletejects;
extern cvar_t* cl_dmshowgrenades;
extern cvar_t* cl_dmshowobjects;
extern cvar_t* cl_dmshowplayers;

void Game_AddObjects();

int CL_DLLEXPORT HUD_AddEntity( int type, cl_entity_s* entity, const char* modelname )
{
	g_pcldstAddrs->pAddEntity( &type, &entity, &modelname );
	int minimap = gHUD.GetMinimapState();
	if ( gEngfuncs.IsSpectateOnly() || g_iUser1 || gHUD.GetMinimapState() > 0 )
	{
		if ( g_iTeamNumber == 1 || gEngfuncs.IsSpectateOnly() )
		{
			if ( strstr( modelname, "w_grenade" ) && ( !minimap || cl_dmshowgrenades->value > 0.0 ) )
				gHUD.m_Spectator.AddOverviewEntityToList( gHUD.m_Spectator.m_hsprGrenade, entity,
					gEngfuncs.GetClientTime() - 1.0 );
			if ( strstr( modelname, "w_mills" ) && ( !minimap || cl_dmshowgrenades->value > 0.0 ) )
				gHUD.m_Spectator.AddOverviewEntityToList( gHUD.m_Spectator.m_hsprGrenade, entity,
					gEngfuncs.GetClientTime() - 1.0 );
		}
		if ( ( g_iTeamNumber == 2 || gEngfuncs.IsSpectateOnly() ) && strstr( modelname, "w_stick" ) &&
			( !minimap || cl_dmshowgrenades->value > 0.0 ) )
			gHUD.m_Spectator.AddOverviewEntityToList( gHUD.m_Spectator.m_hsprStick, entity,
				gEngfuncs.GetClientTime() - 1.0 );
		if ( strstr( modelname, "tnt" ) && ( !minimap || cl_dmshowobjects->value > 0.0 ) )
			gHUD.m_Spectator.AddOverviewEntityToList( gHUD.m_Spectator.m_hsprTnT, entity,
				gEngfuncs.GetClientTime() - 1.0 );
		if ( strstr( modelname, "bangalore" ) && ( !minimap || cl_dmshowobjects->value > 0.0 ) )
			gHUD.m_Spectator.AddOverviewEntityToList( gHUD.m_Spectator.m_hsprBanglr, entity,
				gEngfuncs.GetClientTime() - 1.0 );
		if ( strstr( modelname, "markernull" ) )
		{
			HSPRITE marker = gHUD.m_Spectator.GetMarkerSPR( entity->curstate.body );
			if ( marker )
				gHUD.m_Spectator.AddOverviewEntityToList( marker, entity, gEngfuncs.GetClientTime() + 0.2f );
		}
		const char* custom = strstr( modelname, "hltv_" );
		if ( custom && ( entity->curstate.rendermode != kRenderTransAdd || entity->curstate.renderamt ) &&
			( !minimap || cl_dmshowobjects->value > 0.0 ) )
		{
			char file[64];
			char sprite[128];
			strcpy( file, custom );
			file[strlen( file ) - 3] = '\0';
			strcat( file, "spr" );
			sprintf( sprite, "sprites/%s", file );
			gHUD.m_Spectator.m_hsprCustom = gEngfuncs.pfnSPR_Load( sprite );
			gHUD.m_Spectator.AddOverviewEntityToList( gHUD.m_Spectator.m_hsprCustom, entity,
				gEngfuncs.GetClientTime() - 1.0 );
		}
	}
	if ( g_iUser1 || ( minimap && cl_dmshowplayers->value > 0.0 ) )
	{
		gHUD.m_Spectator.AddOverviewEntity( type, entity, modelname );
		return ( g_iUser1 != 4 && gHUD.m_Spectator.m_pip->value != 2.0 ) || entity->index != g_iUser2;
	}
	return 1;
}

/*
=========================
HUD_TxferLocalOverrides

The server sends us our origin with extra precision as part of the clientdata structure, not during the normal
playerstate update in entity_state_t.  In order for these overrides to eventually get to the appropriate playerstate
structure, we need to copy them into the state structure at this point.
=========================
*/
void CL_DLLEXPORT HUD_TxferLocalOverrides( entity_state_s* state, const clientdata_s* client )
{
	g_pcldstAddrs->pTxferLocalOverrides( &state, &client );
//	RecClTxferLocalOverrides(state, client);

	VectorCopy( client->origin, state->origin );

	// Spectator
	state->iuser1 = client->iuser1;
	state->iuser2 = client->iuser2;

	// Duck prevention
	state->iuser3 = client->iuser3;
	state->iuser4 = client->iuser4;
	state->fuser4 = client->fuser4;
	VectorCopy( client->vuser1, state->vuser1 );
}

void CL_DLLEXPORT HUD_ProcessPlayerState( entity_state_s* destination, const entity_state_s* source )
{
	g_pcldstAddrs->pProcessPlayerState( &destination, &source );
	VectorCopy( source->origin, destination->origin );
	VectorCopy( source->angles, destination->angles );
	VectorCopy( source->velocity, destination->velocity );
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
	memcpy( destination->blending, source->blending, sizeof( short ) );
	VectorCopy( source->basevelocity, destination->basevelocity );
	destination->friction = source->friction;
	destination->gravity = source->gravity;
	destination->gaitsequence = source->gaitsequence;
	destination->spectator = source->spectator;
	destination->usehull = source->usehull;
	destination->playerclass = source->playerclass;
	destination->team = source->team;
	destination->colormap = source->colormap;
	if ( gEngfuncs.IsSpectateOnly() )
	{
		g_PlayerExtraInfo[destination->number].teamnumber = source->team;
		g_PlayerExtraInfo[destination->number].playerclass = source->playerclass;
	}
	destination->iuser1 = source->iuser1;
	destination->iuser2 = source->iuser2;
	destination->iuser3 = source->iuser3;
	destination->iuser4 = source->iuser4;
	destination->fuser4 = source->fuser4;
	VectorCopy( source->vuser1, destination->vuser1 );
	g_PlayerExtraInfo[destination->number].health = source->health;
	if ( destination->number == gEngfuncs.GetLocalPlayer()->index )
	{
		g_iPlayerClass = destination->playerclass;
		g_iTeamNumber = destination->team;
		g_iUser1 = source->iuser1;
		g_iUser2 = source->iuser2;
		if ( g_iUser3 == 3 )
		{
			if ( source->iuser3 != 3 )
				gHUD.SetMortarUnDeployTime();
		}
		else if ( source->iuser3 == 3 )
		{
			gHUD.SetMortarDeployTime();
		}
		g_iUser3 = source->iuser3;
		g_iWeaponBits2 = source->iuser4;
		g_iVuser1x = (int)source->vuser1[0];
		g_iVuser1z = (int)source->vuser1[2];
		g_iMovetype = source->movetype;
		g_iEffects = source->effects;
		g_fStamina = source->fuser4;
	}
}

void CL_DLLEXPORT HUD_TxferPredictionData( entity_state_s* state, const entity_state_s* previous,
	clientdata_s* client, const clientdata_s* oldClient, weapon_data_s* weapons, const weapon_data_s* oldWeapons )
{
	g_pcldstAddrs->pTxferPredictionData( &state, &previous, &client, &oldClient, &weapons, &oldWeapons );
	state->oldbuttons = previous->oldbuttons;
	state->flFallVelocity = previous->flFallVelocity;
	state->iStepLeft = previous->iStepLeft;
	state->playerclass = previous->playerclass;
	client->viewmodel = oldClient->viewmodel;
	client->m_iId = oldClient->m_iId;
	client->ammo_shells = oldClient->ammo_shells;
	client->ammo_rockets = oldClient->ammo_rockets;
	client->m_flNextAttack = oldClient->m_flNextAttack;
	client->fov = oldClient->fov;
	client->weaponanim = oldClient->weaponanim;
	client->tfstate = oldClient->tfstate;
	client->maxspeed = oldClient->maxspeed;
	client->deadflag = oldClient->deadflag;
	client->iuser1 = oldClient->iuser1;
	g_iAlive = oldClient->iuser1 || !oldClient->deadflag;
	g_iDeadFlag = oldClient->deadflag;
	client->iuser2 = oldClient->iuser2;
	client->iuser3 = oldClient->iuser3;
	client->iuser4 = oldClient->iuser4;
	if ( gEngfuncs.IsSpectateOnly() )
	{
		client->iuser1 = g_iUser1;
		client->iuser2 = g_iUser2;
		client->iuser3 = g_iUser3;
	}
	client->fuser2 = oldClient->fuser2;
	client->fuser4 = oldClient->fuser4;
	client->vuser1 = oldClient->vuser1;
	memcpy( weapons, oldWeapons, sizeof( *weapons ) * MAX_WEAPONS );
}

void CL_DLLEXPORT HUD_CreateEntities()
{
	g_pcldstAddrs->pCreateEntities();

	// Add in any game specific objects
	Game_AddObjects();
	GetClientVoiceHud()->CreateEntities();
	gHUD.m_StatusBar.CreateEntities();
}

void Event_EjectBrassP( const cl_entity_s* entity, int shellType )
{
	int model = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shells.mdl" );
	if ( cl_bulletejects->value == 0.0 || (unsigned int)shellType > 3 )
		return;
	Vector angles = entity->angles;
	VectorNormalize( angles );
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	Vector velocity = up * 100;
	Vector shellAngles( 0.0f, entity->angles.y, 0.0f );
	TEMPENTITY* shell = gEngfuncs.pEfxAPI->R_TempModel( (float*)&entity->attachment[1], velocity,
		shellAngles, 2.5, model, TE_BOUNCE_SHELL );
	shell->entity.curstate.body = shellType;
}

void Event_EjectBrassV( const cl_entity_s* entity, int shellType )
{
	int model = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shells.mdl" );
	if ( cl_bulletejects->value == 0.0 || (unsigned int)shellType > 3 )
		return;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( v_angles, forward, right, up );
	Vector velocity = right * (float)gEngfuncs.pfnRandomLong( 60, 80 );
	Vector lift = up * (float)gEngfuncs.pfnRandomLong( 60, 80 );
	velocity = velocity + lift;
	Vector shellAngles( 0.0f, entity->angles.y, 0.0f );
	TEMPENTITY* shell = gEngfuncs.pEfxAPI->R_TempModel( (float*)&entity->attachment[1], velocity,
		shellAngles, 2.5, model, TE_BOUNCE_SHELL );
	shell->entity.curstate.body = shellType;
}

void Event_MuzzleFlash( const cl_entity_s* entity, int options )
{
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( entity->angles, forward, right, up );
	float scale = ( options / 10 ) * 0.1;
	int type = options % 10 % 4;
	if ( scale == 0.0 )
		scale = 0.5;
	model_s* mgSprite = NULL;
	if ( type == 3 )
		mgSprite = (model_s*)gEngfuncs.GetSpritePointer( gEngfuncs.pfnSPR_Load( "sprites/muzzleflash1.spr" ) );
	model_s* sprite = (model_s*)gEngfuncs.GetSpritePointer( gEngfuncs.pfnSPR_Load( "sprites/muzzleflash2.spr" ) );
	if ( !sprite )
	{
		gEngfuncs.Con_DPrintf( "Couldn't load Sprite: %s\n", "sprites/muzzleflash2.spr" );
		return;
	}
	forward.z = -forward.z;
	for ( int i = 0; i < 3; i++ )
	{
		Vector origin = entity->attachment[0] + forward * (float)( 12 - 4 * i );
		bool machinegun = i == 2 && type == 3;
		float size = machinegun ? scale * 120.0 : scale * (float)( 20 * ( i + 1 ) );
		CDoDParticle* particle = CDoDParticle::Create( origin, Vector( 0, 0, 0 ), machinegun ? mgSprite : sprite,
			size, machinegun ? 210.0f : 255.0f, "dod_particle", true );
		if ( !particle )
			return;
		if ( !machinegun )
			particle->m_vAngles.z = gEngfuncs.pfnRandomFloat( 0, 90 );
		particle->m_vVelocity = Vector( 0, 0, 0 );
		particle->m_flDieTime = gEngfuncs.GetClientTime() + 0.0001;
		particle->m_iRendermode = kRenderTransAdd;
		particle->SetLightFlag( LIGHT_NONE );
		particle->SetCullFlag( CULL_FRUSTUM_PLANE );
		particle->m_vColor = Vector( 255, 255, 255 );
	}
}

void Event_MuzzleSmoke( const cl_entity_s* entity, int options )
{
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( entity->angles, forward, right, up );
	float scale = ( options / 10 ) * 0.1;
	if ( scale == 0.0 )
		scale = 0.5;
	model_s* sprite = (model_s*)gEngfuncs.GetSpritePointer( gEngfuncs.pfnSPR_Load( "sprites/bazookapuff.spr" ) );
	if ( !sprite )
	{
		gEngfuncs.Con_DPrintf( "Couldn't load Sprite: %s\n", "sprites/bazookapuff.spr" );
		return;
	}
	forward.z = -forward.z;
	for ( int i = 1; i <= 3; i++ )
	{
		CDoDParticle* particle = CDoDParticle::Create( entity->attachment[0], Vector( 0, 0, 0 ), sprite,
			scale * 100.0, 20, "dod_particle", true );
		if ( !particle )
			return;
		particle->m_vVelocity = forward;
		VectorScale( particle->m_vVelocity, (float)( i * 100 ), particle->m_vVelocity );
		particle->SetCollisionFlags( TRI_ANIMATEDIE );
		particle->m_iRendermode = kRenderTransAdd;
		particle->SetLightFlag( LIGHT_COLOR );
		particle->SetCullFlag( CULL_PVS | CULL_FRUSTUM_PLANE );
		particle->m_flDampingTime = 0.3f;
		particle->m_flDampingVelocity = 0.5;
		particle->m_flFadeSpeed = 2;
		particle->m_flScaleSpeed = 6;
		particle->m_iFrame = 0;
		particle->m_iFramerate = 15;
		particle->m_vColor = Vector( 200, 200, 200 );
		particle->m_flDieTime = gEngfuncs.GetClientTime() + 5;
		particle->AddGlobalWind();
	}
}

void Event_BazookaSmoke( const cl_entity_s* entity )
{
	model_s* sprite = (model_s*)gEngfuncs.GetSpritePointer(
		gEngfuncs.pfnSPR_Load( "sprites/effects/adrian/bazooka_smoke.spr" ) );
	if ( !sprite )
		return;
	float scale = gEngfuncs.pfnRandomFloat( 40, 55 );
	for ( int i = 0; i < 2; i++ )
	{
		CDoDRocketTrail* particle = CDoDRocketTrail::Create( entity->attachment[i], Vector( 0, 0, 1 ),
			sprite, scale, 200, "dod_trailsmoke" );
		if ( !particle )
			return;
		particle->m_flGravity = -0.01f;
		particle->m_iRendermode = kRenderTransAlpha;
		particle->m_flSize = scale;
		particle->SetCollisionFlags( TRI_COLLIDEWORLD );
		particle->SetLightFlag( LIGHT_NONE );
		particle->SetCullFlag( CULL_PVS | CULL_FRUSTUM_SPHERE );
		particle->m_vAVelocity = Vector( 0.0f, 0.0f, gEngfuncs.pfnRandomFloat( -1.5, 1.5 ) );
		particle->m_flMass = gEngfuncs.pfnRandomFloat( 2, 3 );
		particle->m_flDieTime = gEngfuncs.GetClientTime() + 10;
		float frame = gEngfuncs.pfnRandomFloat( 10, 15 );
		particle->m_flScaleSpeed = 2;
		particle->m_vColor = Vector( 25, 25, 25 );
		particle->m_iFrame = (int)frame;
		particle->m_bRocketTrail = true;
		particle->m_flFadeSpeed = -1;
		particle->m_vVelocity = Vector( 0, 0, 0 );
	}
}

void Event_PIATSmoke( const cl_entity_s* entity )
{
	model_s* sprite = (model_s*)gEngfuncs.GetSpritePointer(
		gEngfuncs.pfnSPR_Load( "sprites/effects/adrian/bazooka_smoke.spr" ) );
	if ( !sprite )
		return;
	float scale = gEngfuncs.pfnRandomFloat( 40, 55 );
	CDoDRocketTrail* particle = CDoDRocketTrail::Create( entity->attachment[0], Vector( 0, 0, 1 ),
		sprite, scale, 200, "dod_trailsmoke" );
	if ( !particle )
		return;
	particle->m_flGravity = -0.01f;
	particle->m_iRendermode = kRenderTransAlpha;
	particle->m_flSize = scale;
	particle->SetCollisionFlags( TRI_COLLIDEWORLD );
	particle->SetLightFlag( LIGHT_NONE );
	particle->SetCullFlag( CULL_PVS | CULL_FRUSTUM_SPHERE );
	particle->m_vAVelocity = Vector( 0.0f, 0.0f, gEngfuncs.pfnRandomFloat( -1.5, 1.5 ) );
	particle->m_flMass = gEngfuncs.pfnRandomFloat( 2, 3 );
	particle->m_flDieTime = gEngfuncs.GetClientTime() + 10;
	float frame = gEngfuncs.pfnRandomFloat( 10, 15 );
	particle->m_flScaleSpeed = 2;
	particle->m_vColor = Vector( 25, 25, 25 );
	particle->m_iFrame = (int)frame;
	particle->m_bRocketTrail = true;
	particle->m_flFadeSpeed = -1;
	particle->m_vVelocity = Vector( 0, 0, 0 );
}

void CL_DLLEXPORT HUD_StudioEvent( const mstudioevent_s* event, const cl_entity_s* entity )
{
	g_pcldstAddrs->pStudioEvent( &event, &entity );
	int options = atoi( event->options );
	switch ( event->event )
	{
	case 5001:
		Event_MuzzleFlash( entity, options );
		Event_MuzzleSmoke( entity, options );
		break;
	case 5002:
		gEngfuncs.pEfxAPI->R_SparkEffect( (float*)&entity->attachment[0], atoi( event->options ), -100, 100 );
		break;
	// Client side sound
	case 5004:
		gEngfuncs.pfnPlaySoundByNameAtLocation( (char*)event->options, 1.0, (float*)&entity->attachment[0] );
		break;
	case 5011:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float*)&entity->attachment[1], options );
		break;
	case 5021:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float*)&entity->attachment[2], options );
		break;
	case 5031:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float*)&entity->attachment[3], options );
		break;
	case 6001:
		Event_EjectBrassP( entity, options );
		break;
	case 6002:
		Event_EjectBrassV( entity, options );
		break;
	case 7000:
		if ( entity->player )
		{
			pmodel_fx_t& effect = g_PModelFxInfo[entity->index];
			effect.bSwitch = true;
			effect.iSwitchSeq = entity->curstate.sequence;
			effect.iSwitchFrame = options ? options : -1;
		}
		break;
	case 7001:
		Event_BazookaSmoke( entity );
		break;
	case 7002:
		if ( entity->player )
		{
			pmodel_fx_t& effect = g_PModelFxInfo[entity->index];
			effect.bAnim = true;
			effect.iAnimSeq = entity->curstate.sequence;
			effect.iAnimFrame = event->frame;
			effect.iAnimTargetSeq = options ? options : 1;
		}
		break;
	case 7003:
		Event_PIATSmoke( entity );
		break;
	}
}

cl_entity_s CL_DLLEXPORT* HUD_GetUserEntity( int index )
{
	g_pcldstAddrs->pGetUserEntity( &index );
	return NULL;
}

void CL_DLLEXPORT HUD_TempEntUpdate( double frametime, double clientTime, double gravityValue,
	TEMPENTITY** freeList, TEMPENTITY** activeList,
	int ( *addVisible )( cl_entity_s* ), void ( *playSound )( TEMPENTITY*, float ) )
{
	static int gTempEntFrame;
	g_pcldstAddrs->pTempEntUpdate( &frametime, &clientTime, &gravityValue,
		&freeList, &activeList, &addVisible, &playSound );
	g_flGravity = (float)gravityValue;
	if ( g_pParticleMan )
		g_pParticleMan->SetVariables( g_flGravity, gHUD.m_vecAngles );
	if ( !*activeList )
		return;

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL 
	// tent, then set this BOOL to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers( -1 );

	// !!!BUGBUG	-- This needs to be time based
	gTempEntFrame = ( gTempEntFrame + 1 ) & 31;
	if ( frametime <= 0.0 )
	{
		for ( TEMPENTITY* temp = *activeList; temp; temp = temp->next )
		{
			if ( !( temp->flags & FTENT_NOMODEL ) )
				addVisible( &temp->entity );
		}
		gEngfuncs.pEventAPI->EV_PopPMStates();
		return;
	}
	float fastFrequency = 5.5 * clientTime;
	float gravity = -frametime * gravityValue;
	TEMPENTITY* previous = NULL;
	TEMPENTITY* temp = *activeList;
	while ( temp )
	{
		TEMPENTITY* next = temp->next;
		int flags = temp->flags;
		if ( !( flags & FTENT_ENVMODEL ) )
		{
			float life = temp->die - clientTime;
			bool active = true;
			if ( life < 0.0 )
			{
				if ( flags & FTENT_FADEOUT )
				{
					if ( temp->entity.curstate.rendermode == kRenderNormal )
						temp->entity.curstate.rendermode = kRenderTransTexture;
					float amount = temp->entity.baseline.renderamt * ( life * temp->fadeSpeed + 1.0 );
					temp->entity.curstate.renderamt = (int)amount;
					active = temp->entity.curstate.renderamt > 0;
				}
				else
				{
					active = false;
				}
			}
			if ( !active )
			{
				temp->die = -1;
				temp->next = *freeList;
				*freeList = temp;
				if ( previous )
					previous->next = next;
				else
					*activeList = next;
				temp = next;
				continue;
			}
		}
		VectorCopy( temp->entity.origin, temp->entity.prevstate.origin );
		if ( flags & FTENT_SPARKSHOWER )
		{
			// Adjust speed if it's time
			// Scale is next think time
			if ( clientTime > temp->entity.baseline.scale )
			{
				gEngfuncs.pEfxAPI->R_SparkEffect( temp->entity.origin, 8, -200, 200 );
				temp->entity.baseline.framerate -= 0.1;
				if ( temp->entity.baseline.framerate > 0.0 )
				{
					temp->die = clientTime + 0.5;
					temp->entity.baseline.scale = clientTime + 0.1;
				}
				else
				{
					temp->die = clientTime;
				}
				flags = temp->flags;
			}
		}
		else if ( flags & FTENT_PLYRATTACHMENT )
		{
			cl_entity_s* owner = gEngfuncs.GetEntityByIndex( temp->clientIndex );
			if ( temp->hitSound )
				temp->entity.origin = owner->attachment[temp->hitSound - 1];
			else
				temp->entity.origin = owner->origin + temp->tentOffset;
		}
		else if ( flags & FTENT_SINEWAVE )
		{
			temp->x += temp->entity.baseline.origin[0] * frametime;
			temp->y += temp->entity.baseline.origin[1] * frametime;
			temp->entity.origin.x = temp->x + sin( temp->entity.baseline.origin[2] +
				clientTime * temp->entity.prevstate.frame ) * ( 10.0 * temp->entity.curstate.framerate );
			temp->entity.origin.y = temp->y + sin( temp->entity.baseline.origin[2] +
				fastFrequency + 0.7 ) * ( 8.0 * temp->entity.curstate.framerate );
			temp->entity.origin.z += temp->entity.baseline.origin[2] * frametime;
		}
		else if ( flags & FTENT_SPIRAL )
		{
			temp->entity.origin.x += temp->entity.baseline.origin[0] * frametime +
				8 * sin( clientTime * 20 + (int)temp );
			temp->entity.origin.y += temp->entity.baseline.origin[1] * frametime +
				4 * sin( clientTime * 30 + (int)temp );
			temp->entity.origin.z += temp->entity.baseline.origin[2] * frametime;
		}
		else
		{
			for ( int i = 0; i < 3; i++ )
				temp->entity.origin[i] += temp->entity.baseline.origin[i] * frametime;
		}
		if ( flags & FTENT_SPRANIMATE )
		{
			temp->entity.curstate.frame += temp->entity.curstate.framerate * frametime;
			if ( temp->entity.curstate.frame >= temp->frameMax )
			{
				temp->entity.curstate.frame -= (int)temp->entity.curstate.frame;
				if ( ( flags & FTENT_LONGTRACE ) || !( flags & FTENT_SPRANIMATELOOP ) )
				{
					if ( !( flags & FTENT_LONGTRACE ) )
						temp->die = clientTime;
					previous = temp;
					temp = next;
					continue;
				}
			}
		}
		else if ( flags & FTENT_SPRCYCLE )
		{
			temp->entity.curstate.frame += 10.0 * frametime;
			if ( temp->entity.curstate.frame >= temp->frameMax )
				temp->entity.curstate.frame -= (int)temp->entity.curstate.frame;
		}
		if ( flags & FTENT_ROTATE )
		{
			for ( int i = 0; i < 3; i++ )
				temp->entity.angles[i] += temp->entity.baseline.angles[i] * frametime;
			VectorCopy( temp->entity.angles, temp->entity.latched.prevangles );
		}
		if ( ( flags & ( FTENT_COLLIDEALL | FTENT_COLLIDEWORLD ) ) && !( flags & FTENT_SKIPPHYSICS ) )
		{
			float fraction = 1.0;
			Vector normal;
			pmtrace_t trace;
			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			if ( flags & FTENT_COLLIDEALL )
			{
				gEngfuncs.pEventAPI->EV_PlayerTrace( temp->entity.prevstate.origin, temp->entity.origin,
					PM_STUDIO_BOX, -1, &trace );
				if ( trace.fraction != 1.0 )
				{
					physent_t* hit = gEngfuncs.pEventAPI->EV_GetPhysent( trace.ent );
					if ( !trace.ent || hit->info != temp->clientIndex )
					{
						fraction = trace.fraction;
						normal = trace.plane.normal;
						if ( temp->hitcallback )
							temp->hitcallback( temp, &trace );
					}
				}
			}
			else
			{
				Vector end = temp->entity.origin;
				if ( temp->flags & FTENT_LONGTRACE )
				{
					Vector direction = temp->entity.baseline.origin;
					VectorNormalize( direction );
					end = Vector( temp->entity.prevstate.origin ) + direction * 36;
				}
				gEngfuncs.pEventAPI->EV_PlayerTrace( temp->entity.prevstate.origin, end,
					PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &trace );
				if ( trace.fraction != 1.0 )
				{
					fraction = trace.fraction;
					VectorCopy( trace.plane.normal, normal );
					if ( temp->flags & FTENT_SPARKSHOWER )
					{
						VectorScale( temp->entity.baseline.origin, 0.6f, temp->entity.baseline.origin );
						if ( Length( temp->entity.baseline.origin ) < 10.0 )
							temp->entity.baseline.framerate = 0;
					}
					if ( temp->hitcallback )
						temp->hitcallback( temp, &trace );
				}
			}
			if ( fraction != 1.0 )
			{
				VectorMA( temp->entity.prevstate.origin, fraction * frametime,
					temp->entity.baseline.origin, temp->entity.origin );
				float damping = temp->bounceFactor;
				if ( temp->flags & ( FTENT_GRAVITY | FTENT_SLOWGRAVITY ) )
				{
					damping *= 0.5;
					if ( normal.z > 0.9 && temp->entity.baseline.origin[2] <= 0.0 &&
						temp->entity.baseline.origin[2] >= gravity * 3.0 )
					{
						damping = 0;
						temp->flags &= ~( FTENT_ROTATE | FTENT_GRAVITY | FTENT_SLOWGRAVITY |
							FTENT_COLLIDEWORLD | FTENT_SMOKETRAIL );
						temp->entity.angles.x = 0;
						temp->entity.angles.z = 0;
					}
				}
				if ( temp->hitSound )
					playSound( temp, damping );
				if ( temp->flags & FTENT_COLLIDEKILL )
				{
					temp->flags &= ~FTENT_FADEOUT;
					temp->die = clientTime;
				}
				else
				{
					if ( damping != 0.0 )
					{
						float projection = DotProduct( temp->entity.baseline.origin, normal );
						VectorMA( temp->entity.baseline.origin, -projection - projection, normal,
							temp->entity.baseline.origin );
						temp->entity.angles.y = -temp->entity.angles.y;
					}
					if ( damping != 1.0 )
					{
						VectorScale( temp->entity.baseline.origin, damping, temp->entity.baseline.origin );
						VectorScale( temp->entity.angles, 0.9f, temp->entity.angles );
					}
				}
			}
			flags = temp->flags;
		}
		if ( ( flags & FTENT_FLICKER ) && temp->entity.curstate.effects == gTempEntFrame )
		{
			dlight_t* light = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
			light->origin = temp->entity.origin;
			light->radius = 60;
			light->color.r = 255;
			light->color.g = 120;
			light->color.b = 0;
			light->die = clientTime + 0.01;
			flags = temp->flags;
		}
		if ( flags & FTENT_SMOKETRAIL )
		{
			gEngfuncs.pEfxAPI->R_RocketTrail( temp->entity.prevstate.origin, temp->entity.origin, 1 );
			flags = temp->flags;
		}
		if ( !( flags & FTENT_SKIPPHYSICS ) )
		{
			if ( flags & FTENT_GRAVITY )
				temp->entity.baseline.origin[2] += gravity;
			else if ( flags & FTENT_SLOWGRAVITY )
				temp->entity.baseline.origin[2] += (float)( gravity * 0.5 );
			else if ( flags & FTENT_GRAVITYFACTOR )
				temp->entity.baseline.origin[2] += gravity * temp->entity.baseline.gravity;
		}
		if ( ( flags & FTENT_CLIENTCUSTOM ) && temp->callback )
		{
			temp->callback( temp, (float)frametime, (float)clientTime );
			flags = temp->flags;
		}
		if ( flags & FTENT_ENVMODEL )
		{
			temp->entity.model->mins = Vector( -999, -999, -999 );
			temp->entity.model->maxs = Vector( 999, 999, 999 );
			addVisible( &temp->entity );
		}
		else if ( !( flags & FTENT_NOMODEL ) && !addVisible( &temp->entity ) && !( temp->flags & FTENT_PERSIST ) )
		{
			temp->die = clientTime;
			temp->flags &= ~FTENT_FADEOUT;
		}
		previous = temp;
		temp = next;
	}
	gEngfuncs.pEventAPI->EV_PopPMStates();
}
