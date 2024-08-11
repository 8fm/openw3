/*
#pragma once

#include "QuadraticBezier_Segment2.h"

namespace QuadraticBezierNamespace
{
	class QuadraticBezier2
	{
	public:
		enum QuadraticBezier2Initializer { QuadraticBezier2Default };

		QuadraticBezier2();
		QuadraticBezier2( QuadraticBezier2Initializer init );
		~QuadraticBezier2();
		void Create(int NumSegments);
		void Update(float2* ControlPoints, int NumControlPoints);
		inline void Release()
		{
			if(m_Segments){delete [] m_Segments; m_Segments=NULL;}
			m_NumSegments=0;
		}
		float FindNearest(const float2 & x, int & out);
		void FindBoundingBox(float2 & min, float2 & max);
		float2 PointOnLength(float len);
		float2 Velocity(float len);
		void FindPointsAtDistance(const float2 & x, float dist, float2* arr, int & numpoints);
		inline float2 & PointA(int i){return m_Segments[(i*2)+0];}
		inline float2 & PointB(int i){return m_Segments[(i*2)+1];}
		inline float2 & PointC(int i){return m_Segments[(i*2)+2];}
		inline int NumPoints(int NumSegments){return (NumSegments*2)+2;}
		float2* m_Segments;
		int m_NumSegments;
	};
}

*/