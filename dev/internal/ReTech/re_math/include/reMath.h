#pragma once
#include <math.h>
#include <memory.h>
#include <float.h>

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//
//
//	TODO: evaluate operators and ctor's
//
//
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#define DEGTORAD	0.0174533f
#define RADTODEG	57.2958f
#define PAT_PI		3.1415926535897932384626433f

inline float arctan2(float a, float b)
{
	return atan2(a,b);
}

inline float absVal(float a)
{
	return fabs(a);
}

struct float4
{
	float4( float _x = 0.f, float _y = 0.f, float _z = 0.f, float _w = 0.f )
	{
		x=_x; y=_y; z=_z; w=_w;
	}

	float x;
	float y;
	float z;
	float w;
};
/////////////////////////////////////////////////////////////////////////

class float2
{
public:
	inline float2(float a=0.0f, float b=0.0f)
	{
		x=a;
		y=b;
	}
	inline float2(const float2 & f)
	{
		x=f.x;
		y=f.y;
	}

public:
	inline float2 & operator= (const float2 & f)
	{
		if(&f!=this)
		{
			x=f.x;
			y=f.y;
		}
		return *this;
	}
	inline float2 operator- ()
	{
		return float2(-x,-y);
	}
	inline void operator+= (const float2 & a)
	{
		x+=a.x;
		y+=a.y;
	}
	inline void operator-= (const float2 & a)
	{
		x-=a.x;
		y-=a.y;
	}
	inline void operator*= (float n)
	{
		x*=n;
		y*=n;
	}
	inline void operator/= (float n)
	{
		x*=n;
		y*=n;
	}

public:
	inline void normalize()
	{
		float len = length();
		x/=len;
		y/=len;
	}

	inline float length()
	{
		return (float)sqrt( x*x+y*y );
	}

	inline void clamp( const float2 & min, const float2 & max )
	{
		x = x<min.x ? min.x : x;
		y = y<min.y ? min.y : y;
		x = x>max.x ? max.x : x;
		y = y>max.y ? max.y : y;
	}

public:
	float x;
	float y;
};

inline float2 operator+ (const float2 & a,const float2 & b)
{
	return float2(a.x+b.x,a.y+b.y);
}
inline float2 operator- (const float2 & a,const float2 & b)
{
	return float2(a.x-b.x,a.y-b.y);
}
inline float operator* (const float2 & a,const float2 & b)
{
	return a.x*b.x+a.y*b.y;
}
inline float2 operator* (const float2 & a, float n)
{
	return float2(a.x*n,a.y*n);
}
inline float2 operator* (float n, const float2 & a)
{
	return float2(a.x*n,a.y*n);
}
inline float2 operator/ (const float2 & a, float n)
{
	return float2(a.x/n,a.y/n);
}
inline float length(const float2 & a)
{
	return (float)sqrt( a.x*a.x+a.y*a.y );
}
inline float2 normalize(const float2 & a)
{
	float len=length(a);
	return float2(a.x/len,a.y/len);
}
inline float2 cross(const float2 & a)
{
	return float2(-a.y,a.x);
}

////////////////////////////////////////////////////////////////////////////

class M22
{
public:
	inline M22( float p0, float p1, float p2, float p3 )
	{
		row1.x = p0;
		row1.y = p1;
		row2.x = p2;
		row2.y = p3;
	}
	inline M22(float2 a=float2(1.0f,0.0f), float2 b=float2(0.0f,1.0f))
	{
		row1 = a;
		row2 = b;
	}
	inline M22(const M22 & m)
	{
		row1=m.row1;
		row2=m.row2;
	}
	inline M22 & operator= (const M22 & m)
	{
		if(&m!=this)
		{
			row1=m.row1;
			row2=m.row2;
		}
		return *this;
	}
	inline void identity()
	{
		row1.x=1.0f;
		row1.y=0.0f;
		row2.x=0.0f;
		row2.y=1.0f;
	}
	float2 row1;
	float2 row2;
};

inline M22 transpose(const M22 & m)
{
	M22 wyn=m;
	wyn.row1.y=m.row2.x;
	wyn.row2.x=m.row1.y;
	return wyn;
}
inline float det(const M22 & m)
{
	return ( (m.row1.x*m.row2.y)-(m.row1.y*m.row2.x) );
}
inline M22 inverse(const M22 & m)
{
	float d=det(m);
	return M22( float2(m.row2.y, -m.row1.y)/d, float2(-m.row2.x, m.row1.x)/d );
}
inline M22 operator+ (const M22 & a, const M22 & b)
{
	return M22(a.row1+b.row1,a.row2+b.row2);
}
inline M22 operator- (const M22 & a, const M22 & b)
{
	return M22(a.row1-b.row1,a.row2-b.row2);
}

inline M22 operator* (const M22 & a, const M22 & b)
{
	return M22(		float2((a.row1.x*b.row1.x)+(a.row1.y*b.row2.x) , (a.row1.x*b.row1.y)+(a.row1.y*b.row2.y)) ,
		float2( (a.row2.x*b.row1.x)+(a.row2.y*b.row2.x) , (a.row2.x*b.row1.y)+(a.row2.y*b.row2.y) ) );
}
inline float2 operator* (const float2 & w, const M22 & m)
{
	return float2(  (w.x*m.row1.x)+(w.y*m.row2.x)  ,  (w.x*m.row1.y)+(w.y*m.row2.y) );
}
inline M22 operator* (const M22 & m, float n)
{
	return M22( m.row1*n, m.row2*n);
}
inline M22 operator* (float n, const M22 & m)
{
	return M22( m.row1*n, m.row2*n);
}
inline M22 operator/ (const M22 & m, float n)
{
	return M22( m.row1/n, m.row2/n);
}

////////////////////////////////////////////////////////////////////////////

class transform2
{
public:
	inline transform2(float2 p=float2(), M22 r=M22()){pos=p;rot=r;}
	transform2( float px, float py, float r0, float r1, float r2, float r3 )
	{
		pos.x = px;
		pos.y = py;
		rot.row1.x = r0;
		rot.row1.y = r1;
		rot.row2.x = r2;
		rot.row2.y = r3;
	}
	float2 pos;
	M22 rot;
};

inline float2 operator* (const float2 & w, const transform2 & m)
{
	return (w*m.rot)+m.pos;
}
inline transform2 operator* (const transform2 & a, const transform2 & b)
{
	return transform2( a.pos*b , M22( a.rot.row1*b.rot, a.rot.row2*b.rot ) );
}
inline transform2 inverse(const transform2 & t)
{
	M22 inv = inverse(t.rot);
	return transform2(-(t.pos*inv),inv);
}

class complex
{
public:
	inline complex(float a=1.0f,float b=0.0f)
	{
		x=a;
		y=b;
	}
	inline complex(const complex & f)
	{
		x=f.x;
		y=f.y;	
	}
	inline complex & operator= (const complex & f)
	{
		if(&f!=this)
		{
			x=f.x;
			y=f.y;
		}
		return *this;
	}
	inline void operator+= (const complex & a)
	{
		x+=a.x;
		y+=a.y;
	}
	inline void operator-= (const complex & a)
	{
		x-=a.x;
		y-=a.y;
	}
	inline void operator*= (float n)
	{
		x*=n;
		y*=n;
	}
	inline void operator/= (float n)
	{
		x/=n;
		y/=n;
	}
	float x;
	float y;
};

inline complex operator+ (const complex & a,const complex & b)
{
	return complex(a.x+b.x,a.y+b.y);
}
inline complex operator- (const complex & a,const complex & b)
{
	return complex(a.x-b.x,a.y-b.y);
}
inline complex operator* (const complex & a,const complex & b)
{
	return complex( a.x*b.x - a.y*b.y , a.x*b.y + a.y*b.x );
}
inline complex operator* (const complex & a, float n)
{
	return complex(a.x*n,a.y*n);
}
inline complex operator* (float n, const complex & a)
{
	return complex(a.x*n,a.y*n);
}
inline complex operator/ (const complex & a, float n)
{
	return complex(a.x/n,a.y/n);
}
inline float length(const complex & a)
{
	return (float)sqrt( a.x*a.x+a.y*a.y );
}
inline complex normalize(const complex & a)
{
	float len=length(a);
	return complex(a.x/len,a.y/len);
}

/// lh test complex operations
//////////////////////////////

inline complex operator+ (const complex & a, float b)
{
	return complex(a.x+b,a.y);
}
inline complex operator+ (float a, const complex & b)
{
	return complex(a+b.x,b.y);
}
inline complex operator- (const complex & a,float b)
{
	return complex(a.x-b,a.y);
}
inline complex operator- (float a, const complex & b)
{
	return complex(a-b.x,-b.y);
}
inline complex conjugate(const complex & a)
{
	return complex(a.x,-a.y);
}
inline float conjugate(float a)
{
	return a;
}
inline float absolute(const complex & a)
{
	return (float)sqrt( a.x*a.x+a.y*a.y );
}
inline float absolute(float a)
{
	return absVal(a);
}
inline float argument(const complex & a)
{
	return arctan2(a.y,a.x);
}
inline complex complexsqrt(const complex& c)
{
	complex com;
	float b = sqrt((sqrt(c.x*c.x+c.y*c.y)-c.x)/2.f);
	float a = c.y/(2*b);
	com.x = a;
	com.y = b;
	return com;
}

/////////////////////////////////////////////////////////////////////////

class float3
{
public:
	inline float3( float a )
		: x(a)
		, y(a)
		, z(a)
	{}

	inline float3(float a=0.0f, float b=0.0f, float c=0.0f)
	{
		x=a;
		y=b;
		z=c;
	}

	inline float3& operator = ( const float3& f )
	{
		if(&f!=this)
		{
			x=f.x;
			y=f.y;
			z=f.z;
		}
		return *this;
	}

	inline float3 operator- ()
	{
		return float3(-x,-y,-z);
	}
	inline void operator+= (const float3 & a)
	{
		x+=a.x;
		y+=a.y;
		z+=a.z;
	}
	inline void operator-= (const float3 & a)
	{
		x-=a.x;
		y-=a.y;
		z-=a.z;
	}
	inline void operator*= (float n)
	{
		x*=n;
		y*=n;
		z*=n;
	}
	inline void operator/= (float n)
	{
		x/=n;
		y/=n;
		z/=n;
	}

	inline float& operator[]( int a )
	{
		return v[a];
	}
	inline const float& operator[]( int a ) const
	{
		return v[a];
	}

	inline bool operator== ( const float3 & a )
	{
		if( a.x==x && a.y==y && a.z==z )
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	inline void normalize()
	{
		float len = length();
		x/=len;
		y/=len;
		z/=len;
	}
	inline float length()
	{
		return (float)sqrt(x*x+y*y+z*z);
	}
	inline void clamp( const float3 & min, const float3 & max )
	{
		x = x<min.x ? min.x : x;
		y = y<min.y ? min.y : y;
		z = z<min.z ? min.z : z;
		x = x>max.x ? max.x : x;
		y = y>max.y ? max.y : y;
		z = z>max.z ? max.z : z;
	}
	inline bool isOK() const
	{
		return !( _isnan(x) && _isnan(y) && _isnan(z) );
	}

public:
	union
	{
		struct 
		{
			float x;
			float y;
			float z;
		};
		struct 
		{
			float v[3];
		};
	};
};
inline float3 operator+ (const float3 & a,const float3 & b)
{
	return float3(a.x+b.x,a.y+b.y,a.z+b.z);
}
inline float3 operator- (const float3 & a,const float3 & b)
{
	return float3(a.x-b.x,a.y-b.y,a.z-b.z);
}
inline float  operator* (const float3 & a,const float3 & b)
{
	return a.x*b.x+a.y*b.y+a.z*b.z;
}
inline float3 operator* (const float3 & a, float n)
{
	return float3(a.x*n,a.y*n,a.z*n);
}
inline float3 operator* (float n, const float3 & a)
{
	return float3(a.x*n,a.y*n,a.z*n);
}
inline float3 operator/ (const float3 & a, float n)
{
	return float3(a.x/n,a.y/n,a.z/n);
}
inline float length(const float3 & a)
{
	return (float)sqrt( a.x*a.x+a.y*a.y+a.z*a.z );
}
inline float3 normalize(const float3 & a)
{
	float len=length(a);
	return float3(a.x/len,a.y/len,a.z/len);
}
inline float3 cross(const float3 & a,const float3 & b)
{
	return float3( (a.y*b.z)-(a.z*b.y), (a.z*b.x)-(a.x*b.z), (a.x*b.y)-(a.y*b.x)   );
}
inline float  triple(const float3 & a,const float3 & b, const float3 & c)
{
	return cross(a,b)*c;
}

////////////////////////////////////////////////////////////////

class M33
{
public:
	inline M33(float3 a=float3(1.0f,0.0f,0.0f), float3 b=float3(0.0f,1.0f,0.0f), float3 c=float3(0.0f,0.0f,1.0f))
	{
		row1 = a;
		row2 = b;
		row3 = c;
	}
	M33( float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8 )
	{
		row1.x = p0;
		row1.y = p1;
		row1.z = p2;

		row2.x = p3;
		row2.y = p4;
		row2.z = p5;

		row3.x = p6;
		row3.y = p7;
		row3.z = p8;
	}
	inline M33(const M33 & m)
	{
		row1=m.row1;
		row2=m.row2;
		row3=m.row3;		
	}
	inline M33 & operator= (const M33 & m)
	{
		if(&m!=this)
		{
			row1=m.row1;
			row2=m.row2;
			row3=m.row3;
		}
		return *this;	
	}

	inline void identity()
	{
		row1 = float3 (1.0f, 0.0f, 0.0f);
		row2 = float3 (0.0f, 1.0f, 0.0f);
		row3 = float3 (0.0f, 0.0f, 1.0f);
	}

	float3 row1;
	float3 row2;
	float3 row3;
};

inline M33 transpose(const M33 & m)
{
	return M33( float3(m.row1.x,m.row2.x,m.row3.x) , float3(m.row1.y,m.row2.y,m.row3.y) , float3(m.row1.z,m.row2.z,m.row3.z) );
}
inline float det(const M33 & m)
{
	return ( (m.row1.x*m.row2.y*m.row3.z)
		+(m.row1.y*m.row2.z*m.row3.x)
		+(m.row1.z*m.row2.x*m.row3.y)
		-(m.row1.z*m.row2.y*m.row3.x)
		-(m.row1.x*m.row2.z*m.row3.y)
		-(m.row1.y*m.row2.x*m.row3.z) );
}
inline M33 inverse(const M33 & m)
{
	float dd=det(m);
	float a=m.row1.x;
	float b=m.row1.y;
	float c=m.row1.z;
	float d=m.row2.x;
	float e=m.row2.y;
	float f=m.row2.z;
	float g=m.row3.x;
	float h=m.row3.y;
	float i=m.row3.z;
	return M33(	float3( ((e*i)-(h*f))/dd , (-((b*i)-(c*h)))/dd  , ((b*f)-(c*e))/dd ),
		float3( (-((d*i)-(f*g)))/dd , ((a*i)-(c*g))/dd  , (-((a*f)-(c*d)))/dd ),
		float3( ((d*h)-(e*g))/dd , (-((a*h)-(b*g)))/dd  , ((a*e)-(b*d))/dd ));
}
inline M33 operator+ (const M33 & a, const M33 & b)
{
	return M33( a.row1+b.row1 , a.row2+b.row2, a.row3+b.row3 );
}
inline M33 operator- (const M33 & a, const M33 & b)
{
	return M33( a.row1-b.row1 , a.row2-b.row2, a.row3-b.row3 );
}
inline M33 operator* (const M33 & a, const M33 & b)
{
	return M33(
		float3(		(a.row1.x*b.row1.x)+(a.row1.y*b.row2.x)+(a.row1.z*b.row3.x),
		(a.row1.x*b.row1.y)+(a.row1.y*b.row2.y)+(a.row1.z*b.row3.y),
		(a.row1.x*b.row1.z)+(a.row1.y*b.row2.z)+(a.row1.z*b.row3.z) ),
		float3(		(a.row2.x*b.row1.x)+(a.row2.y*b.row2.x)+(a.row2.z*b.row3.x),
		(a.row2.x*b.row1.y)+(a.row2.y*b.row2.y)+(a.row2.z*b.row3.y),
		(a.row2.x*b.row1.z)+(a.row2.y*b.row2.z)+(a.row2.z*b.row3.z) ),
		float3(		(a.row3.x*b.row1.x)+(a.row3.y*b.row2.x)+(a.row3.z*b.row3.x),
		(a.row3.x*b.row1.y)+(a.row3.y*b.row2.y)+(a.row3.z*b.row3.y),
		(a.row3.x*b.row1.z)+(a.row3.y*b.row2.z)+(a.row3.z*b.row3.z) )
		);

}
inline float3 operator* (const float3 & w, const M33 & m)
{
	return float3( (w.x*m.row1.x)+(w.y*m.row2.x)+(w.z*m.row3.x) , (w.x*m.row1.y)+(w.y*m.row2.y)+(w.z*m.row3.y) , (w.x*m.row1.z)+(w.y*m.row2.z)+(w.z*m.row3.z) );
}
inline M33 operator* (const M33 & m, float n)
{
	return M33(m.row1*n,m.row2*n,m.row3*n);
}
inline M33 operator* (float n, const M33 & m)
{
	return M33(m.row1*n,m.row2*n,m.row3*n);
}
inline M33 operator/ (const M33 & m, float n)
{
	return M33(m.row1/n,m.row2/n,m.row3/n);
}

////////////////////////////////////////////////////////////////////////////

class quat
{
public:
	inline quat(float xx=0.0f, float yy=0.0f, float zz=0.0f, float ww=1.0f)
	{
		x=xx;
		y=yy;
		z=zz;
		w=ww;
	}
	inline quat(const quat & q)
	{
		x=q.x;
		y=q.y;
		z=q.z;
		w=q.w;	
	}
	inline quat & operator= (const quat & q)
	{
		if(&q!=this)
		{
			x=q.x;
			y=q.y;
			z=q.z;
			w=q.w;
		}
		return *this;
	}

public:
	float x;
	float y;
	float z;
	float w;

public:
	inline void normalize()
	{
		float len=(float)sqrt((x*x)+(y*y)+(z*z)+(w*w));
		if(len==0){w=1;return;}
		x/=len;
		y/=len;
		z/=len;
		w/=len;	
	}
	inline void polaryzeto(const quat & q)
	{
		float d = (x*q.x) + (y*q.y) + (z*q.z) + (w*q.w);
		if(d<0)
		{
			x=-x;
			y=-y;
			z=-z;
			w=-w;
		}	
	}
	inline void blendto(quat q, float ww)
	{
		float d = (x*q.x) + (y*q.y) + (z*q.z) + (w*q.w);
		if(d<0)	{	q.x=-x;	q.y=-y;	q.z=-z;	q.w=-w;	}
		quat del(q.x-x,q.y-y,q.z-z,q.w-w);
		del*=ww;
		x+=del.x;
		y+=del.y;
		z+=del.z;
		w+=del.w;
		normalize();
	}

	inline bool isOK() const
	{
		return !( _isnan(x) && _isnan(y) && _isnan(z) && _isnan(w) );
	}

	inline void operator+= (const quat & a){x+=a.x;y+=a.y;z+=a.z;w+=a.w;}
	inline void operator-= (const quat & a){x-=a.x;y-=a.y;z-=a.z;w-=a.w;}
	inline void operator*= (float n){x*=n;y*=n;z*=n;w*=n;}
	inline void operator/= (float n){x/=n;y/=n;z/=n;w/=n;}
};

float Sign(float x);
float logr(float a);

inline void M33_quat( const M33 & m, quat & q )
{

	float q0 = ( m.row1.x + m.row2.y + m.row3.z + 1.0f) / 4.0f;
	float q1 = ( m.row1.x - m.row2.y - m.row3.z + 1.0f) / 4.0f;
	float q2 = (-m.row1.x + m.row2.y - m.row3.z + 1.0f) / 4.0f;
	float q3 = (-m.row1.x - m.row2.y + m.row3.z + 1.0f) / 4.0f;
	if(q0 < 0.0f) q0 = 0.0f;
	if(q1 < 0.0f) q1 = 0.0f;
	if(q2 < 0.0f) q2 = 0.0f;
	if(q3 < 0.0f) q3 = 0.0f;
	q0 = sqrt(q0);
	q1 = sqrt(q1);
	q2 = sqrt(q2);
	q3 = sqrt(q3);
	if(q0 >= q1 && q0 >= q2 && q0 >= q3) 
	{
		q0 *= +1.0f;
		q1 *= Sign(m.row3.y - m.row2.z);
		q2 *= Sign(m.row1.z - m.row3.x);
		q3 *= Sign(m.row2.x - m.row1.y);
	} 
	else if(q1 >= q0 && q1 >= q2 && q1 >= q3) 
	{
		q0 *= Sign(m.row3.y - m.row2.z);
		q1 *= +1.0f;
		q2 *= Sign(m.row2.x + m.row1.y);
		q3 *= Sign(m.row1.z + m.row3.x);
	} 
	else if(q2 >= q0 && q2 >= q1 && q2 >= q3) 
	{
		q0 *= Sign(m.row1.z - m.row3.x);
		q1 *= Sign(m.row2.x + m.row1.y);
		q2 *= +1.0f;
		q3 *= Sign(m.row3.y + m.row2.z);
	} 
	else if(q3 >= q0 && q3 >= q1 && q3 >= q2) 
	{
		q0 *= Sign(m.row2.x - m.row1.y);
		q1 *= Sign(m.row3.x + m.row1.z);
		q2 *= Sign(m.row3.y + m.row2.z);
		q3 *= +1.0f;
	}	
	q.x = q1;
	q.y = q2;
	q.z = q3;
	q.w = q0;
	q.normalize();
}

inline void quat_M33( const quat & q, M33 & m )
{
	m.row1 = float3(1-(2*((q.y*q.y)+(q.z*q.z))),2*((q.x*q.y)-(q.w*q.z)),2*((q.w*q.y)+(q.x*q.z)));
	m.row2 = float3(2*((q.x*q.y)+(q.w*q.z)),1-(2*((q.x*q.x)+(q.z*q.z))),2*((q.y*q.z)-(q.w*q.x)));
	m.row3 = float3(2*((q.x*q.z)-(q.w*q.y)),2*((q.y*q.z)+(q.w*q.x)),1-(2*((q.x*q.x)+(q.y*q.y))));
}


class transform3
{
public:
	inline transform3(float3 p=float3(0,0,0), M33 r=M33(float3(1.0f,0.0f,0.0f),float3(0.0f,1.0f,0.0f),float3(0.0f,0.0f,1.0f)))
	{
		mPosition=p;
		mRotation=r;
	}
	transform3( float px, float py, float pz, float r0, float r1, float r2, float r3, float r4, float r5, float r6, float r7, float r8 )
	{
		mPosition.x = px;
		mPosition.y = py;
		mPosition.z = pz;

		mRotation.row1.x = r0;
		mRotation.row1.y = r1;
		mRotation.row1.z = r2;

		mRotation.row2.x = r3;
		mRotation.row2.y = r4;
		mRotation.row2.z = r5;

		mRotation.row3.x = r6;
		mRotation.row3.y = r7;
		mRotation.row3.z = r8;
	}
	inline transform3(const transform3 & tr)
	{
		mPosition=tr.mPosition;
		mRotation=tr.mRotation;
	}
	inline transform3 & operator = (const transform3 & tr)
	{
		if(&tr!=this)
		{
			mPosition=tr.mPosition;
			mRotation=tr.mRotation;
		}
		return *this;
	}
	void copy( float* arr ) const
	{
		arr[0] = mRotation.row1.x;
		arr[1] = mRotation.row1.y;
		arr[2] = mRotation.row1.z;

		arr[3] = mRotation.row2.x;
		arr[4] = mRotation.row2.y;
		arr[5] = mRotation.row2.z;

		arr[6] = mRotation.row3.x;
		arr[7] = mRotation.row3.y;
		arr[8] = mRotation.row3.z;

		arr[9] = mPosition.x;
		arr[10] = mPosition.y;
		arr[11] = mPosition.z;
	}
	void copy_qtm( float* arr ) const
	{
		quat q;
		M33_quat( mRotation, q );
		arr[3] = q.x;
		arr[4] = q.y;
		arr[5] = q.z;
		arr[6] = q.w;
		arr[0] = mPosition.x;
		arr[1] = mPosition.y;
		arr[2] = mPosition.z;
	}

public:
	float3		mPosition;
	M33			mRotation;
};
inline float3 operator* (const float3 & p, const transform3 & m)
{
	return ((p*m.mRotation)+m.mPosition);
}
inline transform3 operator* (const transform3 & a, const transform3 & b)
{
	return transform3( a.mPosition*b, M33(a.mRotation.row1*b.mRotation,a.mRotation.row2*b.mRotation,a.mRotation.row3*b.mRotation) ); 
}
inline transform3 inverse (const transform3 & t)
{
	M33 im=inverse(t.mRotation);
	return transform3( (-1*t.mPosition)*im , im );
}
inline transform3 directiontrlookat ( const float3 & posp, const float3 & dirp, const float3 & lookatp )
{
	transform3 wyn=transform3();
	wyn.mPosition=posp;
	wyn.mRotation.row3=normalize( lookatp - posp );
	wyn.mRotation.row1=normalize(  cross( wyn.mRotation.row3, dirp )  );
	wyn.mRotation.row2=normalize( cross( wyn.mRotation.row1, wyn.mRotation.row3 ) );
	return wyn;
}
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

inline quat operator* (const quat & a, const quat & b)
{
	float3 av=float3(a.x,a.y,a.z);
	float3 bv=float3(b.x, b.y, b.z);
	float3 v=(cross(av,bv))+(av*b.w)+(bv*a.w);
	return quat(v.x, v.y, v.z, (a.w*b.w)-(av*bv));
}
inline float3 operator* (const float3 & f, const quat & q)
{
	quat w=quat(f.x, f.y, f.z, 0);
	quat s=quat(-q.x, -q.y, -q.z, q.w);
	quat wyn= (s*w*q);
	return float3(wyn.x, wyn.y, wyn.z);
}
inline quat operator+ (const quat & a,const quat & b)
{
	return quat( a.x+b.x , a.y+b.y, a.z+b.z, a.w+b.w );
}
inline quat operator- (const quat & a,const quat & b)
{
	return quat( a.x-b.x , a.y-b.y , a.z-b.z, a.w-b.w );
}
inline quat operator* (const quat & a, float n)
{
	return quat( a.x*n, a.y*n, a.z*n, a.w*n );
}
inline quat operator* (float n, const quat & a)
{
	return quat( a.x*n, a.y*n, a.z*n, a.w*n );
}
inline quat operator/ (const quat & a, float n)
{
	return quat(a.x/n, a.y/n, a.z/n, a.w/n);
}

///////////////////////////////////////////////////////////////

class qtransform
{
public:
	float3		pos;
	quat		rot;

	static const unsigned int NUM_ELEMENTS = 7;

public:
	inline qtransform(float3 p=float3(), quat r=quat())
		: pos( p )
		, rot( r )
	{
	}
	inline void add(const qtransform & qt, float w)
	{
		pos += qt.pos*w;
		quat r = (qt.rot);
		r.w-=1;
		r=r*w;
		r.w+=1;
		r.normalize();
		rot = rot*r;
	}
	inline bool isOK() const
	{
		return pos.isOK() && rot.isOK();
	}
};

class qtransform_scale : public qtransform
{
public:
	static const unsigned NUM_ELEMENTS = 10;

	inline qtransform_scale( const qtransform& qt )
		: qtransform( qt )
		, mScale( 1.f, 1.f, 1.f )
	{

	}

	inline qtransform_scale( float3 p = float3(), quat r = quat(), float3 s = float3( 1.f, 1.f, 1.f ) )
		: qtransform( p, r )
		, mScale( s )
	{
	}

	inline float operator[](int a) const { return reinterpret_cast<const float*>( this )[a]; }
	
	/*
	inline qtransform_scale& operator = ( const qtransform_scale& other ) const
	{
		if( &other != this )
		{
			pos = other.pos;
			rot = other.rot;
			mScale = other.mScale;
		}
		return this;
	}
	*/

	inline float operator == ( const qtransform_scale& other) const
	{
		return pos.x == other.pos.x && pos.y == other.pos.y && pos.z == other.pos.z &&
		rot.x == other.rot.x && rot.y == other.rot.y && rot.z == other.rot.z && rot.w == other.rot.w &&
		mScale.x == other.mScale.x && mScale.y == other.mScale.y && mScale.z == other.mScale.z;
	}

	inline float operator != ( const qtransform_scale& other ) const
	{
		return pos.x != other.pos.x || pos.y != other.pos.y || pos.z != other.pos.z ||
			rot.x != other.rot.x || rot.y != other.rot.y || rot.z != other.rot.z || rot.w != other.rot.w ||
			mScale.x != other.mScale.x || mScale.y != other.mScale.y || mScale.z != other.mScale.z;
	}

// private:
	float3 mScale;
};

static_assert( qtransform::NUM_ELEMENTS == sizeof(qtransform)/sizeof(float) , "Change num elements" );
static_assert( qtransform_scale::NUM_ELEMENTS == sizeof(qtransform_scale)/sizeof(float) , "Change num elements" );

inline float3 operator* (const float3 & w, const qtransform & q)
{
	return (w*q.rot)+q.pos;
}
inline qtransform operator* (const qtransform & a, const qtransform & b)
{
	return qtransform( a.pos * b, a.rot * b.rot);
}
inline qtransform inverse(const qtransform & q)
{
	quat iq=quat(-q.rot.x, -q.rot.y, -q.rot.z, q.rot.w);
	return qtransform( float3(-q.pos.x,-q.pos.y,-q.pos.z)*iq , iq );
}

inline void transform3_qtransform( const transform3 & tm, qtransform & qtm )
{
	qtm.pos = tm.mPosition;
	M33_quat( tm.mRotation, qtm.rot );
}

inline void qtransform_transform3( const qtransform & qtm, transform3 & tm )
{
	tm.mPosition = qtm.pos;
	quat_M33( qtm.rot, tm.mRotation );
}

inline void qtransform_transform3( const qtransform_scale & qtm, transform3 & tm )
{
	qtransform_transform3( static_cast<const qtransform&>( qtm ) , tm );

	tm.mRotation.row1 *= qtm.mScale.x;
	tm.mRotation.row2 *= qtm.mScale.y;
	tm.mRotation.row3 *= qtm.mScale.z;
}

inline float re_simple_lerp( float a, float b, float w)
{
	return a+(b)*w;
}

inline float3 lerp( const float3 & a, const float3 & b, float w )
{
	return float3( a.x*(1.0f-w)+b.x*w, a.y*(1.0f-w)+b.y*w, a.z*(1.0f-w)+b.z*w );
}

inline float2 lerp( const float2 & a, const float2 & b, float w )
{
	return float2( a.x*(1.0f-w)+b.x*w, a.y*(1.0f-w)+b.y*w );
}

inline float3 slerp( const float3 & a, const float3 & b, float w )
{
	float d = a*b;
	if( d>0.9995f )
	{
		return normalize( lerp( a, b, w) );
	}
	float t = acosf( d )*w;
	float3 x = normalize( b - a*d );
	return a*cosf(t) + x*sinf(t);
}

inline quat normalize( const quat & q )
{
	quat w = q;
	w.normalize();
	return w;
}

inline quat lerp( const quat & a, const quat & b, float w )
{
	float d = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
	if( d<0.0f )
	{
		quat q = a + (quat(-b.x,-b.y,-b.z,-b.w)-a)*w;
		q.normalize();
		return q;		
	}
	else
	{
		quat q = a + (b-a)*w;
		q.normalize();
		return q;
	}
}

inline quat slerp( const quat & a, const quat & b, float w )
{
	float d = a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
	quat temp = b;
	if( d<0.0f )
	{
		temp.x = -b.x; 
		temp.y = -b.y; 
		temp.z = -b.z; 
		temp.w = -b.w; 
	}
	d = a.x*temp.x+a.y*temp.y+a.z*temp.z+a.w*temp.w;
	if( d>0.995f )
	{
		return normalize( lerp(a,temp, w) );
	}
	float t = acosf( d )*w;
	quat x = normalize( temp - a*d );
	return a*cosf(t) + x*sinf(t);
}


inline quat M33_quat( const M33 & m )
{
	quat q;
	M33_quat( m, q );
	return q;
}

inline M33 quat_M33( const quat & q )
{
	M33 m;
	quat_M33( q, m );
	return m;
}

inline qtransform transform3_qtransform( const transform3 & m )
{
	qtransform q;
	transform3_qtransform( m, q );
	return q;
}

inline transform3 qtransform_transform3( const qtransform& q )
{
	transform3 m;
	qtransform_transform3( q, m );
	return m;
}

inline transform3 qtransform_transform3( const qtransform_scale& q )
{
	transform3 m;
	qtransform_transform3( q, m );
	return m;
}

/////////////////////////////////////////////////////////////////

enum euler_order{xyz, xzy, yxz, yzx, zxy, zyx};

class eulers
{
public:
	inline eulers()
	{
		order=xyz;
		x=y=z=0.0f;
	}
	inline eulers(float a, float b, float c, int or=xyz)
	{
		x=a;
		y=b;
		z=c;
		order=or;
	}
	inline eulers(const eulers & q)
	{
		x=q.x;
		y=q.y;
		z=q.z;
		order = q.order;
	}
	inline eulers & operator= (const eulers & q)
	{
		if(&q!=this)
		{
			x=q.x;
			y=q.y;
			z=q.z;
			order = q.order;
		}
		return *this;	
	}
	operator M33();
	void Get(const quat & q);

	float x;
	float y;
	float z;
	int order;
};

/////////////////////////////////////////////////////////////////

class ray2
{
public:
	inline ray2(float2 p=float2(), float2 d=float2(0.0f,1.0f))
	{
		pos = p;
		dir = d;
	}
	inline bool hits_line(  )
	{
		//(a.y*b.x-a.x*b.y-a.y*p.x+b.y*p.x+a.x*p.y-b.x*p.y)/(a.y*d.x-b.y*d.x-a.x*d.y+b.x*d.y)
	}
	float2 pos;
	float2 dir;
};

class bbox2
{
public:
	inline bbox2(float2 a = float2(-100.0f,-100.0f), float2 b = float2( 100.0f, 100.0f))
	{
		minp = a;
		maxp = b;	
	}
	float2 minp;
	float2 maxp;
};

///////////////////////////////////////////////////////////////////////////////////////

class ray3
{
public:
	inline ray3(float3 p=float3(), float3 d=float3(0.0f,0.0f,1.0f))
	{
		pos = p;
		dir = d;
	}

public:
	bool hits_triangle( const float3 & aa, const float3 & bb, const float3 & cc, float3 & out, float & dist );
	bool hits_triangle( const float3 & aa, const float3 & bb, const float3 & cc );
	bool hits_bbox( const float3 & minp, const float3 & maxp, float & hit_distance_back, float & hit_distance_front );
	bool hits_bbox( const float3 & minp, const float3 & maxp );
	bool hits_sphere( const float3 & center, float radius );
	bool hits_sphere( const float3 & center, float radius, float & hit_distance_back, float & hit_distance_front );
	bool hits_plane( const float3 & point, const float3 & planedir );
	bool hits_plane( const float3 & point, const float3 & planedir, float & hit_distance );

public:
	float3 pos;
	float3 dir;
};

float3 two_lines_closest_points( const ray3 & a, const ray3 & b );

bool transform2_contains( const transform2 & tm, const float2 & p ); // checks if p is in 0-1 plane
bool bbox_line_intersection( const float2 & a, const float2 & b, const float2 & min, const float2 & max );


////////////////////////////////////////////////////////////////////////////////////////

bool box3_triangle_intersection( const float3 & minb, const float3 & maxb, const float3 & aa, const float3 & bb, const float3 & cc );

inline bool box1_intersection( float mina, float maxa, float minb, float maxb )
{
	if( mina>=maxb || maxa<=minb ){return false;}else{return true;}
}
inline bool box2_intersection( const float2 & mina, const float2 & maxa, const float2 & minb, const float2 & maxb )
{
	if( !box1_intersection(mina.x, maxa.x, minb.x, maxb.x) ){return false;}
	if( !box1_intersection(mina.y, maxa.y, minb.y, maxb.y) ){return false;}
	return true;
}
inline bool box3_intersection( const float3 & mina, const float3 & maxa, const float3 & minb, const float3 & maxb )
{
	if( !box1_intersection(mina.x, maxa.x, minb.x, maxb.x) ){return false;}
	if( !box1_intersection(mina.y, maxa.y, minb.y, maxb.y) ){return false;}
	if( !box1_intersection(mina.z, maxa.z, minb.z, maxb.z) ){return false;}
	return true;
}

class bbox3
{
public:
	inline bbox3(float3 a = float3(-100.0f,-100.0f,-100.0f), float3 b = float3( 100.0f, 100.0f, 100.0f))
	{
		minp = a;
		maxp = b;
		is_reset = true;	
	}
	float3 minp;
	float3 maxp;
	bool is_reset;
	inline bool intersects_triangle( const float3 & aa, const float3 & bb, const float3 & cc )
	{
		return box3_triangle_intersection(minp,maxp,aa,bb,cc);
	}
	inline bool intersects_bbox( const float3 & aa, const float3 & bb )
	{
		return box3_intersection(minp,maxp,aa,bb);
	}
	inline void reset(  )
	{
		is_reset = true;
	}
	inline void point( const float3 & v )
	{
		if( is_reset )
		{
			minp = v;
			maxp = v;
			is_reset = false;
		}
		else
		{
			if( v.x<minp.x ){ minp.x = v.x; }
			if( v.y<minp.y ){ minp.y = v.y; }
			if( v.z<minp.z ){ minp.z = v.z; }
			if( v.x>maxp.x ){ maxp.x = v.x; }
			if( v.y>maxp.y ){ maxp.y = v.y; }
			if( v.z>maxp.z ){ maxp.z = v.z; }
		}
	}
};




//////////////////////////
//pat_math cpp

inline M22 Mrotation (float alfa)
{
	float2 os = float2(  (float)cosf((alfa/2)*0.0348888888889f), (float)sinf((alfa/2)*0.0348888888889f));
	return M22(  os, float2( -os.y, os.x )  );
}

inline M33 Mrotationx(float alfa)
{
	M33 wyn;
	alfa*=0.03488888888f*0.5;
	wyn.row1=float3(1,0,0);
	wyn.row2=float3(0,(float)cos(alfa),(float)sin(alfa));
	wyn.row3=float3(0,-wyn.row2.z, wyn.row2.y);
	return wyn;
}

inline M33 Mrotationy(float alfa)
{
	M33 wyn;
	alfa*=0.0348888888f*0.5;
	wyn.row1=float3((float)cos(alfa),0,(float)sin(alfa));
	wyn.row2=float3(0,1,0);
	wyn.row3=float3(-wyn.row1.z,0, wyn.row1.x);
	return wyn;
}

inline M33 Mrotationz(float alfa)
{
	M33 wyn;
	alfa*=0.0348888888f*0.5;
	wyn.row1=float3((float)cos(alfa),(float)sin(alfa),0);
	wyn.row2=float3(-wyn.row1.y, wyn.row1.x,0);
	wyn.row3=float3(0,0,1);
	return wyn;
}

inline bool box3_triangle_intersection( const float3 & minb, const float3 & maxb, const float3 & aa, const float3 & bb, const float3 & cc )
{
	float3 cen = (maxb+minb)*0.5;
	float3 half = maxb-cen;
	float3 a = aa-cen;
	float3 b = bb-cen;
	float3 c = cc-cen;

	float3 minp = a;
	float3 maxp = a;
	if( b.x<minp.x ){minp.x=b.x;}
	if( b.y<minp.y ){minp.y=b.y;}
	if( b.z<minp.z ){minp.z=b.z;}
	if( b.x>maxp.x ){maxp.x=b.x;}
	if( b.y>maxp.y ){maxp.y=b.y;}
	if( b.z>maxp.z ){maxp.z=b.z;}

	if( c.x<minp.x ){minp.x=c.x;}
	if( c.y<minp.y ){minp.y=c.y;}
	if( c.z<minp.z ){minp.z=c.z;}
	if( c.x>maxp.x ){maxp.x=c.x;}
	if( c.y>maxp.y ){maxp.y=c.y;}
	if( c.z>maxp.z ){maxp.z=c.z;}

	if( !box3_intersection(half*-1,half,minp,maxp) ){return false;}

	float3 ed = b-a;
	float edist = (a.x*ed.y)-(a.y*ed.x);
	float vdist = (c.x*ed.y)-(c.y*ed.x);
	if(edist>vdist){float temp = edist;edist = vdist;vdist = temp;}
	float rad = (half.x*fabs(ed.y))+(half.y*fabs(ed.x));
	if( !box1_intersection( -rad, rad, edist, vdist) ){return false;}

	edist = (a.x*ed.z)-(a.z*ed.x);
	vdist = (c.x*ed.z)-(c.z*ed.x);
	if(edist>vdist){float temp = edist;edist = vdist;vdist = temp;}
	rad = (half.x*fabs(ed.z))+(half.z*fabs(ed.x));
	if( !box1_intersection( -rad, rad, edist, vdist) ){return false;}

	edist = (a.y*ed.z)-(a.z*ed.y);
	vdist = (c.y*ed.z)-(c.z*ed.y);
	if(edist>vdist){float temp = edist;edist = vdist;vdist = temp;}
	rad = (half.y*fabs(ed.z))+(half.z*fabs(ed.y));
	if( !box1_intersection( -rad, rad, edist, vdist) ){return false;}

	////////////////////
	ed = c-a;
	edist = (a.x*ed.y)-(a.y*ed.x);
	vdist = (b.x*ed.y)-(b.y*ed.x);
	if(edist>vdist){float temp = edist;edist = vdist;vdist = temp;}
	rad = (half.x*fabs(ed.y))+(half.y*fabs(ed.x));
	if( !box1_intersection( -rad, rad, edist, vdist) ){return false;}

	edist = (a.x*ed.z)-(a.z*ed.x);
	vdist = (b.x*ed.z)-(b.z*ed.x);
	if(edist>vdist){float temp = edist;edist = vdist;vdist = temp;}
	rad = (half.x*fabs(ed.z))+(half.z*fabs(ed.x));
	if( !box1_intersection( -rad, rad, edist, vdist) ){return false;}

	edist = (a.y*ed.z)-(a.z*ed.y);
	vdist = (b.y*ed.z)-(b.z*ed.y);
	if(edist>vdist){float temp = edist;edist = vdist;vdist = temp;}
	rad = (half.y*fabs(ed.z))+(half.z*fabs(ed.y));
	if( !box1_intersection( -rad, rad, edist, vdist) ){return false;}

	///////////////////
	ed = c-b;
	edist = (b.x*ed.y)-(b.y*ed.x);
	vdist = (a.x*ed.y)-(a.y*ed.x);
	if(edist>vdist){float temp = edist;edist = vdist;vdist = temp;}
	rad = (half.x*fabs(ed.y))+(half.y*fabs(ed.x));
	if( !box1_intersection( -rad, rad, edist, vdist) ){return false;}

	edist = (b.x*ed.z)-(b.z*ed.x);
	vdist = (a.x*ed.z)-(a.z*ed.x);
	if(edist>vdist){float temp = edist;edist = vdist;vdist = temp;}
	rad = (half.x*fabs(ed.z))+(half.z*fabs(ed.x));
	if( !box1_intersection( -rad, rad, edist, vdist) ){return false;}

	edist = (b.y*ed.z)-(b.z*ed.y);
	vdist = (a.y*ed.z)-(a.z*ed.y);
	if(edist>vdist){float temp = edist;edist = vdist;vdist = temp;}
	rad = (half.y*fabs(ed.z))+(half.z*fabs(ed.y));
	if( !box1_intersection( -rad, rad, edist, vdist) ){return false;}

	////////////////////////////////////

	float3 nor = normalize( cross(b-a,c-a) );
	float d = (nor*a)*-1;
	float3 vmin;
	float3 vmax;
	if( nor.x>0 ){vmin.x=-half.x; vmax.x=half.x;}else{vmin.x = half.x; vmax.x=-half.x;}
	if( nor.y>0 ){vmin.y=-half.y; vmax.y=half.y;}else{vmin.y = half.y; vmax.y=-half.y;}
	if( nor.z>0 ){vmin.z=-half.z; vmax.z=half.z;}else{vmin.z = half.z; vmax.z=-half.z;}

	if( (nor*vmin)+d>0 || (nor*vmax)+d<=0 ){return false;}

	return true;
}

inline float3 two_lines_closest_points( const ray3 & a, const ray3 & b )
{
	transform3 tm;
	tm.mPosition = a.pos;
	tm.mRotation.row1 = a.dir;
	tm.mRotation.row2 = b.dir;
	tm.mRotation.row3 = normalize( cross( tm.mRotation.row1, tm.mRotation.row2 ) );
	float3 p = b.pos * inverse(tm);
	return float3( (p.x), (p.y), fabs(p.z) );
}


inline transform3 rotate_fromdir( const float3 & from, const float3 & to, float w )
{
	float3 os = normalize( cross( from, to ) );
	float an = acosf( from*to )*0.5f;
	float san = sinf( an );
	quat q( os.x*san, os.y*san, os.z*san, cosf(an) );
	q.w -= 1.0f;
	q *= w;
	q.w += 1.0f;
	q.normalize();
	M33 rot;
	quat_M33( q, rot );
	return transform3( float3(), rot );
}

///////////////////////////
// headers

bool bbox_line_intersection( const float2 & a, const float2 & b, const float2 & min, const float2 & max );


////////////////////////////
// pat_math_context

class pat_math_context
{
public:

	static inline void identity_m22( float* m )
	{
		memset(m,0,sizeof(float)*4);
		m[0] = 1.0f;
		m[3] = 1.0f;
	}
	static inline void identity_m32_rp( float* m )
	{
		memset(m,0,sizeof(float)*6);
		m[0] = 1.0f;
		m[3] = 1.0f;
	}
	static inline void identity_m32_pr( float* m )
	{
		memset(m,0,sizeof(float)*6);
		m[2] = 1.0f;
		m[5] = 1.0f;
	}
	static inline void set_m32( float* m, float p1, float p2, float p3, float p4, float p5, float p6 )
	{
		m[0] = p1;		m[1] = p2;		m[2] = p3;		m[3] = p4;		m[4] = p5;		m[5] = p6;
	}

	static inline void mul_vxm( float * w, float * m, float * o, int count, int rows, int columns )
	{
		float* wek = w;
		float* mat = m;
		float* out = o;
		float* matend = mat + (rows*columns);
		float* outend = out + (count-1);
		while( mat!=matend )
		{
			(*out) += (*wek)*(*mat++);
			if( out==outend ){	wek++;	out = o;	}else{	out++;	}
		}
	}

	static inline void add_v2( float * o, float * m )
	{
		o[0] += m[0];
		o[1] += m[1];
	}

	static inline void mul_v2( float * o, float v )
	{
		o[0] *= v;
		o[1] *= v;
	}

	static inline void mul_v2xm22( float * w, float * m, float * o )
	{
		o[0] = w[0]*m[0] + w[1]*m[2];
		o[1] = w[0]*m[1] + w[1]*m[3];
	}

	static void inverse_m22( float* m, float* o )
	{
		float d = (m[0]*m[3]) - (m[1]*m[2]);
		o[0] =  m[3]/d;
		o[1] = -m[1]/d;
		o[2] = -m[2]/d;
		o[3] =  m[0]/d;
	}

	static inline void mul_m22xm22( float * a, float * b, float * o )
	{
		o[0] = a[0]*b[0] + a[1]*b[2];
		o[1] = a[0]*b[1] + a[1]*b[3];
		o[2] = a[2]*b[0] + a[3]*b[2];
		o[3] = a[2]*b[1] + a[3]*b[3];
	}

	static inline void mul_v2xm32_rp( float * w, float * m, float * o )
	{
		mul_v2xm22( w, m, o );
		add_v2( o, m+4 );
	}

	static inline void mul_v2xm32_pr( float * w, float * m, float * o )
	{
		mul_v2xm22( w, m+2, o );
		add_v2( o, m );
	}

	static inline void mul_v2xm32_r0( float * w, float * m, float * o )
	{
		mul_v2xm22( w, m, o );
	}

	static inline void mul_v2xm32_0r( float * w, float * m, float * o )
	{
		mul_v2xm22( w, m+2, o );
	}

	static inline void mul_m32xm32_rp( float * a, float * b, float * o )
	{
		mul_m22xm22(a,b,o);
		mul_v2xm32_rp(a+4,b,o+4);
	}

	static inline void mul_m32xm32_pr( float * a, float * b, float * o )
	{
		mul_m22xm22(a+2,b+2,o+2);
		mul_v2xm32_rp(a,b,o);
	}

	static void inverse_m32_rp( float* m, float* o )
	{
		inverse_m22( m, o );
		mul_v2xm22( m+4, o, o+4 );
		mul_v2( o+4, -1.0f );
	}

	static void inverse_m32_pr( float* m, float* o )
	{
		inverse_m22( m+2, o+2 );
		mul_v2xm22( m, o, o );
		mul_v2( o, -1.0f );
	}

};

inline quat inverse( const quat& q )
{
	return quat( -q.x, -q.y, -q.z, q.w );
}

////////////////////////////////////////////////////////////////////////////

class ReBBox
{
public:
	ReBBox();
	ReBBox( const float3& min, const float3& max );


public:
	void			addPoint( const float3& point );
	float3			getCenter();
	float3			getDiagonal();
	int				getLongestEdge();

private:
	float3	mMin;
	float3	mMax;
};
