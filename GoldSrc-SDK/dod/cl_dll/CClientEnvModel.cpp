#include "hud.h"
#include "cl_util.h"
#include "event_api.h"
#include "r_efx.h"
#include "r_studioint.h"
#include "com_model.h"
#include "studio.h"
#include "pmtrace.h"
#include "pm_defs.h"

#define FTENT_ENVMODEL	0x00100000

extern engine_studio_api_t IEngineStudio;
extern Vector v_origin;

int GetSequence( void* model, const char* label )
{
	if ( !model )
		return 0;
	studiohdr_t* header = (studiohdr_t*)model;
	mstudioseqdesc_t* sequence = (mstudioseqdesc_t*)( (byte*)header + header->seqindex );
	for ( int i = 0; i < header->numseq; ++i )
	{
		if ( !stricmp( sequence[i].label, label ) )
			return i;
	}
	return 0;
}

void EV_EnvModelCallback( TEMPENTITY* ent, float, float )
{
	if ( gEngfuncs.IsSpectateOnly() )
		ent->entity.origin = v_origin;
	else
		ent->entity.origin = gEngfuncs.GetLocalPlayer()->origin;
}

int CClientEnvModel::Init()
{
	m_iFlags |= HUD_ACTIVE;
	memset( m_sEnvModels, 0, sizeof( m_sEnvModels ) );
	gHUD.AddHudElem( this );
	return 1;
}

int CClientEnvModel::VidInit()
{
	return 1;
}

void CClientEnvModel::RemoveAllModels()
{
	memset( m_sEnvModels, 0, sizeof( m_sEnvModels ) );
	m_iNumEnvModels = 0;
	if ( m_teEnvModelTE )
		m_teEnvModelTE->die = 0;
	m_teEnvModelTE = NULL;
}

void CClientEnvModel::AddEnvModel( env_model_t* model )
{
	if ( m_iNumEnvModels >= 192 )
	{
		gEngfuncs.Con_DPrintf( "CClientEnvModel::AddEnvModel: Too many static env_models! Limit is %d\n", 192 );
		return;
	}
	env_model_t* env = &m_sEnvModels[m_iNumEnvModels];
	*env = *model;
	gEngfuncs.CL_LoadModel( env->szModel, &env->iModel );
	env->pModel = IEngineStudio.GetModelByIndex( env->iModel );
	env->sequence = GetSequence( IEngineStudio.Mod_Extradata( env->pModel ), env->szSequence );
	if ( model->spawnflags & 2 )
	{
		Vector end( env->vecOrigin.x, env->vecOrigin.y, -8000.0f );
		float oldz = env->vecOrigin.z;
		pmtrace_t trace;
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( env->vecOrigin, end, PM_STUDIO_IGNORE | PM_GLASS_IGNORE, -1, &trace );
		env->vecOrigin.z = (float)(int)( trace.fraction == 1 ? oldz : trace.endpos[2] );
	}
	++m_iNumEnvModels;
}

void CClientEnvModel::Think()
{
	if ( m_teEnvModelTE )
	{
		if ( m_teEnvModelTE->die >= 0 )
			return;
		gEngfuncs.Con_DPrintf( "Warning! CClientEnvModel::m_teEnvModelTE died somehow, tell Mugsy!\n" );
	}
	Vector origin( 0, 0, 0 );
	int modelindex = 0;
	gEngfuncs.CL_LoadModel( "models/p_stick.mdl", &modelindex );
	model_t* model = IEngineStudio.GetModelByIndex( modelindex );
	m_teEnvModelTE = gEngfuncs.pEfxAPI->CL_TempEntAlloc( origin, model );
	if ( m_teEnvModelTE )
	{
		m_teEnvModelTE->flags |= FTENT_CLIENTCUSTOM | FTENT_PERSIST | FTENT_ENVMODEL;
		m_teEnvModelTE->callback = EV_EnvModelCallback;
		m_teEnvModelTE->entity.curstate.entityType = 4;
	}
}
