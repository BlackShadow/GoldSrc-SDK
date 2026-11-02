#ifndef TUTOR_CS_TUTOR_H
#define TUTOR_CS_TUTOR_H

#include "tutor_base_tutor.h"
#include "player.h"

#include <map>
#include <string>
#include <vector>

enum TutorMessageID
{
	YOU_FIRED_A_SHOT,
	YOU_SHOULD_RELOAD,
	YOU_ARE_OUT_OF_AMMO,
	YOU_KILLED_A_TEAMMATE,
	YOU_KILLED_PLAYER,
	YOU_KILLED_PLAYER_ONE_LEFT,
	YOU_KILLED_LAST_ENEMY,
	YOU_KILLED_PLAYER_HEADSHOT,
	YOU_KILLED_PLAYER_HEADSHOT_ONE_LEFT,
	YOU_KILLED_LAST_ENEMY_HEADSHOT,
	YOU_DIED,
	YOU_DIED_HEADSHOT,
	YOU_FELL_TO_YOUR_DEATH,
	YOU_WERE_JUST_HURT,
	YOU_ARE_BLIND_FROM_FLASHBANG,
	YOU_ATTACKED_TEAMMATE,
	BUY_TIME_BEGIN,
	BOMB_PLANTED_T,
	BOMB_PLANTED_CT,
	TEAMMATE_KILLED,
	TEAMMATE_KILLED_ONE_LEFT,
	LAST_TEAMMATE_KILLED,
	ENEMY_KILLED,
	ENEMY_KILLED_ONE_LEFT,
	LAST_ENEMY_KILLED,
	YOU_SPAWNED,
	YOU_SEE_FRIEND,
	YOU_SEE_ENEMY,
	YOU_SEE_FRIEND_CORPSE,
	YOU_SEE_ENEMY_CORPSE,
	YOU_SEE_LOOSE_BOMB_T,
	YOU_SEE_LOOSE_BOMB_CT,
	YOU_SEE_BOMB_CARRIER_T,
	YOU_SEE_BOMB_CARRIER_CT,
	YOU_SEE_PLANTED_BOMB_T,
	YOU_SEE_PLANTED_BOMB_CT,
	YOU_ARE_BOMB_CARRIER,
	YOU_SEE_LOOSE_WEAPON,
	YOU_SEE_LOOSE_DEFUSER,
	YOU_SEE_BOMBSITE_T,
	YOU_SEE_BOMBSITE_CT,
	YOU_SEE_BOMBSITE_T_BOMB,
	YOU_SEE_HOSTAGE_T,
	YOU_SEE_HOSTAGE_CT,
	YOU_SEE_HOSTAGE_CT_EXAMINE,
	YOU_USED_HOSTAGE_MORE_LEFT,
	YOU_USED_HOSTAGE_NO_MORE_LEFT,
	ALL_HOSTAGES_FOLLOWING_T,
	ALL_HOSTAGES_FOLLOWING_CT,
	HOSTAGE_RESCUED_T,
	HOSTAGE_RESCUED_CT,
	YOU_RESCUED_HOSTAGE,
	YOU_ARE_IN_BOMBSITE_T,
	YOU_ARE_IN_BOMBSITE_CT,
	YOU_ARE_IN_BOMBSITE_T_BOMB,
	ALL_HOSTAGES_RESCUED_T,
	ALL_HOSTAGES_RESCUED_CT,
	YOU_DAMAGED_HOSTAGE,
	YOU_KILLED_HOSTAGE,
	ALL_HOSTAGES_DEAD,
	YOU_HAVE_BEEN_SHOT_AT,
	TIME_RUNNING_OUT_DE_T,
	TIME_RUNNING_OUT_DE_CT,
	TIME_RUNNING_OUT_CS_T,
	TIME_RUNNING_OUT_CS_CT,
	DEFUSING_WITHOUT_KIT,
	BOMB_DEFUSED_T,
	BOMB_DEFUSED_CT,
	YOU_DEFUSED_BOMB,
	BOMB_EXPLODED_T,
	BOMB_EXPLODED_CT,
	ROUND_START_DE_T,
	ROUND_START_DE_CT,
	ROUND_START_CS_T,
	ROUND_START_CS_CT,
	ROUND_OVER,
	ROUND_DRAW,
	CT_WIN,
	T_WIN,
	DEATH_CAMERA_START,
	RADIO_COVER_ME,
	RADIO_YOU_TAKE_THE_POINT,
	RADIO_HOLD_THIS_POSITION,
	RADIO_REGROUP_TEAM,
	RADIO_FOLLOW_ME,
	RADIO_TAKING_FIRE,
	RADIO_GO_GO_GO,
	RADIO_TEAM_FALL_BACK,
	RADIO_STICK_TOGETHER_TEAM,
	RADIO_GET_IN_POSITION_AND_WAIT,
	RADIO_STORM_THE_FRONT,
	RADIO_REPORT_IN_TEAM,
	RADIO_AFFIRMATIVE,
	RADIO_ENEMY_SPOTTED,
	RADIO_NEED_BACKUP,
	RADIO_SECTOR_CLEAR,
	RADIO_IN_POSITION,
	RADIO_REPORTING_IN,
	RADIO_GET_OUT_OF_THERE,
	RADIO_NEGATIVE,
	RADIO_ENEMY_DOWN,
	BUY_NEED_PRIMARY,
	BUY_NEED_PRIMARY_AMMO,
	BUY_NEED_SECONDARY_AMMO,
	BUY_NEED_ARMOR,
	BUY_NEED_DEFUSE_KIT,
	BUY_NEED_GRENADE,
	CAREER_TASK_DONE_MORE_LEFT,
	CAREER_TASK_DONE_ONE_LEFT,
	CAREER_TASK_DONE_ALL_DONE,
	HINT_BEGIN,
	HINT_1,
	HINT_2,
	HINT_3,
	HINT_4,
	HINT_5,
	HINT_10,
	HINT_11,
	HINT_12,
	HINT_13,
	HINT_14,
	HINT_15,
	HINT_20,
	HINT_21,
	HINT_22,
	HINT_23,
	HINT_24,
	HINT_25,
	HINT_26,
	HINT_30,
	HINT_31,
	HINT_32,
	HINT_33,
	HINT_34,
	HINT_40,
	HINT_50,
	HINT_51,
	HINT_52,
	HINT_53,
	HINT_BOMB_START,
	HINT_60 = HINT_BOMB_START,
	HINT_61,
	HINT_BOMB_END = HINT_61,
	HINT_HOSTAGE_START,
	HINT_70 = HINT_HOSTAGE_START,
	HINT_71,
	HINT_72,
	HINT_73,
	HINT_HOSTAGE_END = HINT_73,
	HINT_END,
	INGAME_HINT_BEGIN,
	INGAME_HINT_1,
	INGAME_HINT_2,
	INGAME_HINT_END,

	TUTOR_NUM_MESSAGES
};

typedef std::map<std::string, TutorMessage*> TutorMessageMap;

struct ClientCorpseStruct
{
	Vector	m_position;
	int		m_team;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

typedef std::vector<ClientCorpseStruct*> ClientCorpseList;

class CCSTutor : public CBaseTutor
{
public:
	CCSTutor();
	~CCSTutor() override;
	void TutorThink( float time ) override;
	void PurgeMessages() override;

protected:
	void CreateAndAddEventToList( TutorMessageID id, CBaseEntity* entity, CBaseEntity* other );
	TutorMessageEvent* CreateTutorMessageEvent( TutorMessageID id, CBaseEntity* entity, CBaseEntity* other );
	void AddToEventList( TutorMessageEvent* event );
	void DeleteEventFromEventList( TutorMessageEvent* event );
	void ClearEventList();
	void ClearCurrentEvent( bool closeWindow, bool processDeaths );
	void DeleteEvent( TutorMessageEvent* event );
	void ShowTutorMessage( TutorMessageEvent* event ) override;
	bool ShouldShowMessageEvent( TutorMessageEvent* event, float time );
	bool ShouldUpdateCurrentMessage( TutorMessageID id );
	void ComputeDisplayTimesForMessage();
	void UpdateCurrentMessage( TutorMessageEvent* event );
	void ConstructMessageAndDisplay();
	void LookupHotKey( TutorMessageID id, int index, wchar_t* buffer, int buflen );
	void CheckForWindowClose( float time );
	void CheckForContentUpdate();
	bool HasCurrentWindowBeenActiveLongEnough( float time );
	void CheckForInterruptingMessageEvent( float time );
	void CheckForInactiveEvents( float time );
	void CancelEvent( TutorMessageID id );
	void ProcessShownDeathsForEvent( TutorMessageEvent* event );
	void TransferDeathEvents( TutorMessageEvent* oldEvent, TutorMessageEvent* newEvent );
	TutorMessageEvent* GetTutorMessageUpdateEvent();
	bool GetDuplicateMessagesFromEventList( TutorMessageEvent*& event1, TutorMessageEvent*& event2 );
	void CallEventHandler( GameEventType event, CBaseEntity* entity, CBaseEntity* other ) override;
	bool IsBombMap();
	bool IsHostageMap();
	void HandleWeaponFired( CBaseEntity* entity, CBaseEntity* other );
	void HandleWeaponFiredOnEmpty( CBaseEntity* entity, CBaseEntity* other );
	void HandleWeaponReloaded( CBaseEntity* entity, CBaseEntity* other );
	void HandlePlayerDied( CBaseEntity* entity, CBaseEntity* other );
	void HandlePlayerSpawned( CBaseEntity* entity, CBaseEntity* other );
	void HandleClientCorpseSpawned( CBaseEntity* entity, CBaseEntity* other );
	void HandlePlayerTookDamage( CBaseEntity* entity, CBaseEntity* other );
	void HandlePlayerBlindedByFlashbang( CBaseEntity* entity, CBaseEntity* other );
	void HandleBuyTimeStart( CBaseEntity* entity, CBaseEntity* other );
	void HandlePlayerLeftBuyZone( CBaseEntity* entity, CBaseEntity* other );
	void HandleBombPlanted( CBaseEntity* entity, CBaseEntity* other );
	void HandleRoundStart( CBaseEntity* entity, CBaseEntity* other );
	void HandleBombDefused( CBaseEntity* entity, CBaseEntity* other );
	void HandleBombExploded( CBaseEntity* entity, CBaseEntity* other );
	void HandleHostageUsed( CBaseEntity* entity, CBaseEntity* other );
	void HandleHostageRescued( CBaseEntity* entity, CBaseEntity* other );
	void HandleHostageDamaged( CBaseEntity* entity, CBaseEntity* other );
	void HandleHostageKilled( CBaseEntity* entity, CBaseEntity* other );
	void HandleAllHostagesRescued( CBaseEntity* entity, CBaseEntity* other );
	void HandleBeingShotAt( CBaseEntity* entity, CBaseEntity* other );
	void HandleRoundDraw( CBaseEntity* entity, CBaseEntity* other );
	void HandleCTWin( CBaseEntity* entity, CBaseEntity* other );
	void HandleTWin( CBaseEntity* entity, CBaseEntity* other );
	void HandleDeathCameraStart( CBaseEntity* entity, CBaseEntity* other );
	void HandleBombDefusing( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioCoverMe( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioYouTakeThePoint( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioHoldThisPosition( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioRegroupTeam( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioFollowMe( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioTakingFire( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioGoGoGo( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioTeamFallBack( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioStickTogetherTeam( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioGetInPositionAndWait( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioStormTheFront( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioReportInTeam( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioAffirmative( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioEnemySpotted( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioNeedBackup( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioSectorClear( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioInPosition( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioReportingIn( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioGetOutOfThere( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioNegative( CBaseEntity* entity, CBaseEntity* other );
	void HandleRadioEnemyDown( CBaseEntity* entity, CBaseEntity* other );
	void HandleBuyMenuOpenned( CBaseEntity* entity, CBaseEntity* other );
	void HandleAutoBuy( CBaseEntity* entity, CBaseEntity* other );
	void HandleNotBuyingAnything( CBaseEntity* entity, CBaseEntity* other );
	void HandleNeedToBuyPrimaryWeapon( CBaseEntity* entity, CBaseEntity* other );
	void HandleNeedToBuyPrimaryAmmo( CBaseEntity* entity, CBaseEntity* other );
	void HandleNeedToBuySecondaryAmmo( CBaseEntity* entity, CBaseEntity* other );
	void HandleNeedToBuyArmor( CBaseEntity* entity, CBaseEntity* other );
	void HandleNeedToBuyDefuseKit( CBaseEntity* entity, CBaseEntity* other );
	void HandleNeedToBuyGrenade( CBaseEntity* entity, CBaseEntity* other );
	void HandleCareerTaskDone( CBaseEntity* entity, CBaseEntity* other );
	void HandleShotFired( Vector source, Vector target ) override;
	void GetNumPlayersAliveOnTeams( int& terrorists, int& cts );
	void CheckForBombViewable();
	void CheckForLooseWeaponViewable();
	void CheckForLooseDefuserViewable();
	void CheckForBombsiteViewable();
	TutorMessageID CheckForInBombZone();
	void CheckForHostageViewable();
	void CheckForTimeRunningOut();
	void CheckForAllHostagesDead();
	bool CheckForAllHostagesFollowingSomeone();
	void CheckHintMessages( float time );
	void CheckInGameHintMessages( float time );
	void CheckExamineMessages( float time );
	void CheckForNeedToReload( bool fired );
	bool CanLocalPlayerBuyStuff();
	void CheckBuyZoneMessages();
	bool IsBombPlantedInBombsite( CBaseEntity* bombsite );
	void ReadTutorMessageFile();
	void ApplyPersistentDecay();
	TutorMessage* GetTutorMessageDefinition( int id ) override;
	CBaseEntity* GetEntityForMessageID( int id, CBaseEntity* other );
	void ResetPlayerDeathInfo();
	void ConstructRecentDeathsList( TeamName team, char* buffer, int buflen, TutorMessageEvent* event );

	float m_nextViewableCheckTime;
	TutorMessageMap m_messageMap;
	TutorMessageID m_currentlyShownMessageID;
	float m_currentlyShownMessageCloseTime;
	float m_currentlyShownMessageStartTime;
	float m_currentlyShownMessageMinimumCloseTime;
	TutorMessageEvent* m_currentMessageEvent;
	TutorMessageEvent* m_lastScenarioEvent;
	TutorMessageID m_lastHintShown;
	TutorMessageID m_lastInGameHintShown;
	ClientCorpseList m_clientCorpseList;
	int m_messageTypeMask;
	bool m_haveSpawned;

	struct PlayerDeathStruct
	{
		bool m_hasBeenShown;
		TutorMessageEvent* m_event;
	};

	PlayerDeathStruct m_playerDeathInfo[32];
};

#endif
