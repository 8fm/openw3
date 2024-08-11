
#include "build.h"

#include "EngineBezierTools.h"
#include "renderFrame.h"

namespace QuadraticBezierNamespace
{

void EngineBezierTools::DrawSegment2(const QuadraticBezierNamespace::float2 & p0, const QuadraticBezierNamespace::float2 & p1, const QuadraticBezierNamespace::float2 & p2, CRenderFrame* frame, int numsegments)
{
	Vector Start(p0.x,p0.y,0.0f);
	Vector End;
	Color color(255,255,0);
	Float Delta = (1.0f/Float(numsegments));
	Float t=0.0f;
	for(t=Delta;t<=1.0f-Delta;t+=Delta)
	{
		QuadraticBezierNamespace::float2 x = Evaluate(p0,p1,p2,t);
		End.Set3(x.x,x.y,0.0f);
		frame->AddDebugLine(Start,End,color);
		Start.Set3(End);
	}
	QuadraticBezierNamespace::float2 x = Evaluate(p0,p1,p2,1.0f);
	End.Set3(x.x,x.y,0.0f);
	frame->AddDebugLine(Start,End,color);

	Vector a;
	Vector b;
	Vector c;
	a.Set3(p0.x,p0.y,0.0f);
	b.Set3(p1.x,p1.y,0.0f);
	c.Set3(p2.x,p2.y,0.0f);
	Color wirecolor(100,0,0);
	frame->AddDebugLine(a,b,wirecolor);
	frame->AddDebugLine(b,c,wirecolor);
}

void EngineBezierTools::Draw2(Curve2* cur, CRenderFrame* frame, int numsegments)
{
	const Int32 size = cur->m_NumSegments;
	for(Int32 i=0;i<size;i++)
	{
		DrawSegment2( cur->PointA(i),cur->PointB(i),cur->PointC(i),frame,numsegments);
	}
}

void EngineBezierTools::Serialize2( Curve2*& cur, IFile& file )
{
	if( file.IsWriter() )
	{
		ASSERT( cur );
		Uint32 numsegments = cur->m_NumSegments;
		Uint32 numpoints = cur->NumPoints(numsegments);
		file<<numsegments;
		for(Uint32 i=0;i<numpoints;i++)
		{
			Float pointx = cur->m_Segments[i].x;
			Float pointy = cur->m_Segments[i].y;
			file<<pointx;
			file<<pointy;
		}
	}
	else if ( file.IsReader() )
	{
		Uint32 numsegments = 0;
		Uint32 numpoints = 0;
		file<<numsegments;
		ASSERT( numsegments >= 1 );
		if(!cur)
		{
			cur = new Curve2();
		}
		cur->Create(numsegments);
		numpoints = cur->NumPoints(numsegments);
		for(Uint32 i=0;i<numpoints;i++)
		{
			Float pointx = 0.f;
			Float pointy = 0.f;
			file<<pointx;
			file<<pointy;
			cur->m_Segments[i].x = pointx;
			cur->m_Segments[i].y = pointy;
		}
	}
}

}
