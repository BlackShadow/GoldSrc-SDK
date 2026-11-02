#if !defined( _WIN32 )

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <x86intrin.h>

typedef long long int64;
typedef unsigned long long uint64;

#define MICROSECONDS_PER_SECOND				1000000
#define CPU_FREQUENCY_SAMPLE_MICROSECONDS	5000
#define MAX_CPU_FREQUENCY_ATTEMPTS			600
#define FREQUENCY_TOLERANCE_HZ				((int64)40000)
#define MIN_CPU_FREQUENCY_HZ				((uint64)10000000)

typedef struct TimeVal
{
	timeval	m_TimeVal;
} TimeVal;

int64 GetCPUFreqFromPROC()
{
	FILE* file;
	char line[1024];
	double mhz;

	file = fopen( "/proc/cpuinfo", "r" );
	if ( file == NULL )
	{
		return 0;
	}

	mhz = 0.0;
	while ( fgets( line, sizeof( line ), file ) != NULL )
	{
		char* scan;

		if ( strstr( line, "cpu MHz" ) == NULL )
		{
			continue;
		}

		scan = line;
		while ( *scan && *scan != ':' )
		{
			scan++;
		}

		if ( *scan != 0 && sscanf( scan + 1, "%lf", &mhz ) == 1 )
		{
			break;
		}
	}

	fclose( file );
	return (int64)( mhz * (double)MICROSECONDS_PER_SECOND );
}

int64 CalculateCPUFreq()
{
	int64 period1;
	int64 period3;
	int nRemaining;

	period1 = 2 * FREQUENCY_TOLERANCE_HZ;
	period3 = 0;
	nRemaining = MAX_CPU_FREQUENCY_ATTEMPTS;

	for ( ;; )
	{
		TimeVal start;
		TimeVal end;
		uint64 startTick;
		int64 usecSpan;
		int64 freq;
		uint64 average;

		gettimeofday( &start.m_TimeVal, NULL );
		startTick = __rdtsc();
		usleep( CPU_FREQUENCY_SAMPLE_MICROSECONDS );
		gettimeofday( &end.m_TimeVal, NULL );

		usecSpan = (int64)MICROSECONDS_PER_SECOND * (int64)start.m_TimeVal.tv_sec
			- (int64)end.m_TimeVal.tv_usec
			+ (int64)start.m_TimeVal.tv_usec
			- (int64)MICROSECONDS_PER_SECOND * (int64)end.m_TimeVal.tv_sec;
		freq = (int64)( (double)( __rdtsc() - startTick ) / ( (double)usecSpan / (double)MICROSECONDS_PER_SECOND ) );

		if ( llabs( period1 - period3 ) <= FREQUENCY_TOLERANCE_HZ
			&& llabs( period3 - freq ) <= FREQUENCY_TOLERANCE_HZ
			&& llabs( period1 - freq ) <= FREQUENCY_TOLERANCE_HZ )
		{
			average = ( (uint64)period1 + (uint64)period3 + (uint64)freq ) / 3;
			if ( average < MIN_CPU_FREQUENCY_HZ )
			{
				return GetCPUFreqFromPROC();
			}

			return (int64)average;
		}

		if ( nRemaining-- == 1 )
		{
			return GetCPUFreqFromPROC();
		}

		period1 = period3;
		period3 = freq;
	}
}

#endif
