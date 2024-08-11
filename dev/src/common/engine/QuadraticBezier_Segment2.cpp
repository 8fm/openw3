
#include "build.h"
/*
#include "QuadraticBezier_Segment2.h"

namespace QuadraticBezierNamespace
{
	float2 QuadraticBezier_Segment2::Evaluate(const float2 & p0, const float2 & p1, const float2 & p2, float t)
	{
		float2 A = p0-2*p1+p2;
		float2 B = 2*p1-2*p0;
		float2 C = p0;
		return (t*t)*A+t*B+C; 
	}
	float2 QuadraticBezier_Segment2::EvaluateOffset(const float2 & p0, const float2 & p1, const float2 & p2, float t, float offset)
	{
		float2 A = p0-2*p1+p2;
		float2 B = 2*p1-2*p0;
		float2 C = p0;
		float2 vel = Normalize(2*(t)*A+B);
		float2 delta = float2(-vel.y,vel.x)*offset; 
		return ((t*t)*A+t*B+C)+delta; 
	}
	float2 QuadraticBezier_Segment2::Velocity(const float2 & p0, const float2 & p1, const float2 & p2, float t)
	{
			float2 A = p0-2*p1+p2;
			float2 B = 2*p1-2*p0;
			return 2*(t)*A+B; 
	}
	float QuadraticBezier_Segment2::NearestPoint(const float2 & p0, const float2 & p1, const float2 & p2, const float2 & x)
	{
			float2 a = p0-2*p1+p2;
			float2 b = 2*p1-2*p0;
			float2 c = p0;
			float A = a.x*a.x + a.y*a.y;
			float B = 2*a.x*b.x + 2*a.y*b.y;
			float C = 2*a.x*c.x-2*a.x*x.x+b.x*b.x + 2*a.y*c.y-2*a.y*x.y+b.y*b.y;
			float D = 2*b.x*c.x-2*b.x*x.x + 2*b.y*c.y-2*b.y*x.y;
			float E = pow((c.x-x.x),2) + pow((c.y-x.y),2);
			Polynomial p(4);
			p.m_constants[0]=A;
			p.m_constants[1]=B;
			p.m_constants[2]=C;
			p.m_constants[3]=D;
			p.m_constants[4]=E;
			Polynomial* dev = p.Derivative();
			float roots[3];
			int numroots=0;
			dev->FindRoots((float*)&roots,numroots);
			float win = 0.f;
			float minv = p.Value(win);
			float testv = p.Value(1.0f);
			if(testv<minv){minv=testv; win = 1.0f ;}
			for(int i=0;i<numroots;i++)
			{
				if(roots[i]>=0 && roots[i]<=1)
				{
					float testv = p.Value(roots[i]);
					if(testv<minv){minv=testv; win = roots[i];}	
				}
			}
			delete dev;
			return win;
	}
	void QuadraticBezier_Segment2::PointsOnSplineAtDistance(const float2 & p0, const float2 & p1, const float2 & p2, const float2 & xx, float z, float* arr, int & numpoints)
	{
			float2 pa = p0-2*p1+p2;
			float2 pb = 2*p1-2*p0;
			float2 pc = p0;	
			float a = pa.x;
			float u = pa.y;
			float b = pb.x;
			float v = pb.y;
			float c = pc.x;
			float w = pc.y;
			float x=xx.x;
			float y=xx.y;
			float A=(a*a)+(u*u);
			float B=2*((a*b)+(u*v));
			float C=2*((a*c)-(a*x)+(u*w)-(u*y))+(b*b)+(v*v);
			float D=2*((b*c)-(b*x)+(v*w)-(v*y));
			float F=(c*c)-(2*c*x)+(w*w)-(2*w*y)+(x*x)+(y*y)-(z*z);
			Polynomial p(4);
			p.m_constants[0]=A;
			p.m_constants[1]=B;
			p.m_constants[2]=C;
			p.m_constants[3]=D;
			p.m_constants[4]=F;
			p.FindRoots(arr,numpoints);
	}
	float QuadraticBezier_Segment2::Length(const float2 & p0, const float2 & p1, const float2 & p2, float t)
	{
			float2 A = p0-2*p1+p2;
			float2 B = 2*p1-2*p0;
			if(fabs(A.x)<0.00001 && fabs(B.x)<0.00001)
			{
				return fabs(p2.y-p0.y)*t;
			}
			if(fabs(A.y)<0.00001 && fabs(B.y)<0.00001)
			{
				return fabs(p2.x-p0.x)*t;
			}
			return Len(t, A.x,B.x,A.y,B.y);
	}
	float QuadraticBezier_Segment2::LengthNormalized(const float2 & p0, const float2 & p1, const float2 & p2, float t)
	{
			float Leng = Length(p0,p1,p2,t);
			float LengAll = Length(p0,p1,p2);
			return Leng/LengAll;
	}
	float QuadraticBezier_Segment2::FindTOnLength(const float2 & p0, const float2 & p1, const float2 & p2, float len)
	{
			float st=0.0f;
			float en=1.0f;
			float sr = (st+en)*0.5f;
			float vst = Length(p0,p1,p2,st);
			float ven = Length(p0,p1,p2,en);
			while(en-st>0.001)
			{
				sr = (st+en)*0.5f;
				float srv = Length(p0,p1,p2,sr);
				if(len<srv)
				{
					en=sr;	
				}
				else
				{
					st=sr;
				}
			}
			return sr;
	}
	void QuadraticBezier_Segment2::FindBoundingBox(const float2 & p0, const float2 & p1, const float2 & p2, float2 & min, float2 & max)
	{
			float2 pa = p0-2*p1+p2;
			float2 pb = 2*p1-2*p0;
			float2 pc = p0;
			Polynomial px(2); px.m_constants[0]=pa.x; px.m_constants[1]=pb.x; px.m_constants[2]=pc.x;
			Polynomial py(2); py.m_constants[0]=pa.y; py.m_constants[1]=pb.y; py.m_constants[2]=pc.y;
			float roots[1];
			int numroots=0;
			Polynomial* pxd = px.Derivative();
			pxd->FindRoots((float*)&roots,numroots);
			float x0 = px.Value(0.f);
			float x1 = px.Value(1.f);
			min.x=x0;
			max.x=x0;
			if(x1<min.x){min.x=x1;} if(x1>max.x){max.x=x1;}
			if(numroots>0 && roots[0]>=0 && roots[0]<=1)
			{
				float x = px.Value(roots[0]);
				if(x<min.x){min.x=x;} if(x>max.x){max.x=x;}
			}
			numroots=0;
			Polynomial* pyd = py.Derivative();
			pyd->FindRoots((float*)&roots,numroots);
			float y0 = py.Value(0.f);
			float y1 = py.Value(1.f);
			min.y=y0;
			max.y=y0;
			if(y1<min.y){min.y=y1;} if(y1>max.y){max.y=y1;}
			if(numroots>0 && roots[0]>=0 && roots[0]<=1)
			{
				float y = py.Value(roots[0]);
				if(y<min.y){min.y=y;} if(y>max.y){max.y=y;}
			}
			delete pxd;
			delete pyd;
	}
	float QuadraticBezier_Segment2::Len(float z, float a, float b, float c, float d)
	{
			float q=pow(a,2)+pow(c,2);
			float w=pow(b,2)+pow(d,2);
			float p = sqrtf( w + (4*a*b*z) + (4*z*((c*d)+(q*z))));
			float naw1 = 1.0f/(4*sqrt(pow(q,3)));
			float naw2 = -sqrt(q)*((a*b)+(c*d))*sqrt(w);
			float naw3 = sqrt(q)*((a*b)+(c*d)+(2*q*z))*p;
			float naw4 = pow(((b*c)-(a*d)),2) * log( (a*b)+(c*d)+(sqrt(q)*sqrt(w)) );
			float naw5 = pow(((b*c)-(a*d)),2) * log((a*b)+(c*d) +(2*z*q)+(sqrt(q)*p));
			return naw1*(naw2+naw3-naw4+naw5);
	}	
}
*/