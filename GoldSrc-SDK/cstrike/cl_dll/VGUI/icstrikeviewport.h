#ifndef ICSTRIKEVIEWPORT_H
#define ICSTRIKEVIEWPORT_H

class ICSViewPort
{
public:
	virtual int GetForceCamera() = 0;
	virtual void UpdateBuyPresets() = 0;
	virtual void UpdateScheme() = 0;
	virtual bool IsProgressBarVisible() = 0;
	virtual void StartProgressBar( const char* title, int ticks, int progress, bool timeBased ) = 0;
	virtual void UpdateProgressBar( const char* status, int progress ) = 0;
	virtual void StopProgressBar() = 0;
};

extern ICSViewPort* gCSViewPortInterface;

#endif
