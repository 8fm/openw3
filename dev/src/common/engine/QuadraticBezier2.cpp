
#include "build.h"
/*
#include "QuadraticBezier2.h"

namespace QuadraticBezierNamespace
{

		QuadraticBezier2::QuadraticBezier2()
		{
			m_Segments=NULL;
			m_NumSegments=0;
		}
		QuadraticBezier2::~QuadraticBezier2()
		{
			Release();
		}
		void QuadraticBezier2::Create(int NumSegments)
		{
			Release();
			m_Segments = new float2[NumPoints(NumSegments)];
			m_NumSegments = NumSegments;		
		}
		void QuadraticBezier2::Update(float2* ControlPoints, int NumControlPoints)
		{
			int NumSegments = NumControlPoints-2;
			if(NumSegments<=0){Release();return;}
			if( m_NumSegments != NumSegments )
			{
				Create(NumSegments);
			}
			for(int i=0;i<m_NumSegments;i++)
			{
				float2 cp0 = ControlPoints[(i)+0];
				float2 cp1 = ControlPoints[(i)+1];
				float2 cp2 = ControlPoints[(i)+2];
				float2 p0 = (cp0+cp1)*0.5;
				float2 p1 = cp1;
				float2 p2 = (cp1+cp2)*0.5;
				PointA(i) = p0;
				PointB(i) = p1;
				PointC(i) = p2;
			}		
		}
		float QuadraticBezier2::FindNearest(const float2 & x, int & out)
		{
			float t=0.f;
			float minv = 9999999999999999999999999.f;
			int ind=0;
			float win=0.f;
			for(int i=0;i<m_NumSegments;i++)
			{
				float2 p0 = PointA(i);
				float2 p1 = PointB(i);
				float2 p2 = PointC(i);
				t = QuadraticBezier_Segment2::NearestPoint(p0,p1,p2,x);
				float2 min = (QuadraticBezier_Segment2::Evaluate(p0,p1,p2,t)-x);
				float m = min.x*min.x+min.y*min.y;
				if(m<minv){minv=m;ind=i; win=t;}
			}
			out=ind;
			return win;
		}
		void QuadraticBezier2::FindBoundingBox(float2 & min, float2 & max)
		{
			for(int i=0;i<m_NumSegments;i++)
			{
				if(i==0)
				{
					float2 p0 = PointA(0);
					float2 p1 = PointB(0);
					float2 p2 = PointC(0);
					float2 smin;
					float2 smax;
					QuadraticBezier_Segment2::FindBoundingBox(p0,p1,p2,smin,smax);
					min=smin;
					max=smax;
				}
				else
				{
					float2 p0 = PointA(i);
					float2 p1 = PointB(i);
					float2 p2 = PointC(i);
					float2 smin;
					float2 smax;
					QuadraticBezier_Segment2::FindBoundingBox(p0,p1,p2,smin,smax);
					if(smin.x<min.x){min.x=smin.x;}
					if(smin.y<min.y){min.y=smin.y;}
					if(smax.x>max.x){max.x=smax.x;}
					if(smax.y>max.y){max.y=smax.y;}
				}
			}
		}
		float2 QuadraticBezier2::PointOnLength(float len)
		{
			float sumall=0.0;
			for(int i=0;i<m_NumSegments;i++)
			{
				float2 p0 = PointA(i);
				float2 p1 = PointB(i);
				float2 p2 = PointC(i);
				float slen=QuadraticBezier_Segment2::Length(p0,p1,p2);
				if(sumall+slen>len)
				{
					float t = len-sumall;
					float nt = QuadraticBezier_Segment2::FindTOnLength(p0,p1,p2,t);
					return QuadraticBezier_Segment2::Evaluate(p0,p1,p2,nt);
				}
				else
				{
					sumall+=slen;	
				}
			}
			return float2(0,0);		
		}
		float2 QuadraticBezier2::Velocity(float len)
		{
			float sumall=0.0;
			for(int i=0;i<m_NumSegments;i++)
			{
				float2 p0 = PointA(i);
				float2 p1 = PointB(i);
				float2 p2 = PointC(i);
				float slen=QuadraticBezier_Segment2::Length(p0,p1,p2);
				if(sumall+slen>len)
				{
					float t = len-sumall;
					float nt = QuadraticBezier_Segment2::FindTOnLength(p0,p1,p2,t);
					return QuadraticBezier_Segment2::Velocity(p0,p1,p2,nt);
				}
				else
				{
					sumall+=slen;	
				}
			}
			return float2(1,0);
		}
		void QuadraticBezier2::FindPointsAtDistance(const float2 & x, float dist, float2* arr, int & numpoints)
		{
			numpoints=0;
			float roots[4];
			int numroots=0;
			for(int i=0;i<m_NumSegments;i++)
			{
				float2 p0 = PointA(i);
				float2 p1 = PointB(i);
				float2 p2 = PointC(i);
				QuadraticBezier_Segment2::PointsOnSplineAtDistance(p0,p1,p2,x,dist,(float*)&roots, numroots);
				for(int j=0;j<numroots;j++)
				{
					if(roots[j]>=0 && roots[j]<=1)
					{
						float2 p = QuadraticBezier_Segment2::Evaluate(p0,p1,p2,roots[j]);
						arr[numpoints]=p;
						numpoints++;
					}
				}
			}
		}

}

*/