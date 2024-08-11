///////////////////////////////////////////////////////////////////////
//  InstancingMgr.inl
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Constants

const st_int32 c_nInitialInstanceBatchSize = 1000;


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX11::CInstancingMgrDirectX11

inline CInstancingMgrDirectX11::CInstancingMgrDirectX11( ) :
	m_pObjectBuffers(NULL),
	m_siSizeOfInstanceData(0)
{
	m_aInstances.SetHeapDescription("CInstancingMgrDirectX11::m_aInstances");
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX11::~CInstancingMgrDirectX11

inline CInstancingMgrDirectX11::~CInstancingMgrDirectX11( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX11::SInstancesPerLod::SInstancesPerLod

inline CInstancingMgrDirectX11::SInstancesPerLod::SInstancesPerLod( ) :
	m_nNumInstances(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX11::Init

inline st_bool CInstancingMgrDirectX11::Init(SVertexDecl::EInstanceType eInstanceType, 
											 st_int32 nNumLods, 
											 const CGeometryBuffer* pGeometryBuffers, 
											 st_int32 nNumGeometryBuffers)
{
	st_bool bSuccess = false;

	if (eInstanceType != SVertexDecl::INSTANCES_NONE && pGeometryBuffers && nNumGeometryBuffers > 0)
	{
		// determine size of instance vertex based on the type of geometry being instanced
		const SVertexDecl::SAttribDesc* pInstanceVertexDesc = NULL;
		switch (eInstanceType)
		{
		case SVertexDecl::INSTANCES_GRASS:
			pInstanceVertexDesc = c_asGrassInstanceStreamDesc;
			m_siSizeOfInstanceData = sizeof(SGrassInstanceVertex);
			break;
		case SVertexDecl::INSTANCES_BILLBOARDS:
			pInstanceVertexDesc = c_asBillboardInstanceStreamDesc;
			m_siSizeOfInstanceData = sizeof(SBillboardInstanceVertex);
			break;
		default: // SVertexDecl::INSTANCES_3D_TREE
			pInstanceVertexDesc = c_as3dTreeInstanceStreamDesc;
			m_siSizeOfInstanceData = sizeof(S3dTreeInstanceVertex);
		}
		SVertexDecl sInstanceDecl;
		sInstanceDecl.Set(pInstanceVertexDesc);

		// retain geometry buffers ptr, we'll have to call Bind() on them in Render()
		m_pObjectBuffers = pGeometryBuffers;

		// set size of per-instance array based on number of LOD levels
		m_aInstances.resize(nNumLods);

		// initialize instance buffers, one per LOD
		for (st_int32 nLod = 0; nLod < nNumLods; ++nLod)
		{
			CGeometryBuffer& cInstanceBuffer = m_aInstances[nLod].m_cInstanceBuffer;
			if (cInstanceBuffer.SetVertexDecl(sInstanceDecl, NULL))
			{
				bSuccess = cInstanceBuffer.CreateUninitializedVertexBuffer(c_nInitialInstanceBatchSize);
			}
			else
			{
				CCore::SetError("CInstancingMgrDirectX11::Init, SetVertexDecl() failed");
				bSuccess = false;
			}
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX11::ReleaseGfxResources

inline void CInstancingMgrDirectX11::ReleaseGfxResources(void)
{
	for (size_t i = 0; i < m_aInstances.size( ); ++i)
		m_aInstances[i].m_cInstanceBuffer.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX11::Update

inline st_bool CInstancingMgrDirectX11::Update(st_int32 nLod, const st_byte* pInstanceData, st_int32 nNumInstances)
{
	assert(nLod > -1);
	assert(nLod < st_int32(m_aInstances.size( )));
	assert(m_siSizeOfInstanceData > 0);

	st_bool bSuccess = true;

	m_aInstances[nLod].m_nNumInstances = nNumInstances;
	if (pInstanceData && nNumInstances > 0)
	{
		bSuccess = m_aInstances[nLod].m_cInstanceBuffer.OverwriteVertices(pInstanceData, nNumInstances, 0, c_nInstanceVertexStream);
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX11::Render

inline st_bool CInstancingMgrDirectX11::Render(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats) const
{
	assert(m_pObjectBuffers);
	assert(m_siSizeOfInstanceData > 0);
	assert(nLod < st_int32(m_aInstances.size( )));

	st_bool bSuccess = true;

	// set up a few aliases
	const SInstancesPerLod& sInstanceData = m_aInstances[nLod];
	const CGeometryBuffer& cInstanceBuffer = sInstanceData.m_cInstanceBuffer;
	const CGeometryBuffer* pObjectBuffer = m_pObjectBuffers + nGeometryBufferIndex;
	assert(pObjectBuffer);

	// check for early exit conditions
	if (pObjectBuffer->NumIndices( ) == 0 || sInstanceData.m_nNumInstances == 0)
		return true;

	// bind composite vertex layout
	if (pObjectBuffer->EnableFormat( ))
	{
		if (pObjectBuffer->BindIndexBuffer( ))
		{
			// bind both object & instance buffers
			UINT aStrides[2] = { pObjectBuffer->GetVertexDecl( ).m_uiVertexSize, cInstanceBuffer.GetVertexDecl( ).m_uiVertexSize };
			UINT aOffsets[2] = { 0, 0 };
			ID3D11Buffer* aBuffers[2] = { pObjectBuffer->m_tGeometryBufferPolicy.VertexBuffer( ), cInstanceBuffer.m_tGeometryBufferPolicy.VertexBuffer( ) };
			DX11::DeviceContext( )->IASetVertexBuffers(0, 2, aBuffers, aStrides, aOffsets);

			// render instances with one call
			bSuccess &= pObjectBuffer->RenderIndexedInstanced(PRIMITIVE_TRIANGLES, 0, pObjectBuffer->NumIndices( ), sInstanceData.m_nNumInstances);

			// update stats
			sStats.m_nNumDrawCalls++;
			sStats.m_nNumInstancesDrawn += sInstanceData.m_nNumInstances;
			sStats.m_nBatchSize = sInstanceData.m_nNumInstances;
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrDirectX11::NumInstances

inline st_int32 CInstancingMgrDirectX11::NumInstances(st_int32 nLod) const
{
	return m_aInstances[nLod].m_nNumInstances;
}

