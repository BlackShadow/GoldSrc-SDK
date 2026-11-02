#pragma once

#define CTF_ITEM_JUMP		4
#define CTF_ITEM_SHIELD		8
#define CTF_ITEM_BACKPACK	16
#define CTF_ITEM_ACCELERATOR	32
#define CTF_ITEM_REGENERATION	128
#define CTF_POWERUP_MASK		252
#define CLASS_CTFITEM		30

class CBasePlayer;

class CItemCTF : public CBaseAnimating
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData* pkvd );
	void SetObjectCollisionBox();
	void EXPORT DropPreThink();
	void EXPORT DropThink();
	void EXPORT CarryThink();
	void EXPORT ItemTouch( CBaseEntity* pOther );
	virtual void RemoveEffect( CBasePlayer* pPlayer ) {}
	virtual BOOL MyTouch( CBasePlayer* pPlayer ) { return FALSE; }
	void DropItem( CBasePlayer* pPlayer, int nForceRespawn );
	void ThrowItem( CBasePlayer* pPlayer );
	void ScatterItem( CBasePlayer* pPlayer );

	int					team_no;
	int					m_iLastTouched;
	float				m_flNextTouchTime;
	float				m_flPickupTime;
	unsigned int		m_iItemFlag;
	const char*			m_pszItemName;
	static CBaseEntity*	m_pLastSpawn;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

void	RespawnPlayerCTFPowerups( CBasePlayer* pPlayer, int nForceRespawn );
void	ScatterPlayerCTFPowerups( CBasePlayer* pPlayer );
void	DropPlayerCTFPowerup( CBasePlayer* pPlayer );
void	InitItemsForPlayer( CBasePlayer* pPlayer );
void	FlushCTFPowerupTimes();
void	RefreshCustomHUD( CBasePlayer* pPlayer );
