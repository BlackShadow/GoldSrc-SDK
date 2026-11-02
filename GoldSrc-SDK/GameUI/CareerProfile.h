#pragma once

#include "GameUI/ICareerUI.h"
#include <string>
#include <vector>

#define CAREER_PROFILE_VERSION		4
#define MAX_CAREER_TUTOR_MESSAGES	256
#define MAX_CAREER_PROFILE_DATA		0x4000

struct SavedMap
{
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 26495 )
#endif
	SavedMap() = default;
#ifdef _MSC_VER
#pragma warning( pop )
#endif

	std::string	name;
	bool		defeated;
};

typedef std::vector<std::string> StringVector;
typedef std::vector<SavedMap> SavedMapVector;

class CCareerProfileDifficultyData
{
public:
	CCareerProfileDifficultyData() { Reset(); }
	void Reset();

	bool			played;
	int				points;
	StringVector	hired;
	SavedMapVector	maps;
	std::string		lastMap;
	int				pointsFlashed;
};

class CCareerProfileData
{
public:
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 26495 )
#endif
	CCareerProfileData() = default;
#ifdef _MSC_VER
#pragma warning( pop )
#endif

	void Reset();

	int				filetime;
	std::string		name;
	int				sliderPos;
	CareerDifficultyType	lastDifficulty;
	bool				medals[MAX_CAREER_DIFFICULTY];
	CCareerProfileDifficultyData difficulty[MAX_CAREER_DIFFICULTY];
	int				tutorData[MAX_CAREER_TUTOR_MESSAGES];
};

typedef std::vector<CCareerProfileData*> ProfileVector;
