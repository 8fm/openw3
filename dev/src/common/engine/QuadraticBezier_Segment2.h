/*
#pragma once
#include "Bezier_Polynomials.h"
#include "QuadBezier_Math2.h"

namespace QuadraticBezierNamespace
{
class QuadraticBezier_Segment2
{
public:
		static float2 Evaluate(const float2 & p0, const float2 & p1, const float2 & p2, float t);
		static float2 EvaluateOffset(const float2 & p0, const float2 & p1, const float2 & p2, float t, float offset);
		static float2 Velocity(const float2 & p0, const float2 & p1, const float2 & p2, float t);
		static float NearestPoint(const float2 & p0, const float2 & p1, const float2 & p2, const float2 & x);
		static void PointsOnSplineAtDistance(const float2 & p0, const float2 & p1, const float2 & p2, const float2 & xx, float z, float* arr, int & numpoints);
		static float Length(const float2 & p0, const float2 & p1, const float2 & p2, float t = 1.0f);
		static float LengthNormalized(const float2 & p0, const float2 & p1, const float2 & p2, float t);
		static float FindTOnLength(const float2 & p0, const float2 & p1, const float2 & p2, float len);
		static void FindBoundingBox(const float2 & p0, const float2 & p1, const float2 & p2, float2 & min, float2 & max);
		static float Len(float z, float a, float b, float c, float d);	
};
}
*/