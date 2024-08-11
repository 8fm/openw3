/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "apexDebugVisualizer.h"

#ifdef APEX_ENABLE_DEBUG_VISUALIZATION

#include "renderFrame.h"
#include "NxApexRenderContext.h"



CApexDebugVisualizerResourceBase::CApexDebugVisualizerResourceBase( const physx::apex::NxUserRenderResourceDesc& rrDesc )
	: m_desc( rrDesc )
{
	// With debug rendering, it seems there are some simplifications we can make. So, just make sure those hold.
	RED_FATAL_ASSERT( rrDesc.numVertexBuffers == 1, "" );
	RED_FATAL_ASSERT( rrDesc.indexBuffer == nullptr, "" );
	RED_FATAL_ASSERT( rrDesc.boneBuffer == nullptr, "" );
	//RED_FATAL_ASSERT( rrDesc.numSurfaceBuffers == 0, "" ); // apparently it's not set at all, at least not when doing debug vis...
	RED_FATAL_ASSERT( rrDesc.instanceBuffer == nullptr, "" );

	m_vertexBuffer = rrDesc.vertexBuffers[ 0 ];
}

CApexDebugVisualizerResourceBase::~CApexDebugVisualizerResourceBase()
{
}


void CApexDebugVisualizerResourceBase::FillVertices( physx::apex::NxUserRenderVertexBuffer* vb, const physx::apex::NxApexRenderVertexBufferData& data, Uint32 firstVertex, Uint32 numVertices )
{
	// Because we assume there is only one vertex buffer, just need to verify that this is the VB we expect.
	RED_FATAL_ASSERT( vb == m_vertexBuffer, "Getting FillVertices call from unexpected vertex buffer" );
	RED_UNUSED( vb );

	m_vertices.ClearFast();

	const auto& posData = data.getSemanticData( physx::apex::NxRenderVertexSemantic::POSITION );
	const auto& colData = data.getSemanticData( physx::apex::NxRenderVertexSemantic::COLOR );
	const auto& normData = data.getSemanticData( physx::apex::NxRenderVertexSemantic::NORMAL );

	const void* posPtr = posData.data;
	const void* colPtr = colData.data;
	const void* normPtr = normData.data;

	RED_FATAL_ASSERT( posPtr != nullptr, "Position is required" );
	RED_FATAL_ASSERT( posData.format == physx::apex::NxRenderDataFormat::FLOAT3, "Unexpected position format: %d", posData.format );

	m_vertices.ResizeFast( numVertices );

	for ( Uint32 i = 0; i < numVertices; ++i )
	{
		Vector position = *( static_cast< const Vector3* >( posPtr ) );

		Color color = Color::WHITE;
		if ( colPtr != nullptr )
		{
			if ( colData.format == physx::apex::NxRenderDataFormat::B8G8R8A8 )
			{
				const Uint8* srcDataByte = static_cast< const Uint8* >( colPtr );
				color.R = srcDataByte[2];
				color.G = srcDataByte[1];
				color.B = srcDataByte[0];
				color.A = srcDataByte[3];
			}
			else if ( colData.format == physx::apex::NxRenderDataFormat::R8G8B8A8 )
			{
				const Uint8* srcDataByte = static_cast< const Uint8* >( colPtr );
				color.R = srcDataByte[0];
				color.G = srcDataByte[1];
				color.B = srcDataByte[2];
				color.A = srcDataByte[3];
			}
			else
			{
				RED_FATAL( "Unexpected color format: %d", colData.format );
			}
		}

		m_vertices[ i ].Set( position, color );

		posPtr = OffsetPtr( posPtr, posData.stride );

		if ( colPtr != nullptr )
		{
			colPtr = OffsetPtr( colPtr, colData.stride );
		}
		if ( normPtr != nullptr )
		{
			normPtr = OffsetPtr( normPtr, normData.stride );
		}
	}



	// For triangles, we need to reverse the vertex ordering...
	if ( m_desc.primitives == physx::apex::NxRenderPrimitiveType::TRIANGLES )
	{
		for ( Uint32 i = 0; i < numVertices; i += 3 )
		{
			Swap( m_vertices[ i ], m_vertices[ i + 2 ] );
		}
	}
}


void CApexDebugVisualizerResourceBase::Render( CRenderFrame* frame, const physx::apex::NxApexRenderContext& context )
{
	Vector c0( context.local2world.column0.x, context.local2world.column0.y, context.local2world.column0.z, context.local2world.column0.w );
	Vector c1( context.local2world.column1.x, context.local2world.column1.y, context.local2world.column1.z, context.local2world.column1.w );
	Vector c2( context.local2world.column2.x, context.local2world.column2.y, context.local2world.column2.z, context.local2world.column2.w );
	Vector c3( context.local2world.column3.x, context.local2world.column3.y, context.local2world.column3.z, context.local2world.column3.w );
	Matrix localToWorld(c0, c1, c2, c3);

	switch ( m_desc.primitives )
	{
	case physx::apex::NxRenderPrimitiveType::TRIANGLES:
		if ( context.isScreenSpace )
		{
			frame->AddDebugTrianglesOnScreen( m_vertices.TypedData(), m_vertices.Size(), nullptr, 0, Color::WHITE, true );
		}
		else
		{
			frame->AddDebugTriangles( localToWorld, m_vertices.TypedData(), m_vertices.Size(), nullptr, 0, Color::WHITE, false );
		}
		break;

	case physx::apex::NxRenderPrimitiveType::TRIANGLE_STRIP:
		RED_HALT( "Triangle strip not implemented. If this is hitting, maybe we should add it?" );
		break;

	case physx::apex::NxRenderPrimitiveType::LINES:
		if ( context.isScreenSpace )
		{
			frame->AddDebugLinesOnScreen( m_vertices.TypedData(), m_vertices.Size(), true );
		}
		else
		{
			frame->AddDebugLines( localToWorld, m_vertices.TypedData(), m_vertices.Size(), false );
		}
		break;

	case physx::apex::NxRenderPrimitiveType::LINE_STRIP:
		RED_HALT( "Line strip not implemented. If this is hitting, maybe we should add it?" );
		break;

	default:
		RED_HALT( "Unsupported primitive type: %d", m_desc.primitives );
	}

}


void CApexDebugVisualizerResourceBase::setVertexBufferRange( physx::PxU32 firstVertex, physx::PxU32 numVerts )
{
	// Do nothing. We're just drawing what comes in to FillVertices
}

void CApexDebugVisualizerResourceBase::setIndexBufferRange( physx::PxU32 firstIndex, physx::PxU32 numIndices )
{
	RED_HALT( "Not expecting any index data" );
}

void CApexDebugVisualizerResourceBase::setBoneBufferRange( physx::PxU32 firstBone, physx::PxU32 numBones )
{
	RED_HALT( "Not expecting any bone data" );
}

void CApexDebugVisualizerResourceBase::setInstanceBufferRange( physx::PxU32 firstInstance, physx::PxU32 numInstances )
{
	RED_HALT( "Not expecting any instance data" );
}

void CApexDebugVisualizerResourceBase::setSpriteBufferRange( physx::PxU32 firstSprite, physx::PxU32 numSprites )
{
	RED_HALT( "Not expecting any sprite data" );
}

void CApexDebugVisualizerResourceBase::setMaterial( void* material )
{
	// Do nothing. Don't use material, we just use the default debug shader.
}

physx::PxU32 CApexDebugVisualizerResourceBase::getNbVertexBuffers() const
{
	return 1;
}

physx::apex::NxUserRenderVertexBuffer* CApexDebugVisualizerResourceBase::getVertexBuffer( physx::PxU32 index ) const
{
	RED_FATAL_ASSERT( index == 0, "Unexpected VB index: %u", index );
	return m_vertexBuffer;
}

physx::apex::NxUserRenderIndexBuffer* CApexDebugVisualizerResourceBase::getIndexBuffer() const
{
	return nullptr;
}

physx::apex::NxUserRenderBoneBuffer* CApexDebugVisualizerResourceBase::getBoneBuffer() const
{
	return nullptr;
}

physx::apex::NxUserRenderInstanceBuffer* CApexDebugVisualizerResourceBase::getInstanceBuffer() const
{
	return nullptr;
}

physx::apex::NxUserRenderSpriteBuffer* CApexDebugVisualizerResourceBase::getSpriteBuffer() const
{
	return nullptr;
}


//////////////////////////////////////////////////////////////////////////


CApexDebugVisualizerRenderer::CApexDebugVisualizerRenderer( CRenderFrame* frame )
	: m_frame( frame )
{
	RED_FATAL_ASSERT( m_frame != nullptr, "Cannot render Apex Debug Vis without a CRenderFrame" );
}

void CApexDebugVisualizerRenderer::renderResource( const physx::apex::NxApexRenderContext& context )
{
	CApexDebugVisualizerResourceBase* resource = static_cast< CApexDebugVisualizerResourceBase* >( context.renderResource );
	resource->Render( m_frame, context );
}


#endif // APEX_ENABLE_DEBUG_VISUALIZATION
