#ifndef UNISIGNALS_H
#define UNISIGNALS_H

class CUnifiedSignals
{
public:
	CUnifiedSignals() : m_flSignal( 0 ), m_flState( 0 ) {}
	int Update( void )
	{
		int old = m_flState;
		m_flState = m_flSignal;
		m_flSignal = 0;
		return m_flState ^ old;
	}
	void Signal( int signal ) { m_flSignal |= signal; }
	int GetState( void ) { return m_flState; }

private:
	int m_flSignal;
	int m_flState;
};

#endif
