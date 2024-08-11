
#pragma once

#include "QuadraticBezier_Knots.h"
#include "QuadraticBezier_Segment.h"

namespace QuadraticBezierNamespace
{
	template<typename T>
	class Curve
	{
	public:
		Curve()
		{
			m_Segments=NULL;
			m_NumSegments=0;		
		}
		~Curve()
		{
			Release();
		}
		inline void Release()
		{
			if(m_Segments){delete [] m_Segments; m_Segments=NULL;}
			m_NumSegments=0;
		}
		void Create(int NumSegments)
		{
			Release();
			m_Segments = new T[NumPoints(NumSegments)];
			m_NumSegments = NumSegments;		
		}
		void Update(T* ControlPoints, int NumControlPoints)
		{
			int NumSegments = NumControlPoints-2;
			if(NumSegments<=0){Release();return;}
			if( m_NumSegments != NumSegments )
			{
				Create(NumSegments);
			}
			for(int i=0;i<m_NumSegments;i++)
			{
				T cp0 = ControlPoints[(i)+0];
				T cp1 = ControlPoints[(i)+1];
				T cp2 = ControlPoints[(i)+2];
				T p0 = (cp0+cp1)*0.5;
				T p1 = cp1;
				T p2 = (cp1+cp2)*0.5;
				PointA(i) = p0;
				PointB(i) = p1;
				PointC(i) = p2;
			}			
		}
		T EvaluateSegment(int seg, float t)
		{
			T p0 = PointA(seg);
			T p1 = PointB(seg);
			T p2 = PointC(seg);
			return Evaluate(p0,p1,p2,t);
		}
		float FindNearest(const T & x, int & out)
		{
			float t=0.f;
			float minv = 9999999999999999999999999.f;
			int ind=0;
			float win=0.f;
			for(int i=0;i<m_NumSegments;i++)
			{
				T p0 = PointA(i);
				T p1 = PointB(i);
				T p2 = PointC(i);
				t = NearestPoint(p0,p1,p2,x);
				T min = (Evaluate(p0,p1,p2,t)-x);
				float m = length(min);
				if(m<minv){minv=m;ind=i; win=t;}
			}
			out=ind;
			return win;
		}
		T PointOnLength(float len)
		{
			float sumall=0.0;
			for(int i=0;i<m_NumSegments;i++)
			{
				T p0 = PointA(i);
				T p1 = PointB(i);
				T p2 = PointC(i);
				float slen=Length(p0,p1,p2);
				if(sumall+slen>len)
				{
					float t = len-sumall;
					float nt = FindTOnLength<float3>(p0,p1,p2,t);
					return Evaluate(p0,p1,p2,nt);
				}
				else
				{
					sumall+=slen;	
				}
			}
			return T();		
		}

		T VelocityOnLength(float len)
		{
			float sumall=0.0;
			for(int i=0;i<m_NumSegments;i++)
			{
				T p0 = PointA(i);
				T p1 = PointB(i);
				T p2 = PointC(i);
				float slen=Length(p0,p1,p2);
				if(sumall+slen>len)
				{
					float t = len-sumall;
					float nt = FindTOnLength<float3>(p0,p1,p2,t);
					return Velocity(p0,p1,p2,nt);
				}
				else
				{
					sumall+=slen;	
				}
			}
			return T();		
		}

		void FindPointsAtDistance(const T & x, float dist, T* arr, int & numpoints)
		{
			numpoints=0;
			float roots[4];
			int numroots=0;
			for(int i=0;i<m_NumSegments;i++)
			{
				T p0 = PointA(i);
				T p1 = PointB(i);
				T p2 = PointC(i);
				PointsOnSplineAtDistance(p0,p1,p2,x,dist,(float*)&roots, numroots);
				for(int j=0;j<numroots;j++)
				{
					if(roots[j]>=0 && roots[j]<=1)
					{
						T p = Evaluate(p0,p1,p2,roots[j]);
						arr[numpoints]=p;
						numpoints++;
					}
				}
			}
		}
		Float CalculateLength()
		{
			Float sum=0.0f;
			for(int i=0;i<m_NumSegments;i++)
			{
				T p0 = PointA(i);
				T p1 = PointB(i);
				T p2 = PointC(i);
				sum += Length(p0,p1,p2);
			}
			return sum;
		}

		inline T & PointA(int i){return m_Segments[(i*2)+0];}
		inline T & PointB(int i){return m_Segments[(i*2)+1];}
		inline T & PointC(int i){return m_Segments[(i*2)+2];}
		inline int NumPoints(int NumSegments){return (NumSegments*2)+2;}
		T* m_Segments;
		int m_NumSegments;	
	};

	typedef Curve<QuadraticBezierNamespace::float2> Curve2;
	typedef Curve<QuadraticBezierNamespace::float3> Curve3;
}