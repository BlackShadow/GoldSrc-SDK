#ifndef ICZEROVIEWPORT_H
#define ICZEROVIEWPORT_H

class ICzeroViewPort
{
public:
	virtual void ShowWorldMap() = 0;
	virtual void HideWorldMap() = 0;
	virtual bool IsWorldMapVisible() = 0;

	virtual void UnlockMission( const char* missionName ) = 0;
	virtual void LockMission( const char* missionName ) = 0;
	virtual void CurrentMission( const char* missionName ) = 0;
};

extern ICzeroViewPort* gCzeroViewPortInterface;

#endif
