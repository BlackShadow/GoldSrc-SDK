#ifndef MAPMARKER_H
#define MAPMARKER_H

class CMapMarker : public CBaseEntity
{
public:
	void SetIcon( int iIcon ) { pev->body = iIcon; }
	int GetIcon( void ) { return pev->body; }
	void Spawn() override;
	void EXPORT Die();
	static CMapMarker* CreateMapMarker( Vector origin, Vector angles, CBaseEntity* owner, int icon );
};

#endif
