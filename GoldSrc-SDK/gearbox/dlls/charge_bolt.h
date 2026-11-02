#pragma once

class COFChargedBolt : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_NONE; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	static COFChargedBolt* ChargedBoltCreate();
	void LaunchChargedBolt( Vector& aim, edict_t* owner, int speed, int acceleration );
	void SetAttachment( CBaseAnimating* entity, int attachment );
	void InitBeams();
	void ClearBeams();
	void ArmBeam( int side );

	int					m_iShowerSparks;
	CBeam*				m_pBeam[8];
	int					m_iBeams;
	CBaseAnimating*	m_pAttachEnt;
	int					m_iAttachIdx;

private:
	void EXPORT ShutdownChargedBolt();
	void EXPORT ChargedBoltTouch( CBaseEntity* other );
	void EXPORT FlyThink();
	void EXPORT AttachThink();
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
