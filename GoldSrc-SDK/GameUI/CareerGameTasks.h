#pragma once

#include "GameUI/ICareerUI.h"

class CCareerUITask : public ICareerTask
{
public:
	CCareerUITask( const char* taskName );

	bool IsComplete() override;
	bool IsCompletedThisRound() override;
	bool IsCompletedThisMatch() override;
	const char* GetMap() override;
	const char* GetTaskName() override;
	const wchar_t* GetLocalizedTaskName() override;
	const wchar_t* GetLocalizedTaskNameWithCompletion() override;
	void Reset() override;
	void Set( bool thisRound ) override;
	void StartRound() override;
	const char* GetWeaponName() const override;
	int GetRepeat() const override;
	bool MustSurvive() const override;
	bool InARow() const override;
	int GetPartial() const override;
	void SetPartial( int partial ) override;
	bool IsMaxRoundTime() const override;

private:
	bool		m_isComplete;
	bool		m_completedThisRound;
	char		m_name[128];
	char		m_weapon[128];
	wchar_t		m_localizedName[512];
	wchar_t		m_localizedNameCompleted[512];
	int			m_repeat;
	bool		m_survive;
	bool		m_inarow;
	int			m_partials;
	bool		m_isWinFast;
};

void	ConstructStringWithReordering( wchar_t* output, int bufferSizeInBytes,
		wchar_t* formatString, int numFormatParameters, ... );
