/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
* Kamil Nowakowski
*/
#include "build.h"
#include "renderDistantLightBatcher.h"

CRenderDistantLightBatcher::CRenderDistantLightBatcher()
	: m_ptr( NULL )
	, m_ptrEnd( NULL )
{
	m_vertexArray.Reserve( 6 * 2048 );
}

CRenderDistantLightBatcher::~CRenderDistantLightBatcher()
{

}

CName CRenderDistantLightBatcher::GetCategory() const
{
	return CNAME( RenderParticleBatcher );
}

void CRenderDistantLightBatcher::PredictedNumberOfLights( Uint32 count )
{
	m_vertexArray.ClearFast();
	m_vertexArray.Grow( count * 6 );

	m_ptr = &m_vertexArray[0];
	m_ptrEnd = m_ptr + count * 6;
}

void CRenderDistantLightBatcher::AddLight( const Vector& position, Float radius , const Vector& color )
{
	RED_ASSERT( m_ptr && m_ptr+6 <= m_ptrEnd , TXT("Out of array while collecting distant light.") );
	GpuApi::SystemVertex_PosColorFUV vertex;
	vertex.x = position.X;
	vertex.y = position.Y;
	vertex.z = position.Z;
	vertex.color[0] = color.X;
	vertex.color[1] = color.Y;
	vertex.color[2] = color.Z;
	vertex.color[3] = color.W;

	vertex.u = -radius; vertex.v = radius;
	m_ptr[0] = vertex;
	m_ptr[5] = vertex;

	vertex.u = radius;
	m_ptr[1] = vertex;

	vertex.v = -radius;
	m_ptr[2] = vertex;
	m_ptr[3] = vertex;

	vertex.u = -radius;
	m_ptr[4] = vertex;

	m_ptr += 6;
}

void CRenderDistantLightBatcher::RenderLights( Float intensityScale )
{
	if( Empty() )
	{
		return;
	}

	GetRenderer()->GetStateManager().SetVertexConst( VSC_Custom_0, Vector ( intensityScale / M_PI, 0, 0, 1 ) );

	GpuApi::DrawSystemPrimitive( GpuApi::PRIMTYPE_TriangleList, m_vertexArray.Size() / 3 , &m_vertexArray[0] );
}