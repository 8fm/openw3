/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderDynamicDecalChunk.h"
#include "renderDynamicDecalTarget.h"
#include "renderDynamicDecal.h"
#include "renderProxyDrawable.h"



CRenderDynamicDecalChunk::CRenderDynamicDecalChunk( CRenderDynamicDecal* ownerDecal, IRenderProxyDrawable* target )
	: m_ownerDecal( ownerDecal )
	, m_targetProxy( target )
{
	RED_ASSERT( m_ownerDecal != nullptr && m_targetProxy != nullptr );
	RED_ASSERT( m_targetProxy->ToDynamicDecalTarget() != nullptr, TXT("CRenderDynamicDecalChunk should only be created with a drawable proxy that is convertible to IDynamicDecalTarget") );
}

CRenderDynamicDecalChunk::~CRenderDynamicDecalChunk()
{
	RED_ASSERT( m_registrationRefCount.GetValue() == 0, TXT("Destroying RenderDynamicDecalChunk which has not been unregistered from RenderElementMap") );
}


Float CRenderDynamicDecalChunk::GetTimeToLive() const
{
	if ( m_ownerDecal == nullptr )
	{
		return 0.0f;
	}

	return m_ownerDecal->GetTimeToLive();
}



const Box& CRenderDynamicDecalChunk::GetBoundingBox() const
{
	if ( m_targetProxy == nullptr )
	{
		return Box::EMPTY;
	}

	return m_targetProxy->GetBoundingBox();
}

const Matrix& CRenderDynamicDecalChunk::GetLocalToWorld() const
{
	if ( m_targetProxy == nullptr )
	{
		return Matrix::IDENTITY;
	}

	return m_targetProxy->GetLocalToWorld();
}

const SRenderProxyDrawableClippingEllipseParams* CRenderDynamicDecalChunk::GetClippingEllipseParams() const
{
	if ( m_targetProxy == nullptr )
	{
		return nullptr;
	}

	return m_targetProxy->GetClippingEllipseParams();
}


CRenderShaderTriple* CRenderDynamicDecalChunk::SelectShader( Bool isSkinned, const GpuApi::VertexLayoutRef& decalVertexLayout ) const
{
	if ( decalVertexLayout.isNull() )
	{
		return nullptr;
	}

	const GpuApi::VertexLayoutDesc* layoutDesc = GpuApi::GetVertexLayoutDesc( decalVertexLayout );
	if ( layoutDesc == nullptr )
	{
		return nullptr;
	}


	enum SelectionFlags
	{
		SKINNED		= FLAG(0),
		NORMAL		= FLAG(1),
		CALC_BINORM	= FLAG(2),
		SINGLE_BONE	= FLAG(3),
		ADDITIVE_NORM = FLAG(4),

		ALL = 0xFF						// For simplicity, this is good enough.
	};

	// TODO? If this becomes a bottleneck, the array could be static instead of initialized every draw.
	CRenderShaderTriple* shaders[ALL] = {};
	shaders[0]										= GetRenderer()->m_shaderDynamicDecalStatic;
	shaders[SKINNED]								= GetRenderer()->m_shaderDynamicDecalSkinned;
	shaders[NORMAL]									= GetRenderer()->m_shaderDynamicDecalStaticN;
	shaders[NORMAL|SKINNED]							= GetRenderer()->m_shaderDynamicDecalSkinnedN;
	shaders[CALC_BINORM|NORMAL]						= GetRenderer()->m_shaderDynamicDecalStaticBN;
	shaders[CALC_BINORM|NORMAL|SKINNED]				= GetRenderer()->m_shaderDynamicDecalSkinnedBN;
	shaders[SINGLE_BONE|SKINNED]					= GetRenderer()->m_shaderDynamicDecalSingleSkinned;
	shaders[SINGLE_BONE|NORMAL|SKINNED]				= GetRenderer()->m_shaderDynamicDecalSingleSkinnedN;
	shaders[SINGLE_BONE|CALC_BINORM|NORMAL|SKINNED]	= GetRenderer()->m_shaderDynamicDecalSingleSkinnedBN;

	shaders[ADDITIVE_NORM|NORMAL]									= GetRenderer()->m_shaderDynamicDecalStaticNa;
	shaders[ADDITIVE_NORM|NORMAL|SKINNED]							= GetRenderer()->m_shaderDynamicDecalSkinnedNa;
	shaders[ADDITIVE_NORM|CALC_BINORM|NORMAL]						= GetRenderer()->m_shaderDynamicDecalStaticBNa;
	shaders[ADDITIVE_NORM|CALC_BINORM|NORMAL|SKINNED]				= GetRenderer()->m_shaderDynamicDecalSkinnedBNa;
	shaders[ADDITIVE_NORM|SINGLE_BONE|NORMAL|SKINNED]				= GetRenderer()->m_shaderDynamicDecalSingleSkinnedNa;
	shaders[ADDITIVE_NORM|SINGLE_BONE|CALC_BINORM|NORMAL|SKINNED]	= GetRenderer()->m_shaderDynamicDecalSingleSkinnedBNa;


	Uint32 flags = 0;
	// We can't just use the existence of PS_SkinIndices, because a skinnable mesh might not be actively skinned.
	if ( isSkinned )
	{
		flags |= SKINNED;

		// If the vertex layout has no weights, we have just one bone (apex destruction).
		if ( layoutDesc->GetUsageOffset( GpuApi::VertexPacking::PS_SkinWeights, 0 ) < 0 )
		{
			flags |= SINGLE_BONE;
		}
	}
	if ( m_ownerDecal->HasNormalTexture() )
	{
		flags |= NORMAL;

		// If there's no binormal, assume we need to calculate it from the normal and tangent.
		if ( layoutDesc->GetUsageOffset( GpuApi::VertexPacking::PS_Binormal, 0 ) < 0 )
		{
			flags |= CALC_BINORM;
		}

		if ( m_ownerDecal->HasAdditiveNormals() )
		{
			flags |= ADDITIVE_NORM;
		}
	}

	RED_ASSERT( flags < ALL, TXT("Invalid decal shader index: %d?"), flags );
	return shaders[ flags ];
}


void CRenderDynamicDecalChunk::DestroyDecalChunk()
{
	m_ownerDecal->OnDynamicDecalChunkDestroyed( this );

	if ( m_targetProxy != nullptr && m_targetProxy->ToDynamicDecalTarget() != nullptr )
	{
		m_targetProxy->ToDynamicDecalTarget()->OnDynamicDecalChunkDestroyed( this );
	}

	// At this point all references to us may have been released, so we might not exist anymore!
}
