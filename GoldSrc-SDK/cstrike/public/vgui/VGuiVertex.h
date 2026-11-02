#pragma once

namespace vgui2
{
class VGuiVertex
{
public:
	VGuiVertex();
	VGuiVertex( int xpos, int ypos, float s, float t )
	{
		SetVertex( xpos, ypos, s, t );
	}

	void SetVertex( int xpos, int ypos );
	void SetVertex( int xpos, int ypos, float s, float t )
	{
		x = xpos;
		y = ypos;
		u = s;
		v = t;
	}

	int GetX();
	int GetY();
	float GetU();
	float GetV();
	void GetVertex( int& xpos, int& ypos, float& s, float& t ) const;
	bool operator==( VGuiVertex& other ) const;

private:
	int		x;
	int		y;
	float	u;
	float	v;
};
}
