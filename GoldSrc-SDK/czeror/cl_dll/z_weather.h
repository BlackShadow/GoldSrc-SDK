#ifndef Z_WEATHER_H
#define Z_WEATHER_H

struct model_s;

class CEnvironment
{
private:
	int m_iWeatherType;
	bool m_bGrassActive;
	bool m_bPlaceInitialGrass;
	Vector m_vWeatherOrigin;
	float m_flWeatherTime;
	Vector m_vGrassOrigin;
	float m_flGrassTime;
	Vector m_cameraForward;
	model_s *m_pSnowSprite;
	model_s *m_pRainSprite;
	model_s *m_pRipple;
	model_s *m_pRainSplash;
	model_s *m_pGrassSprite1;
	model_s *m_pGrassSprite2;
	Vector m_vWind;
	float m_flNextWindChangeTime;
	Vector m_vDesiredWindDirection;
	float m_flDesiredWindSpeed;
	float m_flIdealYaw;
	float m_flOldTime;
	float m_fWindDirectionMinX;
	float m_fWindDirectionMaxX;
	float m_fWindDirectionMinY;
	float m_fWindDirectionMaxY;
	float m_fWindChangeTimeMin;
	float m_fWindChangeTimeMax;
	char m_szGrassSpriteName[256];
	float m_fGrassFadeDist;
	float m_fGrassMinHeight;
	float m_fGrassMaxHeight;
	int m_iGrassDensity;
	float m_flWeatherValue;

public:
	int m_grassCount;

public:
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
	CEnvironment()
	{
	}
	void Initialize();
	void SetWeatherType(int type) { m_iWeatherType = type; }
	int GetWeatherType() { return m_iWeatherType; }
	void SetSnowSprite(model_s *sprite) { m_pSnowSprite = sprite; }
	void SetRainSprite(model_s *sprite) { m_pRainSprite = sprite; }
	void SetRippleSprite(model_s *sprite) { m_pRipple = sprite; }
	void SetSplashSprite(model_s *sprite) { m_pRainSplash = sprite; }
	void SetupGrass();
	void UpdateGrass();
	void PlaceGrass(float innerRadius, float outerRadius, int quantity, const Vector &oldOrigin);
	void PlaceGrassSquare(float radius, int quantity);
	float GetGrassFadeDist() { return m_fGrassFadeDist; }
	void SetGrassSprites(model_s *first, model_s *second)
	{
		m_pGrassSprite1 = first;
		m_pGrassSprite2 = second;
	}
	model_s *GetSnowSprite() { return m_pSnowSprite; }
	model_s *GetRainSprite() { return m_pRainSprite; }
	model_s *GetRippleSprite() { return m_pRipple; }
	model_s *GetSplashSprite() { return m_pRainSplash; }
	model_s *GetGrassSprites();
	void SetWeatherTime(float time) { m_flWeatherTime = time; }
	float GetWeatherTime() { return m_flWeatherTime; }
	void SetWeatherOrigin(Vector origin) { m_vWeatherOrigin = origin; }
	Vector GetWeatherOrigin() { return m_vWeatherOrigin; }
	void SetGrassOrigin(Vector origin) { m_vGrassOrigin = origin; }
	Vector GetGrassOrigin() { return m_vGrassOrigin; }
	void Update();
	void UpdateSnow();
	void UpdateRain();
	void CreateRaindrop(float *origin);
	void CreateSnowFlake(float *origin);
	void CreateGrassPiece(float *origin, Vector normal, float height);
	void CreateWindParticle(float *origin);
	void SetWindSpeed(Vector speed) { m_vWind = speed; }
	Vector GetWindSpeed() { return m_vWind; }
	void UpdateWind();
	float GetOldTime() { return m_flOldTime; }
	Vector &GetCameraForward() { return m_cameraForward; }
	void SetWindDirectionConstraints(float minX, float minY, float maxX, float maxY);
	void SetWindChangeTimeConstraints(float minimum, float maximum);
	void SetGrassConstraints(const char *sprite, float fadeDistance, float minHeight, float maxHeight, int density);
};

extern CEnvironment g_Environment;

#endif
