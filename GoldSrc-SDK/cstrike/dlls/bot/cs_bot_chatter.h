#ifndef CS_BOT_CHATTER_H
#define CS_BOT_CHATTER_H

#include "bot_util.h"
#include "nav.h"
#undef min
#undef max
#include <list>
#include <vector>

typedef unsigned int PlaceCriteria;
typedef unsigned int CountCriteria;

struct BotSpeakable
{
	BotSpeakable( void );
	~BotSpeakable();

	char *m_file;
	float m_duration;
	PlaceCriteria m_place;
	CountCriteria m_count;
};
typedef std::vector<BotSpeakable*> BotVoiceBank;
typedef std::vector<BotVoiceBank*> BotVoiceBankVector;

class BotPhrase
{
public:
	const char* GetSpeakable( int voiceBank, float* duration ) const;
	void ClearCriteria() const { m_placeCriteria = ANY_PLACE; m_countCriteria = 0xFFFF; }
	void SetPlaceCriteria( PlaceCriteria place ) const { m_placeCriteria = place; }
	void SetCountCriteria( CountCriteria count ) const { m_countCriteria = count; }
	const char* GetName() const { return m_name; }
	unsigned int GetID() const { return m_id; }
	GameEventType GetRadioEquivalent() const { return m_radioEvent; }
	bool IsImportant() const { return m_isImportant; }
	bool IsPlace() const { return m_isPlace; }
	void Randomize();

private:
	friend class BotPhraseManager;
	BotPhrase( unsigned int id, bool isPlace );
	~BotPhrase();
	void InitVoiceBank( int voiceBank );

	char* m_name;
	Place m_id;
	bool m_isPlace;
	GameEventType m_radioEvent;
	bool m_isImportant;
	BotVoiceBankVector m_voiceBank;
	std::vector<int> m_count;
	mutable std::vector<int> m_index;
	int m_numVoiceBanks;
	mutable PlaceCriteria m_placeCriteria;
	mutable CountCriteria m_countCriteria;
};

typedef std::list<BotPhrase*> BotPhraseList;

class BotPhraseManager
{
public:
	BotPhraseManager();
	~BotPhraseManager();
	bool Initialize( const char* filename, int voiceBank );
	void OnRoundRestart();
	void OnMapChange();
	unsigned int NameToID( const char* name ) const;
	const char* IDToName( unsigned int id ) const;
	const BotPhrase* GetPhrase( const char* name ) const;
	const BotPhrase* GetPlace( const char* name ) const;
	const BotPhrase* GetPlace( unsigned int id ) const;
	const BotPhraseList* GetPlaceList() const { return &m_placeList; }
	float GetPlaceStatementInterval( Place place ) const;
	void ResetPlaceStatementInterval( Place place );

private:
	struct PlaceTimeInfo
	{
		Place placeID;
		IntervalTimer timer;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	};
	int FindPlaceIndex( Place place ) const;
	BotPhraseList m_list;
	BotPhraseList m_placeList;
	mutable PlaceTimeInfo m_placeStatementHistory[64];
	mutable int m_placeCount;
};

extern BotPhraseManager* TheBotPhrases;

class CCSBot;
class BotStatement;
class BotMeme;

enum BotStatementType
{
	BOT_STATEMENT_ENEMY_SPOTTED,
	BOT_STATEMENT_KILLED_ENEMY,
	BOT_STATEMENT_ENEMIES_REMAINING,
	BOT_STATEMENT_SAY,
	BOT_STATEMENT_CRITICAL,
	BOT_STATEMENT_HELP,
	BOT_STATEMENT_REQUEST,
	BOT_STATEMENT_PERSISTENT,
	BOT_STATEMENT_PLAN,
	BOT_STATEMENT_INFORMATION,
	BOT_STATEMENT_EMOTE,
	BOT_STATEMENT_RESPONSE,
	BOT_STATEMENT_REMAINING_COUNT,
	BOT_STATEMENT_FRIENDLY_FIRE,
	BOT_STATEMENT_KILLED_FRIEND
};

class BotChatterInterface
{
public:
	BotChatterInterface( CCSBot *bot );
	~BotChatterInterface();
	void Reset( void );
	void Update( void );
	void OnEvent( GameEventType event, CBaseEntity *entity, CBaseEntity *other );
	void OnDeath( void );
	void AddStatement( BotStatement *statement, bool mustSpeak = false );
	void RemoveStatement( BotStatement *statement );
	BotStatement *GetActiveStatement( void );
	bool ShouldSpeak( void ) const;
	float GetRadioSilenceDuration( void );
	void ResetRadioSilenceDuration( void );
	void EnemySpotted( void );
	void KilledMyEnemy( int enemyID );
	void EnemiesRemaining( void );
	void ReportEnemies( void );
	void Clear( Place place );
	void ReportIn( void );
	void ReportingIn( void );
	bool NeedBackup( void );
	void PinnedDown( void );
	void Scared( void );
	void HeardNoise( const Vector *position );
	void Affirmative( void );
	void Negative( void );
	void AnnouncePlan( const char *phrase, Place place );
	void GoingToPlantTheBomb( Place place );
	void PlantingTheBomb( Place place );
	void TheyPickedUpTheBomb( void );
	void SpottedBomber( CBasePlayer *bomber );
	void SpottedLooseBomb( CBaseEntity *bomb );
	void GuardingLooseBomb( CBaseEntity *bomb );
	void RequestBombLocation( void );
	void FoundPlantedBomb( int bombsite );
	void BombsiteClear( int bombsite );
	void HostagesBeingTaken( void );
	void HostagesTaken( void );
	void EscortingHostages( void );
	void TalkingToHostages( void );
	void HostageDown( void );
	void GuardingHostages( Place place, bool isPlan );
	void GuardingHostageEscapeZone( bool isPlan );
	void FriendlyFire( void );
	void KilledFriend( void );
	void CelebrateWin( void );
	void Say( const char *phrase, float lifetime, float delay );
	void Encourage( const char *phrase, float interval, float lifetime );
	CCSBot *GetOwner( void ) const { return m_bot; }

private:
	friend class BotStatement;
	BotStatement *m_statementList;
	CCSBot *m_bot;
	bool m_hasSeenEnemy;
	float m_firstSeenEnemyTimestamp;
	bool m_hasReportedEnemies;
	bool m_hasRequestedBombLocation;
	int m_pitch;
	IntervalTimer m_needBackupTimer;
	IntervalTimer m_statementTimer;
	IntervalTimer m_scaredTimer;
	IntervalTimer m_guardTimer;
	CountdownTimer m_looseBombTimer;
	CountdownTimer m_noiseTimer;
	CountdownTimer m_escortingTimer;
	static IntervalTimer m_radioSilenceInterval[2];
	static CountdownTimer m_encourageTimer;
};

class BotStatement
{
public:
	enum ContextType
	{
		CURRENT_ENEMY_COUNT,
		REMAINING_ENEMY_COUNT,
		SHORT_DELAY,
		LONG_DELAY,
		ACCUMULATE_ENEMIES
	};
	enum ConditionType
	{
		IS_ATTACKING,
		RADIO_SILENCE,
		ENEMIES_REMAINING
	};

	BotStatement( BotChatterInterface *owner, BotStatementType type, float lifetime );
	~BotStatement();
	void AppendPhrase( const BotPhrase *phrase );
	void AppendPhrase( ContextType context );
	void AddCondition( ConditionType condition );
	void AttachMeme( BotMeme *meme );
	void Convert( const BotStatement *statement );
	bool Update( void );
	bool IsObsolete( void ) const;
	bool IsImportant( void ) const;
	bool IsRedundant( const BotStatement *statement ) const;
	bool IsValid( void ) const;
	bool HasCount( void ) const;
	CCSBot *GetOwner( void ) const;
	Place GetPlace( void ) const;
	void SetPlace( Place place ) { m_place = place; }
	void SetStartTime( float time ) { m_startTime = time; }
	void SetSubject( int subjectID ) { m_subjectID = subjectID; }

private:
	friend class BotChatterInterface;
	struct PhrasePart
	{
		bool isPhrase;
		union
		{
			const BotPhrase *phrase;
			ContextType context;
		};
	};
	BotChatterInterface *m_owner;
	BotStatement *m_next;
	BotStatement *m_prev;
	BotStatementType m_type;
	int m_subjectID;
	Place m_place;
	BotMeme *m_meme;
	float m_creationTimestamp;
	float m_startTime;
	float m_expireTime;
	float m_speakingStartTime;
	bool m_isSpeaking;
	float m_nextPhraseTime;
	PhrasePart m_phrase[4];
	ConditionType m_condition[4];
	int m_conditionCount;
	int m_currentPhrase;
	int m_phraseCount;
};

class BotMeme
{
public:
	virtual void Interpret( CCSBot *sender, CCSBot *receiver ) const = 0;
	void Transmit( CCSBot *sender ) const;
};

class BotHelpMeme : public BotMeme
{
public:
	explicit BotHelpMeme( Place place ) : m_place( place ) {}
	void Interpret( CCSBot *sender, CCSBot *receiver ) const override;
private:
	Place m_place;
};

class BotFollowMeme : public BotMeme
{
public:
	void Interpret( CCSBot *sender, CCSBot *receiver ) const override;
};

class BotWhereBombMeme : public BotMeme
{
public:
	void Interpret( CCSBot *sender, CCSBot *receiver ) const override;
};

class BotBombStatusMeme : public BotMeme
{
public:
	BotBombStatusMeme( int state, const Vector *position ) : m_state( state ), m_position( *position ) {}
	void Interpret( CCSBot *sender, CCSBot *receiver ) const override;
private:
	int m_state;
	Vector m_position;
};

class BotDefendHereMeme : public BotMeme
{
public:
	explicit BotDefendHereMeme( const Vector *position ) : m_position( *position ) {}
	void Interpret( CCSBot *sender, CCSBot *receiver ) const override;
private:
	Vector m_position;
};

class BotRequestReportMeme : public BotMeme
{
public:
	void Interpret( CCSBot *sender, CCSBot *receiver ) const override;
};

class BotBombsiteStatusMeme : public BotMeme
{
public:
	BotBombsiteStatusMeme( int bombsite, int state ) : m_bombsite( bombsite ), m_state( state ) {}
	void Interpret( CCSBot *sender, CCSBot *receiver ) const override;
private:
	int m_bombsite;
	int m_state;
};

class BotAllHostagesGoneMeme : public BotMeme
{
public:
	void Interpret( CCSBot *sender, CCSBot *receiver ) const override;
};

class BotHostageBeingTakenMeme : public BotMeme
{
public:
	void Interpret( CCSBot *sender, CCSBot *receiver ) const override;
};

const Vector *GetRandomSpotAtPlace( Place place );

#endif
