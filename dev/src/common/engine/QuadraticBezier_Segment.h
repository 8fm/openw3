
#pragma once

#include "Bezier_Polynomials.h"

namespace QuadraticBezierNamespace
{
	template< typename T >
	T Evaluate(const T & p0, const T & p1, const T & p2, float t)
	{
		T A = p0-2*p1+p2;
		T B = 2*p1-2*p0;
		T C = p0;
		return (t*t)*A+t*B+C; 
	}
	template< typename T >
	T Velocity(const T & p0, const T & p1, const T & p2, float t)
	{
		T A = p0-2*p1+p2;
		T B = 2*p1-2*p0;
		return 2*(t)*A+B; 
	}
	template< typename T >
	Float NearestPoint(const T & p0, const T & p1, const T & p2, const T & x)
	{
		T a = p0-2*p1+p2;
		T b = 2*p1-2*p0;
		T c = p0;
		T cx = c-x;
		Float c4 = a*a;
		Float c3 = 2*a*b;
		Float c2 = b*b+2*a*c-2*a*x;
		Float c1 = 2*b*cx;
		Float c0 = cx*cx;
		Polynomial p(4);
		p.m_constants[0]=c4;
		p.m_constants[1]=c3;
		p.m_constants[2]=c2;
		p.m_constants[3]=c1;
		p.m_constants[4]=c0;
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
	template< typename T >
	void PointsOnSplineAtDistance(const T & p0, const T & p1, const T & p2, const T & x, float z, float* arr, int & numpoints)
	{
		T a = p0-2*p1+p2;
		T b = 2*p1-2*p0;
		T c = p0;
		T cx = c-x;
		Float c4 = a*a;
		Float c3 = 2*a*b;
		Float c2 = b*b+2*a*c-2*a*x;
		Float c1 = 2*b*cx;
		Float c0 = cx*cx-(z*z);
		Polynomial p(4);
		p.m_constants[0]=c4;
		p.m_constants[1]=c3;
		p.m_constants[2]=c2;
		p.m_constants[3]=c1;
		p.m_constants[4]=c0;
		p.FindRoots(arr,numpoints);
	}
	inline float LengthExpr(Float c0, Float c1, Float c2, float t = 1.0f)
	{
		Float sc0 = sqrt(c0);
		Float sc2 = sqrt(c2);
		Float m1 = 2*sc2;
		Float pierw = sqrt(c0+t*(c1+c2*t));
		Float logar = log(c1+sc0*m1) - log(c1+m1*pierw+2*c2*t);
		Float licz = m1*(-sc0*c1+pierw*(c1+2*c2*t))+(c1*c1-4*c0*c2)*logar;
		Float mian = 8*pow(c2,(3.f/2.f));
		return licz/mian;
	}
	template< typename T >
	float Length(const T & p0, const T & p1, const T & p2, float t = 1.0f)
	{
		T A = p0-2*p1+p2;
		T B = 2*p1-2*p0;
		Float c2 = 4*A*A;
		Float c1 = 4*A*B;
		Float c0 = B*B;
		return LengthExpr(c0,c1,c2,t);
	}
	template< typename T >
	float LengthNormalized(const T & p0, const T & p1, const T & p2, float t)
	{
		float Leng = Length(p0,p1,p2,t);
		float LengAll = Length(p0,p1,p2);
		return Leng/LengAll;
	}
	template< typename T >
	float FindTOnLength(const T & p0, const T & p1, const T & p2, float len)
	{
		/*
		float st=0.0f;
		float en=1.0f;
		float sr = (st+en)*0.5f;
		float vst = Length(p0,p1,p2,st);
		float ven = Length(p0,p1,p2,en);
		while(en-st>0.001f)
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
		*/
		//pochodna rownania to tylko: sqrt( c0+t*(c1+c2*t) )
		//wersja z siecznymi
		T A = p0-2*p1+p2;
		T B = 2*p1-2*p0;
		Float c2 = 4*A*A;
		Float c1 = 4*A*B;
		Float c0 = B*B;
		Float tim = 0.0f;
		Float a = sqrt( c0+tim*(c1+c2*tim) );
		Float b = 0.0f;
		tim = len/a;
		Float val=LengthExpr(c0,c1,c2,tim);
		while(fabs(val-len)>0.001)
		{
			a = sqrt( c0+tim*(c1+c2*tim) );
			b = val - (a*tim);
			tim = (len-b)/a;
			val=LengthExpr(c0,c1,c2,tim);
		}
		return tim;
	}
}