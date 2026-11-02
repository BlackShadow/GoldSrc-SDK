#include "../external/steamworks/steamclientpublic.h"

#include <cstdio>
#include <cstring>

bool SteamIDFromSteam2String( const char* pchSteam2ID, EUniverse eUniverse, CSteamID* pSteamIDOut )
{
	unsigned short unSteam2Instance = 0;
	unsigned int unSteam2Parity = 0;
	unsigned int unSteam2Account = 0;
	char cExtraCharCheck = 0;
	int cFieldConverted;

	if ( !_strnicmp( pchSteam2ID, "STEAM_", 6 ) )
		pchSteam2ID += 6;

	cFieldConverted = sscanf( pchSteam2ID, "%hu:%u:%u%c", &unSteam2Instance, &unSteam2Parity, &unSteam2Account, &cExtraCharCheck );

	if ( cExtraCharCheck || cFieldConverted == EOF || cFieldConverted < 2 )
		return false;
	if ( cFieldConverted < 3 && unSteam2Instance != 1 )
		return false;

	pSteamIDOut->Set( unSteam2Parity + 2u * unSteam2Account, eUniverse, k_EAccountTypeIndividual );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : pchSteamID -		text representation of a Steam ID
//-----------------------------------------------------------------------------
CSteamID::CSteamID( const char* pchSteamID, EUniverse eDefaultUniverse )
{
	SetFromString( pchSteamID, eDefaultUniverse );
}

void CSteamID::SetFromString( const char* pchSteamID, EUniverse eDefaultUniverse )
{
	unsigned int nAccountID = 0;
	unsigned int nInstance = 1;
	unsigned int nUniverse = eDefaultUniverse;
	unsigned int nAccountType = k_EAccountTypeIndividual;
	bool bHasPrefix = true;

	if ( *pchSteamID == '[' )
		pchSteamID++;

	if ( *pchSteamID == 'A' )
	{
		pchSteamID++; // skip the A
		if ( *pchSteamID == '-' || *pchSteamID == ':' )
			pchSteamID++; // skip the optional - or :

		const char* pchParen = strchr( pchSteamID, '(' );
		if ( pchParen )
#ifdef _MSC_VER
#pragma warning( suppress: 6031 )
#endif
			sscanf( pchParen, "(%u)", &nInstance );

		const char* pchColon = strchr( pchSteamID, ':' );
		if ( pchColon && strchr( pchColon + 1, ':' ) )
#ifdef _MSC_VER
#pragma warning( suppress: 6031 )
#endif
			sscanf( pchSteamID, "%u:%u:%u", &nUniverse, &nAccountID, &nInstance );
		else if ( pchColon )
#ifdef _MSC_VER
#pragma warning( suppress: 6031 )
#endif
			sscanf( pchSteamID, "%u:%u", &nUniverse, &nAccountID );
		else
#ifdef _MSC_VER
#pragma warning( suppress: 6031 )
#endif
			sscanf( pchSteamID, "%u", &nAccountID );

		m_steamid.m_comp.m_unAccountID = nAccountID;
		m_steamid.m_comp.m_EUniverse = (EUniverse)nUniverse;
		m_steamid.m_comp.m_EAccountType = k_EAccountTypeAnonGameServer;
		m_steamid.m_comp.m_unAccountInstance = nAccountID ? nInstance : 0;
		return;
	}

	switch ( *pchSteamID )
	{
	case 'G':
		nAccountType = k_EAccountTypeGameServer;
		break;
	case 'C':
		nAccountType = k_EAccountTypeContentServer;
		break;
	case 'g':
		nInstance = 0;
		nAccountType = k_EAccountTypeClan;
		break;
	case 'c':
		nInstance = k_EChatInstanceFlagClan;
		nAccountType = k_EAccountTypeChat;
		break;
	case 'L':
		nInstance = k_EChatInstanceFlagLobby;
		nAccountType = k_EAccountTypeChat;
		break;
	case 'T':
		nInstance = 0;	// Anon chat
		nAccountType = k_EAccountTypeChat;
		break;
	case 'U':
		nAccountType = k_EAccountTypeIndividual;
		break;
	case 'i':
		nAccountType = k_EAccountTypeInvalid;
		break;
	default:
		bHasPrefix = false;
		break;
	}

	if ( bHasPrefix )
	{
		pchSteamID++;
		if ( *pchSteamID == '-' || *pchSteamID == ':' )
			pchSteamID++; // skip the optional - or :
	}

	if ( strchr( pchSteamID, ':' ) )
	{
		if ( *pchSteamID == '[' )
			pchSteamID++; // skip the optional [
#ifdef _MSC_VER
#pragma warning( suppress: 6031 )
#endif
		sscanf( pchSteamID, "%u:%u", &nUniverse, &nAccountID );
		if ( nUniverse == k_EUniverseInvalid )
			nUniverse = eDefaultUniverse;
	}
	else
	{
#ifdef _MSC_VER
#pragma warning( suppress: 6031 )
#endif
		sscanf( pchSteamID, "%u", &nAccountID );
	}

	m_steamid.m_comp.m_unAccountID = nAccountID;
	m_steamid.m_comp.m_unAccountInstance = nInstance;
	m_steamid.m_comp.m_EAccountType = nAccountType;
	m_steamid.m_comp.m_EUniverse = (EUniverse)nUniverse;
}

const char* CSteamID::Render() const
{
	static char szSteamID[4][37];
	static int nBuf = 0;
	char* pchBuf = szSteamID[nBuf]; // get pointer to current static buf
	int cubBuf = sizeof( szSteamID[0] );

	nBuf = ( nBuf + 1 ) % 4;

	unsigned int nUniverse = m_steamid.m_comp.m_EUniverse;
	unsigned int nAccountID = m_steamid.m_comp.m_unAccountID;
	unsigned int nInstance = m_steamid.m_comp.m_unAccountInstance;

	switch ( m_steamid.m_comp.m_EAccountType )
	{
	case k_EAccountTypeAnonGameServer:
		snprintf( pchBuf, cubBuf, "[A:%u:%u:%u]", nUniverse, nAccountID, nInstance );
		break;
	case k_EAccountTypeGameServer:
		snprintf( pchBuf, cubBuf, "[G:%u:%u]", nUniverse, nAccountID );
		break;
	case k_EAccountTypeMultiseat:
		snprintf( pchBuf, cubBuf, "[M:%u:%u:%u]", nUniverse, nAccountID, nInstance );
		break;
	case k_EAccountTypePending:
		snprintf( pchBuf, cubBuf, "[P:%u:%u]", nUniverse, nAccountID );
		break;
	case k_EAccountTypeContentServer:
		snprintf( pchBuf, cubBuf, "[C:%u:%u]", nUniverse, nAccountID );
		break;
	case k_EAccountTypeClan:
		snprintf( pchBuf, cubBuf, "[g:%u:%u]", nUniverse, nAccountID );
		break;
	case k_EAccountTypeChat:
		if ( nInstance & k_EChatInstanceFlagClan )
			snprintf( pchBuf, cubBuf, "[c:%u:%u]", nUniverse, nAccountID );
		else if ( nInstance & k_EChatInstanceFlagLobby )
			snprintf( pchBuf, cubBuf, "[L:%u:%u]", nUniverse, nAccountID );
		else // Anon chat
			snprintf( pchBuf, cubBuf, "[T:%u:%u]", nUniverse, nAccountID );
		break;
	case k_EAccountTypeInvalid:
		snprintf( pchBuf, cubBuf, "[I:%u:%u]", nUniverse, nAccountID );
		break;
	case k_EAccountTypeIndividual:
		snprintf( pchBuf, cubBuf, "[U:%u:%u]", nUniverse, nAccountID );
		break;
	case k_EAccountTypeAnonUser:
		snprintf( pchBuf, cubBuf, "[a:%u:%u]", nUniverse, nAccountID );
		break;
	default:
		snprintf( pchBuf, cubBuf, "[i:%u:%u]", nUniverse, nAccountID );
		break;
	}

	return pchBuf;
}

//-----------------------------------------------------------------------------
// Purpose: Renders the passed-in steam ID to a buffer.  NOTE: for convenience of calling
//			code, this code returns a pointer to a static buffer and is NOT thread-safe.
// Input:	64-bit representation of Steam ID to render
// Output:  buffer with rendered Steam ID
//-----------------------------------------------------------------------------
const char* CSteamID::Render( uint64 ulSteamID )
{
	CSteamID steamID( ulSteamID );
	return steamID.Render();
}

bool CSteamID::BValidExternalSteamID() const
{
	unsigned int nAccountType = m_steamid.m_comp.m_EAccountType;

	if ( nAccountType == k_EAccountTypePending )
		return false;
	if ( nAccountType == k_EAccountTypeIndividual || nAccountType == k_EAccountTypeGameServer
		 || nAccountType == k_EAccountTypeAnonUser )
		return true;

	return m_steamid.m_comp.m_unAccountID != 0 || m_steamid.m_comp.m_unAccountInstance != 0;
}
