#include "tier0/vprof.h"
#include "tier0/validator.h"

#if defined( _WIN32 )
#include <intrin.h>
#include <mbstring.h>
#include <string>
#include <string.h>

#define GDPERF_WRITE_MSR		0x9c406400
#define GDPERF_READ_MSR		0x9c406404
#define PME_EVENT_ACTIVE		0x100
#define P4_COUNTER_ENABLE	0x1000
#define P4_ESCR_EVENT_MASK	0xfffffffffe0001ffULL
#define P4_ESCR_OS_USER		0x0c
#define P4_BSQ_COUNTER_MSR	0x300
#define P4_BSQ_CCCR_MSR		0x360
#define P4_BSQ_ESCR_MSR		0x3a0

enum PMEError_t
{
	PME_INVALID_COUNTER	= -2,
	PME_UNSUPPORTED_CPU	= -3,
	PME_NO_DRIVER		= -6,
	PME_DRIVER_ERROR	= -8,
	PME_INVALID_EVENT	= -10
};

class CPUInfo
{
public:
	CPUInfo();

	std::string	m_Vendor;
	int			m_Manufacturer;
	std::string	m_Brand;
	unsigned int m_Signature;
	unsigned int m_BrandIndex;
	unsigned int m_Features;
	unsigned char* m_pCacheDescriptors;
};

class PME : public CPUInfo
{
public:
	PME();
	static PME* Instance();
	int ReadMSR( unsigned int index, uint64* pValue );
	int WriteMSR( unsigned int index, const uint64* pValue );
	int SelectEvent( unsigned int event, unsigned int counter, unsigned char user, unsigned char kernel );
	void Validate( CValidator& validator, char* pName );

	HANDLE		m_hDriver;
	bool		m_bDriverOpen;
	uint64		m_Counter;
};

struct PMEEvent_t
{
	int				m_nEvent;
	unsigned short	m_Flags;
	const char*		m_pName;
	PME*			m_pPME;
	uint64			m_ESCR;
	uint64			m_CCCR;
	unsigned int	m_CounterMSR;
	unsigned int	m_CCCRMSR;
	unsigned int	m_ESCRMSR;
	unsigned short* m_pFlags;
};

CPUInfo::CPUInfo()
	: m_Manufacturer( 2 ), m_Signature( 0 ), m_BrandIndex( 0 ), m_Features( 0 ), m_pCacheDescriptors( NULL )
{
	int registers[4];
	__cpuid( registers, 0 );
	int maxLeaf = registers[0];
	char vendor[13];
	memcpy( vendor, &registers[1], sizeof( int ) );
	memcpy( vendor + 4, &registers[3], sizeof( int ) );
	memcpy( vendor + 8, &registers[2], sizeof( int ) );
	vendor[12] = '\0';
	m_Vendor = vendor;
	if ( maxLeaf >= 1 )
	{
		__cpuid( registers, 1 );
		m_Signature = registers[0];
		m_BrandIndex = registers[1];
		m_Features = registers[3];
	}

	if ( maxLeaf >= 2 )
	{
		__cpuid( registers, 2 );
		int repeats = registers[0] & 0xff;
		bool descriptors[256] = {};
		for ( int i = 0; i < repeats; ++i )
		{
			__cpuid( registers, 2 );
			if ( i == 0 )
				registers[0] &= ~0xff;
			for ( int r = 0; r < 4; ++r )
			{
				if ( registers[r] >= 0 )
				{
					for ( int shift = 0; shift < 32; shift += 8 )
						descriptors[(unsigned int)registers[r] >> shift & 0xff] = true;
				}
			}
		}
		int count = 0;
		for ( int i = 0; i < 256; ++i )
			count += descriptors[i];
		m_pCacheDescriptors = new unsigned char[count];
		int next = 0;
		for ( int i = 1; i < 256; ++i )
		{
			if ( descriptors[i] )
				m_pCacheDescriptors[next++] = (unsigned char)i;
		}
		m_pCacheDescriptors[next] = 0;
	}

	__cpuid( registers, (int)0x80000000 );
	if ( (unsigned int)registers[0] >= 0x80000002 )
	{
		char brand[49] = {};
		unsigned int maxExtendedLeaf = registers[0];
		for ( unsigned int leaf = 0x80000002; leaf <= 0x80000004 && leaf <= maxExtendedLeaf; ++leaf )
		{
			__cpuid( registers, leaf );
			memcpy( brand + ( leaf - 0x80000002 ) * sizeof( registers ), registers, sizeof( registers ) );
		}
		m_Brand = brand;
	}

	if ( !_mbscmp( (const unsigned char*)m_Vendor.c_str(), (const unsigned char*)"GenuineIntel" ) )
		m_Manufacturer = 0;
	else if ( !_mbscmp( (const unsigned char*)m_Vendor.c_str(), (const unsigned char*)"AuthenticAMD" ) )
		m_Manufacturer = 1;
	else
		m_Manufacturer = 2;
}

PME::PME()
{
	m_hDriver = NULL;
	m_bDriverOpen = false;
	m_Counter = 0;
	if ( m_Manufacturer < 2 )
	{
		static OSVERSIONINFOA version;
		version.dwOSVersionInfoSize = sizeof( version );
#pragma warning( suppress: 4996 28159 )
		GetVersionExA( &version );
		m_hDriver = CreateFileA( version.dwPlatformId == VER_PLATFORM_WIN32_NT ? "\\\\.\\GDPERF" : "\\\\.\\GDPERF.VXD",
			GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( m_hDriver != INVALID_HANDLE_VALUE )
		{
			m_bDriverOpen = true;
			if ( m_Manufacturer == 0 && ( m_Signature & 0xf00 ) == 0x600 )
			{
				SelectEvent( 0x60790300, 0, 1, 1 );
				SelectEvent( 0x60790300, 1, 1, 1 );
			}
		}
	}
}

PME* PME::Instance()
{
	static PME* pPME;
	if ( !pPME )
		pPME = new PME;
	return pPME;
}

int PME::ReadMSR( unsigned int index, uint64* pValue )
{
	if ( !m_bDriverOpen )
		return PME_NO_DRIVER;
	DWORD bytesReturned;
	BOOL success = DeviceIoControl( m_hDriver, GDPERF_READ_MSR, &index, sizeof( index ), pValue,
		sizeof( *pValue ), &bytesReturned, NULL );
	if ( !success && bytesReturned != sizeof( *pValue ) )
		return PME_DRIVER_ERROR;
	return success;
}

int PME::WriteMSR( unsigned int index, const uint64* pValue )
{
	if ( !m_bDriverOpen )
		return PME_NO_DRIVER;
	unsigned int request[] = { index, (unsigned int)*pValue, (unsigned int)( *pValue >> 32 ) };
	DWORD bytesReturned;
	BOOL success = DeviceIoControl( m_hDriver, GDPERF_WRITE_MSR, request, sizeof( request ), NULL,
		0, &bytesReturned, NULL );
	if ( !success && bytesReturned != 0 )
		return PME_DRIVER_ERROR;
	return success;
}

int PME::SelectEvent( unsigned int event, unsigned int counter, unsigned char user, unsigned char kernel )
{
	if ( counter > 1 )
		return PME_INVALID_COUNTER;
	if ( !m_bDriverOpen )
		return PME_NO_DRIVER;
	unsigned int family = m_Signature >> 8 & 0xf;
	if ( event >> 28 != family || !( ( counter + 1 ) & ( event >> 8 ) & 3 ) )
		return PME_INVALID_EVENT;

	if ( family == 6 )
	{
		uint64 control = ( kernel << 17 ) | ( user << 16 ) | ( event >> 16 & 0xff ) | ( ( ( event & 0xff ) | 0x4400 ) << 8 );
		return WriteMSR( 0x186 + ( counter != 0 ), &control );
	}
	if ( family == 5 )
	{
		uint64 control = 0;
		ReadMSR( 0x11, &control );
		control = (unsigned int)control & ( counter == 0 ? 0xffff0000 : 0xffff );
		WriteMSR( 0x11, &control );
		uint64 zero = 0;
		WriteMSR( 0x12 + ( counter != 0 ), &zero );
		control |= ( kernel << 6 ) | ( user << 7 );
		unsigned int eventSelect = event >> 16 & 0x3f;
		control |= eventSelect == 0x3f ? 0x100 : eventSelect;
		WriteMSR( 0x11, &control );
		return 0;
	}
	return family == 15 ? 0 : PME_UNSUPPORTED_CPU;
}

void PME::Validate( CValidator& validator, char* pName )
{
	validator.Push( "PME", this, pName );
	validator.Pop();
}

CL2Cache::CL2Cache()
{
	static int nextID;
	m_nID = nextID++;
	PMEEvent_t* pEvent = new PMEEvent_t;
	pEvent->m_pPME = PME::Instance();
	pEvent->m_pFlags = &pEvent->m_Flags;
	pEvent->m_Flags = 0;
	pEvent->m_ESCR = 0x18000000;
	pEvent->m_CCCR = 0x3e000;
	pEvent->m_pName = "BSQ_cache_reference";
	pEvent->m_nEvent = 0;
	pEvent->m_CounterMSR = P4_BSQ_COUNTER_MSR;
	pEvent->m_CCCRMSR = P4_BSQ_CCCR_MSR;
	pEvent->m_ESCRMSR = P4_BSQ_ESCR_MSR;
	m_pL2CacheEvent = pEvent;
	m_i64Start = 0;
	m_i64End = 0;
	m_iL2CacheMissCount = 0;
}

CL2Cache::~CL2Cache()
{
	if ( m_pL2CacheEvent )
	{
		delete (PMEEvent_t*)m_pL2CacheEvent;
		m_pL2CacheEvent = NULL;
	}
}

void CL2Cache::Start()
{
	PMEEvent_t* pEvent = (PMEEvent_t*)m_pL2CacheEvent;
	if ( !pEvent )
		return;

	*pEvent->m_pFlags |= PME_EVENT_ACTIVE;
	pEvent->m_ESCR = ( pEvent->m_ESCR & P4_ESCR_EVENT_MASK ) | ( (uint64)*pEvent->m_pFlags << 9 ) | P4_ESCR_OS_USER;
	pEvent->m_pPME->WriteMSR( pEvent->m_ESCRMSR, &pEvent->m_ESCR );
	pEvent->m_CCCR &= ~P4_COUNTER_ENABLE;
	pEvent->m_pPME->WriteMSR( pEvent->m_CCCRMSR, &pEvent->m_CCCR );
	uint64 counter = 0;
	pEvent->m_pPME->WriteMSR( pEvent->m_CounterMSR, &counter );
	pEvent->m_CCCR |= P4_COUNTER_ENABLE;
	pEvent->m_pPME->WriteMSR( pEvent->m_CCCRMSR, &pEvent->m_CCCR );
	counter = 0;
	pEvent->m_pPME->ReadMSR( pEvent->m_CounterMSR, &counter );
	m_i64Start = counter;
}

void CL2Cache::End()
{
	PMEEvent_t* pEvent = (PMEEvent_t*)m_pL2CacheEvent;
	if ( !pEvent )
		return;

	uint64 counter = 0;
	pEvent->m_pPME->ReadMSR( pEvent->m_CounterMSR, &counter );
	m_i64End = counter;
	pEvent->m_CCCR &= ~P4_COUNTER_ENABLE;
	pEvent->m_pPME->WriteMSR( pEvent->m_CCCRMSR, &pEvent->m_CCCR );
	m_iL2CacheMissCount = (int)( m_i64End - m_i64Start );
}

void CL2Cache::Validate( CValidator& validator, char* pName )
{
	validator.Push( "CL2Cache", this, pName );
	validator.Pop();
}

void InitPME()
{
	PME* pPME = PME::Instance();
	if ( !pPME )
	{
		_DMsg( "console", 1, "PME Uninitialized.\n" );
		g_VProfCurrentProfile.PMEInitialized( false );
	}
	else if ( pPME->m_Manufacturer == 0 && ( pPME->m_Signature & 0xf00 ) == 0xf00 )
	{
		SetPriorityClass( GetCurrentProcess(), REALTIME_PRIORITY_CLASS );
		SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );
		_DMsg( "console", 1, "PME Initialized.\n" );
		g_VProfCurrentProfile.PMEInitialized( true );
	}
}

void ShutdownPME()
{
	if ( PME::Instance() )
	{
		SetPriorityClass( GetCurrentProcess(), NORMAL_PRIORITY_CLASS );
		SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
	}
	g_VProfCurrentProfile.PMEInitialized( false );
}

void ValidatePME( CValidator& validator )
{
	PME::Instance()->Validate( validator, "PME" );
}
#endif
