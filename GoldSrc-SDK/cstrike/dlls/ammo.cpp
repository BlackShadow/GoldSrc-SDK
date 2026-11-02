#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

class C9MMAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* pOther )
	{
		if ( pOther->GiveAmmo( 30, "9mm", MAX_AMMO_9MM ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_9mm, C9MMAmmo );

class CBuckShotAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		Precache();
		SET_MODEL( edict(), "models/w_shotbox.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_shotbox.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* pOther )
	{
		if ( pOther->GiveAmmo( 8, "buckshot", MAX_AMMO_BUCKSHOT ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_buckshot, CBuckShotAmmo );

class C556NatoAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* pOther )
	{
		if ( pOther->GiveAmmo( 30, "556Nato", MAX_AMMO_556NATO ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_556nato, C556NatoAmmo );

class C556NatoBoxAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* pOther )
	{
		if ( pOther->GiveAmmo( 30, "556NatoBox", MAX_AMMO_556NATOBOX ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_556natobox, C556NatoBoxAmmo );

class C762NatoAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* pOther )
	{
		if ( pOther->GiveAmmo( 30, "762Nato", MAX_AMMO_762NATO ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_762nato, C762NatoAmmo );

class C45ACPAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* pOther )
	{
		if ( pOther->GiveAmmo( 12, "45acp", MAX_AMMO_45ACP ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_45acp, C45ACPAmmo );

class C50AEAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* pOther )
	{
		if ( pOther->GiveAmmo( 7, "50AE", MAX_AMMO_50AE ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_50ae, C50AEAmmo );

class C338MagnumAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* pOther )
	{
		if ( pOther->GiveAmmo( 10, "338Magnum", MAX_AMMO_338MAGNUM ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_338magnum, C338MagnumAmmo );

class C57MMAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* pOther )
	{
		if ( pOther->GiveAmmo( 50, "57mm", MAX_AMMO_57MM ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_57mm, C57MMAmmo );

class C357SIGAmmo : public CBasePlayerAmmo
{
public:
	void Spawn( void )
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* pOther )
	{
		if ( pOther->GiveAmmo( 13, "357SIG", MAX_AMMO_357SIG ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_357sig, C357SIGAmmo );
