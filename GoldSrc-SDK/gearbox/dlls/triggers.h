#pragma once

#include "cbase.h"

class CBaseTrigger : public CBaseToggle
{
public:
	void EXPORT TeleportTouch( CBaseEntity* other );
	void KeyValue( KeyValueData* data ) override;
	void EXPORT MultiTouch( CBaseEntity* other );
	void EXPORT HurtTouch( CBaseEntity* other );
	void EXPORT CDAudioTouch( CBaseEntity* other );
	void ActivateMultiTrigger( CBaseEntity* activator );
	void EXPORT MultiWaitOver();
	void EXPORT CounterUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void EXPORT ToggleUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void InitTrigger();

	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};
