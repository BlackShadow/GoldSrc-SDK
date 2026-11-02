/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef TRAINS_H
#define TRAINS_H

#define SF_TRACKTRAIN_NOPITCH		1
#define SF_TRACKTRAIN_NOCONTROL		2
#define SF_TRACKTRAIN_FORWARDONLY	4
#define SF_TRACKTRAIN_PASSABLE		8
#define SF_PATH_DISABLED				1
#define SF_PATH_FIREONCE				2
#define SF_PATH_ALTREVERSE			4
#define SF_PATH_DISABLE_TRAIN		8
#define SF_PATH_ALTERNATE			32768
#define SF_CORNER_WAITFORTRIG		1
#define SF_CORNER_TELEPORT			2
#define SF_CORNER_FIREONCE			4

//#define PATH_SPARKLE_DEBUG		1	// This makes a particle effect around path_track entities for debugging
class CPathTrack : public CPointEntity
{
public:
	void RoundRespawn() override;
	void Spawn() override;
	void Activate() override;
	void KeyValue( KeyValueData* pkvd ) override;
	void SetPrevious( CPathTrack* previous );
	void Link();
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;
	CPathTrack* ValidPath( CPathTrack* path, int testFlag );
	void Project( CPathTrack* start, CPathTrack* end, Vector* origin, float dist );
	static CPathTrack* Instance( edict_t* pent );
	CPathTrack* LookAhead( Vector* origin, float dist, int move );
	CPathTrack* Nearest( Vector origin );
	CPathTrack* GetNext();
	CPathTrack* GetPrevious();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	float		m_length;
	string_t	m_altName;
	CPathTrack*	m_pnext;
	CPathTrack*	m_pprevious;
	CPathTrack*	m_paltpath;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CFuncTrackTrain : public CBaseEntity
{
public:
	void RoundRespawn() override;
	void Spawn() override;
	void Precache() override;
	void Blocked( CBaseEntity* pOther ) override;
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;
	void KeyValue( KeyValueData* pkvd ) override;
	void EXPORT Next();
	void EXPORT Find();
	void EXPORT NearestPath();
	void EXPORT DeadEnd();
	void NextThink( float thinkTime, BOOL alwaysThink );
	void SetTrack( CPathTrack* track ) { m_ppath = track->Nearest( pev->origin ); }
	void SetControls( entvars_t* pevControls );
	BOOL OnControls( entvars_t* pev ) override;
	void StopSound();
	void UpdateSound();
	static CFuncTrackTrain* Instance( edict_t* pent );
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	int ObjectCaps() override { return ( CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ) | FCAP_DIRECTIONAL_USE; }
	void OverrideReset() override;
	virtual void SetSpeed( float speed );
	virtual void SetVelocity( Vector velocity );
	virtual void SetAvelocity( Vector velocity );
	virtual void SetAngles( Vector angles );
	static TYPEDESCRIPTION m_SaveData[];

	CPathTrack*		m_ppath;
	float			m_length;
	float			m_height;
	float			m_speed;
	float			m_dir;
	float			m_startSpeed;
	Vector			m_controlMins;
	Vector			m_controlMaxs;
	int				m_soundPlaying;
	int				m_sounds;
	float			m_flVolume;
	float			m_flBank;
	float			m_oldSpeed;
	CBaseEntity*	pTurret;

private:
	unsigned short m_usAdjustPitch;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPathCorner : public CPointEntity
{
public:
	void RoundRespawn() override {}
	void Spawn() override;
	void KeyValue( KeyValueData* pkvd ) override;
	float GetDelay() override { return m_flWait; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int		m_iPathFOV;
	int		m_iOverlay;
	int		m_iSubtitle;
	int		m_iCamFade;
	float	m_flWait;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

#endif
