/*
#pragma once

typedef float Float;

namespace QuadraticBezierNamespace
{
	class float2
	{
	public:
		inline float2(){x=y=0.f;}
		inline float2(Float a, Float b){x=a;y=b;}
		inline float2(const float2 & f){x=f.x;y=f.y;}
		inline float2 & operator= (const float2 & f){if(&f!=this){x=f.x;y=f.y;}return *this;}
		Float x;
		Float y;
		inline float2 operator- (){return float2(-x,-y);}
		inline float2 & operator+= (const float2 & a){x+=a.x;y+=a.y;return *this;}
		inline float2 & operator-= (const float2 & a){x-=a.x;y-=a.y;return *this;}
		inline float2 & operator*= (Float n){x*=n;y*=n;return *this;}
		inline float2 & operator/= (Float n){x/=n;y/=n;return *this;}
	};

	inline float2 operator+ (const float2 & a,const float2 & b){return float2(a.x+b.x,a.y+b.y);}
	inline float2 operator- (const float2 & a,const float2 & b){return float2(a.x-b.x,a.y-b.y);}
	inline Float  operator* (const float2 & a,const float2 & b){return ((a.x*b.x)+(a.y*b.y));}
	inline float2 operator* (const float2 & a, Float n){return float2(a.x*n,a.y*n);}
	inline float2 operator* (Float n, const float2 & a){return float2(a.x*n,a.y*n);}
	inline float2 operator/ (const float2 & a, Float n){return float2(a.x/n,a.y/n);}

	inline Float  Length(const float2 & a){return (float)sqrt( pow(a.x, 2)+pow(a.y, 2) );}
	inline float2 Normalize(const float2 & a){Float len=Length(a);return float2(a.x/len, a.y/len);}
	inline float2 Cross(const float2 & a){return float2(-a.y,a.x);}
}
*/