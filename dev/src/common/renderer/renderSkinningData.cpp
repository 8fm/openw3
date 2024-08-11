/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderSkinningData.h"
#include "renderSkinManager.h"
#include "../engine/skeletonUtils.h"

// define tells how much Floats per Matrix
#define MATRIXFLOATS 16 

CRenderSkinningData::CRenderSkinningData( Uint32 numMatrices, Bool registerInSkinManager /*=true*/ )
	: m_numMatrices( numMatrices )
	, m_dataBuffer( numMatrices * 3 * MATRIXFLOATS )
	, m_registerInSkinManager( registerInSkinManager )
{
	m_readPtr = m_dataBuffer.TypedData() + 2 * MATRIXFLOATS * m_numMatrices;
	m_writePtr = m_dataBuffer.TypedData();

	for ( Uint32 i=0; i<SDCM_MAX; ++i )
	{
		m_customMatrixIndices[i] = -1;
	}

	ResetCustomHeadData();

	if ( m_registerInSkinManager )
	{
		GetRenderer()->GetSkinManager()->Register(this);
	}
}

CRenderSkinningData::~CRenderSkinningData()
{
	if ( m_registerInSkinManager && GetRenderer() && GetRenderer()->GetSkinManager() )
	{
		GetRenderer()->GetSkinManager()->Unregister(this);
	}
}

void CRenderSkinningData::AdvanceRead()
{
	m_readPtr += MATRIXFLOATS * m_numMatrices;
	if ( m_readPtr == m_dataBuffer.TypedData() + 3 * MATRIXFLOATS * m_numMatrices )
	{
		m_readPtr = m_dataBuffer.TypedData();
	}
}

void CRenderSkinningData::AdvanceWrite()
{
	m_writePtr += MATRIXFLOATS * m_numMatrices;
	if ( m_writePtr == m_dataBuffer.TypedData() + 3 * MATRIXFLOATS * m_numMatrices )
	{
		m_writePtr = m_dataBuffer.TypedData();
	}
}

void CRenderSkinningData::Bind()
{
	if ( m_registerInSkinManager )
	{
		// bind skin texture
		GetRenderer()->GetSkinManager()->BindSkinningBuffer();
		// Setup bone data
		GpuApi::SetVertexShaderConstF( VSC_SkinningData, &(m_bindData.A[0]), 1 );
	}
}

void CRenderSkinningData::SetCustomMatrixIndex( ESkinningDataCustomMatrix type, Int16 index )
{
	ASSERT( -1 == index || (Uint32)index < GetNumMatrices() );
	m_customMatrixIndices[type] = index;
}

Bool CRenderSkinningData::GetCustomMatrix( ESkinningDataCustomMatrix type, Matrix &outMatrix ) const
{
	const Int16 matrixIdx = m_customMatrixIndices[type];
	ASSERT( -1 == matrixIdx || (Uint32)matrixIdx < GetNumMatrices() );
	if ( -1 == matrixIdx || matrixIdx >= (Int32)GetNumMatrices() )
	{
		return false;
	}

	const Float *matrixData = static_cast<const Float *>( GetReadData() ) + matrixIdx * SkeletonBonesUtils::GetMatrixNumFloats( GetMatrixType() );
	SkeletonBonesUtils::CopyMatrix( outMatrix, matrixData, GetMatrixType() );

	// Clear the eventual wetness stuff that is being encoded 
	// in the matrix translation W component
	outMatrix.V[3].W = 1;

	return true;
}

void CRenderSkinningData::SetCustomHeadData( const Vector &pos, const Vector &frontDirection, const Vector &upDirection )
{	
	ASSERT( Abs( frontDirection.Mag3() - 1.f ) <= 0.001f );
	m_customHeadPos = pos;
	m_customHeadFrontDir = frontDirection;
	m_customHeadUpDir = upDirection;
}

void CRenderSkinningData::ResetCustomHeadData()
{	
	m_customHeadPos.Set( 0.f, 0.f, 0.f );
	m_customHeadFrontDir.Set( 0.f, 0.f, 0.f ); //< zeros means that it's not set
	m_customHeadUpDir.Set( 0.f, 0.f, 0.f );
}

Bool CRenderSkinningData::GetCustomHeadData( Vector &outPos, Vector &outFrontDirection, Vector &outUpDirection ) const
{
	if ( m_customHeadFrontDir.IsZero() )
	{
		return false;
	}

	outPos = m_customHeadPos;
	outFrontDirection = m_customHeadFrontDir;
	outUpDirection = m_customHeadUpDir;
	return true;
}

CRenderSkinningDataSimple::CRenderSkinningDataSimple( Uint32 numMatrices )
	: m_numMatrices( numMatrices )
	, m_dataBuffer( numMatrices * 16 )
{
}

CRenderSkinningDataSimple::~CRenderSkinningDataSimple()
{
}

CRenderSkinningDataEngineBuffer::CRenderSkinningDataEngineBuffer( Uint32 numMatrices )
	: m_dataBuffer( numMatrices * 16 )
{
	m_numMatrices = numMatrices;
}

CRenderSkinningDataEngineBuffer::~CRenderSkinningDataEngineBuffer()
{
}
