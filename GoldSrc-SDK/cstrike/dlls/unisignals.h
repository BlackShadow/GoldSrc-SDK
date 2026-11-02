#ifndef UNISIGNALS_H
#define UNISIGNALS_H

#define SIGNAL_BUY (1 << 0)
#define SIGNAL_BOMB (1 << 1)
#define SIGNAL_RESCUE (1 << 2)
#define SIGNAL_ESCAPE (1 << 3)
#define SIGNAL_VIPSAFETY (1 << 4)

class CUnifiedSignals
{
public:
	CUnifiedSignals() : m_flSignal( 0 ), m_flState( 0 ) {}
	int Update()
	{
		int changed = m_flSignal ^ m_flState;
		m_flState = m_flSignal;
		m_flSignal = 0;
		return changed;
	}
	void Signal( int signal ) { m_flSignal |= signal; }
	int GetState() { return m_flState; }

private:
	int m_flSignal;
	int m_flState;
};

#endif
