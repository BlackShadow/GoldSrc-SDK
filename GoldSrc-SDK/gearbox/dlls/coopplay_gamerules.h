#ifndef COOPPLAY_GAMERULES_H
#define COOPPLAY_GAMERULES_H

class CHalfLifeCoopplay : public CHalfLifeMultiplay
{
public:
	CHalfLifeCoopplay();

	BOOL IsCoOp() override;
	void UpdateGameMode( CBasePlayer* pPlayer ) override;
	void DeathNotice( CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor ) override;
	void MonsterKilled( CBaseMonster* pVictim, entvars_t* pKiller, entvars_t* pInflictor ) override;
	BOOL IsTeamplay() override;
	int PlayerRelationship( CBaseEntity* pPlayer, CBaseEntity* pTarget ) override;
	BOOL ShouldAutoAim( CBasePlayer* pPlayer, edict_t* target ) override;
	int IPointsForKill( CBasePlayer* pAttacker, CBasePlayer* pKilled ) override;
	int IPointsForMonsterKill( CBasePlayer* pAttacker, CBaseMonster* pKilled ) override;
	float FlPlayerFallDamage( CBasePlayer* pPlayer ) override;
	BOOL FPlayerCanRespawn( CBasePlayer* pPlayer ) override;
	float FlPlayerSpawnTime( CBasePlayer* pPlayer ) override;
	float FlHealthChargerRechargeTime() override;
	int DeadPlayerWeapons( CBasePlayer* pPlayer ) override;
	BOOL FAllowMonsters() override;
	void PlayerKilled( CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor ) override;
	void Think() override;
	int WeaponShouldRespawn( CBasePlayerItem* pWeapon ) override;
	int ItemShouldRespawn( CItem* pItem ) override;
	float FlItemRespawnTime( CItem* pItem ) override;
	int AmmoShouldRespawn( CBasePlayerAmmo* pAmmo ) override;
	float FlAmmoRespawnTime( CBasePlayerAmmo* pAmmo ) override;
	BOOL FPlayerCanTakeDamage( CBasePlayer* pPlayer, CBaseEntity* pAttacker ) override;
	BOOL ClientCommand( CBasePlayer* pPlayer, const char* pcmd ) override;
	float FlWeaponTryRespawn( CBasePlayerItem* pWeapon ) override;
	float FlWeaponRespawnTime( CBasePlayerItem* pWeapon ) override;
	const char* GetGameDescription() override;

	BOOL	m_DisableDeathMessages;
	BOOL	m_DisableDeathPenalty;
};

#endif
