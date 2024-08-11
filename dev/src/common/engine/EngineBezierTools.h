
#pragma once
#include "QuadraticBezier_Curve.h"

class CRenderFrame;

namespace QuadraticBezierNamespace
{
class EngineBezierTools
{
public:
	static void DrawSegment2(const QuadraticBezierNamespace::float2 & p00, const QuadraticBezierNamespace::float2 & p11, const QuadraticBezierNamespace::float2 & p22, CRenderFrame* frame, int numsegments=10);
	static void Draw2(Curve2* cur, CRenderFrame* frame, int numsegments=10);
	static void Serialize2( Curve2*& cur, IFile& file );
};

}