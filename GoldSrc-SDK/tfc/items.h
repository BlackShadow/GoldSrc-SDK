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

#pragma once

class CBasePlayer;

class CItem : public CBaseEntity
{
public:
	void Spawn();
	void KeyValue( KeyValueData* pkvd );
	CBaseEntity* Respawn();
	void EXPORT ItemTouch( CBaseEntity* pOther );
	void EXPORT Materialize();
	void TeamFortress_EMPRemove();
	virtual BOOL MyTouch( CBasePlayer* pPlayer );

	float	m_flRespawnTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
