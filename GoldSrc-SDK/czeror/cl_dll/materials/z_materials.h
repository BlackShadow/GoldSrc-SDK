#ifndef Z_MATERIALS_H
#define Z_MATERIALS_H

#include "../ev_hldm.h"

struct pmtrace_s;

class CMaterial
{
public:
	int				m_iIndex;
	char			m_szName[64];
	CMaterial*		pNext;
	materials_t*	pMaterial;

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
	CMaterial() : m_iIndex( 0 ), pNext( NULL ), pMaterial( NULL )
	{
	}
};

class CMaterialFIO
{
public:
	void WriteToErrorFile( char* textureName, char* error, ... );
};

class CMaterialSystem
{
public:
	int			m_iElements;
	int			m_iNewMaterials;
	CMaterial*	pFirst;
	CMaterial*	pCurrent;

	CMaterialSystem() : m_iElements( 0 ), m_iNewMaterials( 0 ), pFirst( NULL ), pCurrent( NULL )
	{
	}
	bool AddMaterial( char* textureName );
	void ClearMaterials();
	bool CheckMaterial( char* name );
	materials_t* GetMaterialByIndex( int index );
	int GetMaterialIndex( char* name );
	void CopyMaterials( CMaterialSystem* materials );
	void FixTextureName( const char* input, char* output );
	int GetMaterialIndexAtPos( pmtrace_s* trace, float* start, float* end );
	bool ShouldLoad( char* textureName );
	bool IsValidMaterialName( char* textureName );
	bool LoadMaterial( char* textureName );
	~CMaterialSystem();

	CMaterialFIO m_MatFIO;
};

extern CMaterialSystem	g_pMaterialSystem;
extern CMaterialSystem*	g_pMaterialSystemPointer;

void	MAT_FlushAndReload();
void	LoadLevelMaterials();
void	LoadMaterialFromNameList();
void	CreateMaterialFile( char* textureName );
char*	MAT_FindMaterialFromTxt( char* textureName );
int		MAT_GetMaterialIndexByType( char* materialType );
int		MAT_GetRenderMode( char* renderMode );
bool	MAT_SetDefaults( int materialType, materials_t* material );

#endif
