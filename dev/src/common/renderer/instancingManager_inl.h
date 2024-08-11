/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "speedTreeRenderInterface.h"

const Int32 MaxInstanceBatchSize = 50;

inline CInstancingMgrGPUAPI::CInstancingMgrGPUAPI( ) :
	m_pObjectBuffers(NULL),
	m_siSizeOfPerInstanceData(0)

{
	m_asInstances.SetHeapDescription("CInstancingMgrGPUAPI::m_asInstances");
}

inline CInstancingMgrGPUAPI::~CInstancingMgrGPUAPI( )
{
}


inline CInstancingMgrGPUAPI::SInstancesPerLod::SInstancesPerLod( ) :
m_nNumInstances(0),
m_bBufferLocked(false)
{

}

inline st_bool CInstancingMgrGPUAPI::Init(SVertexDecl::EInstanceType eInstanceType, st_int32 nNumLods, const CGeometryBuffer* pGeometryBuffers, st_int32 nNumGeometryBuffers)
{
	st_bool bSuccess = true;

	if (eInstanceType != SVertexDecl::INSTANCES_NONE && pGeometryBuffers && nNumGeometryBuffers > 0)
	{
		const SVertexDecl::SAttribDesc* pInstanceVertexDesc = NULL;

		// determine size of instance vertex based on the type of geometry being instanced
		switch (eInstanceType)
		{
		case SVertexDecl::INSTANCES_GRASS:
			pInstanceVertexDesc = c_asGrassInstanceStreamDesc;
			m_siSizeOfPerInstanceData = sizeof(SGrassInstanceVertex);
			break;
		case SVertexDecl::INSTANCES_BILLBOARDS:
			pInstanceVertexDesc = c_asBillboardInstanceStreamDesc;
			m_siSizeOfPerInstanceData = sizeof(SBillboardInstanceVertex);
			break;
		default: // INSTANCES_3D_TREE
			pInstanceVertexDesc = c_as3dTreeInstanceStreamDesc;
			m_siSizeOfPerInstanceData = sizeof(S3dTreeInstanceVertex);
		}

		SVertexDecl sInstanceDecl;
		sInstanceDecl.Set(pInstanceVertexDesc);

		// retain geometry buffers ptr, we'll have to call Bind() on them in Render()
		m_pObjectBuffers = pGeometryBuffers;

		// set size of per-instance array based on number of LOD levels
		m_asInstances.resize(nNumLods);
	}

	return bSuccess;
}


inline void CInstancingMgrGPUAPI::ReleaseGfxResources()
{
}


inline st_bool CInstancingMgrGPUAPI::Update(st_int32 nLod, const st_byte* pInstanceData, st_int32 nNumInstances, CSpeedTreeInstanceRingBuffer& instanceRingBuffer)
{
	assert(nLod > -1);
	assert(nLod < st_int32(m_asInstances.size( )));
	assert(m_siSizeOfPerInstanceData > 0);

	st_bool bSuccess = true;

	m_asInstances[nLod].m_nNumInstances = nNumInstances;
	if (pInstanceData && nNumInstances > 0)
	{
		m_asInstances[nLod].m_instanceBufferOffset = instanceRingBuffer.m_currentOffset;
		bSuccess = instanceRingBuffer.AppendData( (Uint8*)pInstanceData, nNumInstances * m_siSizeOfPerInstanceData );
	}

	return bSuccess;
}

inline st_bool CInstancingMgrGPUAPI::UpdateWithFrustumTest(st_int32 nLod, const TGrassInstArray& aInstances, const Vec4 * const frustumPlanes, st_float32 cullingRadius, CSpeedTreeInstanceRingBuffer& instanceRingBuffer, st_int32& outNumInstances )
{
	assert(nLod > -1);
	assert(nLod < st_int32(m_asInstances.size( )));
	assert(m_siSizeOfPerInstanceData > 0);

	st_bool bSuccess = true;
	st_int32 nNumInstances = 0;
	st_int32 nInstancesCount = st_int32(aInstances.size());

	if (nInstancesCount > 0)
	{
		// Validate possible instances amount based on the ring buffer current offset
		if( instanceRingBuffer.m_currentOffset + nInstancesCount * m_siSizeOfPerInstanceData > instanceRingBuffer.m_size )
		{
			RED_HALT( "Exceeded the size of the SpeedTree instancing ring buffer. Some foliage won't render." );
			nInstancesCount = (instanceRingBuffer.m_size - instanceRingBuffer.m_currentOffset) / st_int32(m_siSizeOfPerInstanceData);
			if( nInstancesCount == 0 ){ outNumInstances = 0; return false; }
		}
		
		m_asInstances[nLod].m_instanceBufferOffset = instanceRingBuffer.m_currentOffset;

		const __m128 negate = {-1.0f, -1.0f, -1.0f, -1.0f};
		__m128 sseFustrumPlanes[6];

		sseFustrumPlanes[0] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[0]) );
		sseFustrumPlanes[1] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[1]) );
		sseFustrumPlanes[2] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[2]) );
		sseFustrumPlanes[3] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[3]) );
		sseFustrumPlanes[4] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[4]) );
		sseFustrumPlanes[5] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[5]) );

		__m128 radius = _mm_load_ss( &cullingRadius );

		for (st_int32 i = 0; i < nInstancesCount; ++i)
		{
			const SGrassInstance& sInstance = aInstances[i];
			__m128 pos = _mm_loadu_ps( (const float *)&sInstance.GetPos( ) );

			if( SphereOutsideViewFrustum( pos, radius, sseFustrumPlanes ))
				continue;

			SGrassInstance* grassInst = NULL;
			bSuccess = instanceRingBuffer.GetCurrentInstancePtr( (Uint8*&)grassInst, m_siSizeOfPerInstanceData );
			grassInst->m_fLodTransition = sInstance.m_fLodTransition;
			grassInst->m_fLodValue = sInstance.m_fLodValue;
			grassInst->m_fScalar = sInstance.m_fScalar;
			grassInst->m_vPos = sInstance.m_vPos;
			grassInst->m_vRight = sInstance.m_vRight;
			grassInst->m_vUp = sInstance.m_vUp;

			++nNumInstances;
		}
	}

	m_asInstances[nLod].m_nNumInstances = nNumInstances;
	outNumInstances = nNumInstances;

	return bSuccess;
}

inline st_bool CInstancingMgrGPUAPI::UpdateWithFrustumTest(st_int32 nLod, const CArray<SBillboardInstanceVertex>& aInstances, const Vec4 * const frustumPlanes, st_float32 cullingRadius, CSpeedTreeInstanceRingBuffer& instanceRingBuffer, st_int32& outNumInstances )
{
	assert(nLod > -1);
	assert(nLod < st_int32(m_asInstances.size( )));
	assert(m_siSizeOfPerInstanceData > 0);

	st_bool bSuccess = true;
	st_int32 nNumInstances = 0;
	st_int32 nInstancesCount = st_int32(aInstances.size());

	if (nInstancesCount > 0)
	{
		// Validate possible instances amount based on the ring buffer current offset
		if( instanceRingBuffer.m_currentOffset + nInstancesCount * m_siSizeOfPerInstanceData > instanceRingBuffer.m_size )
		{
			RED_HALT( "Exceeded the size of the SpeedTree instancing ring buffer. Some foliage won't render." );
			nInstancesCount = (instanceRingBuffer.m_size - instanceRingBuffer.m_currentOffset) / st_int32(m_siSizeOfPerInstanceData);
			if( nInstancesCount == 0 ){ outNumInstances = 0; return false; }
		}

		m_asInstances[nLod].m_instanceBufferOffset = instanceRingBuffer.m_currentOffset;

		const __m128 negate = {-1.0f, -1.0f, -1.0f, -1.0f};
		__m128 sseFustrumPlanes[6];

		sseFustrumPlanes[0] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[0]) );
		sseFustrumPlanes[1] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[1]) );
		sseFustrumPlanes[2] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[2]) );
		sseFustrumPlanes[3] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[3]) );
		sseFustrumPlanes[4] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[4]) );
		sseFustrumPlanes[5] = _mm_sub_ps( _mm_setzero_ps(), _mm_loadu_ps(frustumPlanes[5]) );

		for (st_int32 i = 0; i < nInstancesCount; ++i)
		{
			const SBillboardInstanceVertex& sInstance = aInstances[i];

			__m128 pos = _mm_loadu_ps( (const float *)&sInstance.m_vPos );
			__m128 radius = _mm_load_ss( &cullingRadius );

			if( SphereOutsideViewFrustum( pos, radius, sseFustrumPlanes ))
				continue;

			SBillboardInstanceVertex* billboardInst = NULL;
			bSuccess = instanceRingBuffer.GetCurrentInstancePtr( (Uint8*&)billboardInst, m_siSizeOfPerInstanceData );
			billboardInst->m_vPos = sInstance.m_vPos;
			billboardInst->m_fScalar = sInstance.m_fScalar;
			billboardInst->m_vUpVector = sInstance.m_vUpVector;
			billboardInst->m_vRightVector = sInstance.m_vRightVector;
			billboardInst->m_fPad0 = sInstance.m_fPad0;
			billboardInst->m_fPad1 = sInstance.m_fPad1;

			++nNumInstances;
		}
	}

	m_asInstances[nLod].m_nNumInstances = nNumInstances;
	outNumInstances = nNumInstances;

	return bSuccess;
}

inline void CInstancingMgrGPUAPI::OverwriteOffsetAndCount( Uint32 lod, Uint32 offset, Uint32 instanceCount )
{
	assert(lod < st_int32(m_asInstances.size( )));
	m_asInstances[lod].m_instanceBufferOffset = offset;
	m_asInstances[lod].m_nNumInstances = instanceCount;
}


inline st_bool CInstancingMgrGPUAPI::Render(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer) const
{
	assert(m_pObjectBuffers);
	assert(m_siSizeOfPerInstanceData > 0);
	assert(nLod < st_int32(m_asInstances.size( )));

	st_bool bSuccess = false;

	// set up a few aliases
	const SInstancesPerLod& sInstanceData = m_asInstances[nLod];
	const CGeometryBuffer* pObjectBuffer = m_pObjectBuffers + nGeometryBufferIndex;
	assert(pObjectBuffer);

	// check for early exit conditions
	if (pObjectBuffer->NumIndices( ) == 0 || sInstanceData.m_nNumInstances == 0)
		return true;

	// flush vertex format
	GpuApi::SetVertexFormatRaw( GpuApi::BCT_VertexSystemPos, false );

	if( pObjectBuffer->EnableFormat() )
	{
		if( pObjectBuffer->BindIndexBuffer() )
		{
			bSuccess = true;
			pObjectBuffer->BindVertexBuffer( 0 );

			instanceRingBuffer.Bind( 1, sInstanceData.m_instanceBufferOffset, (Uint32)m_siSizeOfPerInstanceData );
			
			bSuccess &= pObjectBuffer->RenderIndexedInstanced(PRIMITIVE_TRIANGLES, 0, pObjectBuffer->NumIndices( ), sInstanceData.m_nNumInstances );
		
			sStats.m_nNumDrawCalls++;
			sStats.m_nNumInstancesDrawn += sInstanceData.m_nNumInstances;
			sStats.m_nBatchSize = sInstanceData.m_nNumInstances;
		}
	}

	return bSuccess;
}

inline st_int32 CInstancingMgrGPUAPI::NumInstances(st_int32 nLod) const
{
	return m_asInstances[nLod].m_nNumInstances;
}

