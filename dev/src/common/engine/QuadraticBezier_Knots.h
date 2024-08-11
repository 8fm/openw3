
#pragma once

namespace QuadraticBezierNamespace
{
	class float3
	{
	public:
		inline float3(){x=y=z=0.f;}
		inline float3(Float a, Float b, Float c){x=a;y=b;z=c;}
		inline float3(const float3 & f){x=f.x;y=f.y;z=f.z;}
		inline float3 & operator= (const float3 & f){if(&f!=this){x=f.x;y=f.y;z=f.z;}return *this;}
		Float x;
		Float y;
		Float z;
		inline float3 & operator+= (const float3 & a){x+=a.x;y+=a.y;z+=a.z;return *this;}
		inline float3 & operator-= (const float3 & a){x-=a.x;y-=a.y;z-=a.z;return *this;}
		inline float3 & operator*= (Float n){x*=n;y*=n;z*=n;return *this;}
		inline float3 & operator/= (Float n){x/=n;y/=n;z*=n;return *this;}
	};
	inline float3 operator+ (const float3 & a,const float3 & b){return float3(a.x+b.x,a.y+b.y,a.z+b.z);}
	inline float3 operator- (const float3 & a,const float3 & b){return float3(a.x-b.x,a.y-b.y,a.z-b.z);}
	inline Float  operator* (const float3 & a,const float3 & b){return ((a.x*b.x)+(a.y*b.y)+(a.z*b.z));}
	inline float3 operator* (const float3 & a, Float n){return float3(a.x*n,a.y*n,a.z*n);}
	inline float3 operator* (Float n, const float3 & a){return float3(a.x*n,a.y*n,a.z*n);}
	inline float3 operator/ (const float3 & a, Float n){return float3(a.x/n,a.y/n,a.z/n);}
	inline Float  VectorLength(const float3 & a){return (float)sqrt( pow(a.x, 2)+pow(a.y, 2)+pow(a.z, 2) );}
	inline float3 Normalize(const float3 & a){Float len=VectorLength(a);return float3(a.x/len, a.y/len, a.z/len);}

	class float2
	{
	public:
		inline float2(){x=y=0.f;}
		inline float2(Float a, Float b){x=a;y=b;}
		inline float2(const float2 & f){x=f.x;y=f.y;}
		inline float2 & operator= (const float2 & f){if(&f!=this){x=f.x;y=f.y;}return *this;}
		Float x;
		Float y;
		inline float2 & operator+= (const float2 & a){x+=a.x;y+=a.y;return *this;}
		inline float2 & operator-= (const float2 & a){x-=a.x;y-=a.y;return *this;}
		inline float2 & operator*= (Float n){x*=n;y*=n;return *this;}
		inline float2 & operator/= (Float n){x/=n;y/=n;return *this;}
		inline operator float3(){return float3(x,y,0.0f);}
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