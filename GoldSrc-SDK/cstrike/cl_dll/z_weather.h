#ifndef Z_WEATHER_H
#define Z_WEATHER_H

struct model_s;

class CEnvironment
{
private:
	int m_iWeatherType;
	bool m_bGrassActive;
	Vector m_vWeatherOrigin;
	float m_flWeatherTime;
	Vector m_vGrassOrigin;
	float m_flGrassTime;
	model_s* m_pSnowSprite;
	model_s* m_pRainSprite;
	model_s* m_pRipple;
	model_s* m_pRainSplash;
	model_s* m_pGrassSprite1;
	model_s* m_pGrassSprite2;
	Vector m_vWind;
	Vector m_vDesiredWindDirection;
	float m_flDesiredWindSpeed;
	float m_flNextWindChangeTime;
	float m_flOldTime;
	float m_flIdealYaw;
	float m_flWeatherValue;

public:
	void Initialize( void );
	void SetWeatherType( int type ) { m_iWeatherType = type; }
	int GetWeatherType( void ) { return m_iWeatherType; }
	void SetSnowSprite( model_s* sprite ) { m_pSnowSprite = sprite; }
	void SetRainSprite( model_s* sprite ) { m_pRainSprite = sprite; }
	void SetRippleSprite( model_s* sprite ) { m_pRipple = sprite; }
	void SetSplashSprite( model_s* sprite ) { m_pRainSplash = sprite; }
	float GetOldTime( void ) { return m_flOldTime; }
	void SetupGrass( void );
	void UpdateGrass( void );
	void SetGrassSprites( model_s* first, model_s* second )
	{
		m_pGrassSprite1 = first;
		m_pGrassSprite2 = second;
	}
	model_s* GetSnowSprite( void ) { return m_pSnowSprite; }
	model_s* GetRainSprite( void ) { return m_pRainSprite; }
	model_s* GetRippleSprite( void ) { return m_pRipple; }
	model_s* GetSplashSprite( void ) { return m_pRainSplash; }
	model_s* GetGrassSprites( void )
	{
		return gEngfuncs.pfnRandomLong( 0, 1 ) ? m_pGrassSprite1 : m_pGrassSprite2;
	}
	void SetWeatherTime( float time ) { m_flWeatherTime = time; }
	float GetWeatherTime( void ) { return m_flWeatherTime; }
	void SetWeatherOrigin( Vector origin ) { m_vWeatherOrigin = origin; }
	Vector GetWeatherOrigin( void ) { return m_vWeatherOrigin; }
	void SetGrassOrigin( Vector origin ) { m_vGrassOrigin = origin; }
	Vector GetGrassOrigin( void ) { return m_vGrassOrigin; }
	void Update( void );
	void UpdateSnow( void );
	void UpdateRain( void );
	void CreateRaindrop( float* origin );
	void CreateSnowFlake( float* origin );
	void CreateGrassPiece( float* origin, Vector normal );
	void CreateWindParticle( float* origin );
	void SetWindSpeed( Vector velocity ) { m_vWind = velocity; }
	Vector GetWindSpeed( void ) { return m_vWind; }
	void UpdateWind( void );
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

extern CEnvironment g_Environment;

#endif
