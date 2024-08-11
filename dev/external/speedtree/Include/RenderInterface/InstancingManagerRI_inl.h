///////////////////////////////////////////////////////////////////////  
//  InstancingManagerRI_inl.h
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
//  SInstancedDrawStats::SInstancedDrawStats

inline SInstancedDrawStats::SInstancedDrawStats( ) :
	m_nNumInstancesDrawn(0),
	m_nNumDrawCalls(0),
	m_nBatchSize(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::CInstancingMgrRI

CInstancingMgrRI_TemplateList
inline CInstancingMgrRI_t::CInstancingMgrRI( ) :
	m_nActiveMgrIndex(0),
	m_bInitialized(false)
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::~CInstancingMgrRI

CInstancingMgrRI_TemplateList
inline CInstancingMgrRI_t::~CInstancingMgrRI( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::Init3dTrees

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::Init3dTrees(st_int32 nNumLods, const CArray<TGeometryBufferClass>& aGeometryBuffers)
{
	if (!aGeometryBuffers.empty( ))
	{
		m_bInitialized = true;
		for (st_int32 i = 0; i < c_nNumInstBuffers; ++i)
			m_bInitialized &= m_atInstanceMgrPolicies[i].Init(SVertexDecl::INSTANCES_3D_TREES, nNumLods, &aGeometryBuffers[0], st_int32(aGeometryBuffers.size( )));

		return m_bInitialized;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::InitGrass

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::InitGrass(const CArray<TGeometryBufferClass>& aGeometryBuffers)
{
	if (!aGeometryBuffers.empty( ))
	{
		m_bInitialized = true;
		for (st_int32 i = 0; i < c_nNumInstBuffers; ++i)
			m_bInitialized &= m_atInstanceMgrPolicies[i].Init(SVertexDecl::INSTANCES_GRASS, 1, &aGeometryBuffers[0], st_int32(aGeometryBuffers.size( )));

		return m_bInitialized;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::InitBillboards

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::InitBillboards(const TGeometryBufferClass* pGeometryBuffer)
{
	if (pGeometryBuffer)
	{
		m_bInitialized = true;
		for (st_int32 i = 0; i < c_nNumInstBuffers; ++i)
			m_bInitialized &= m_atInstanceMgrPolicies[i].Init(SVertexDecl::INSTANCES_BILLBOARDS, 1, pGeometryBuffer, 1);

		return m_bInitialized;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::ReleaseGfxResources

CInstancingMgrRI_TemplateList
inline void CInstancingMgrRI_t::ReleaseGfxResources(void)
{
	for (st_int32 i = 0; i < c_nNumInstBuffers; ++i)
		m_atInstanceMgrPolicies[i].ReleaseGfxResources( );
}

////////////////////////////////////////////////////////////////////////
// ::SphereOutsideViewFrustum

inline st_int32 SphereOutsideViewFrustum( const __m128 &pos, const __m128 &radius, const __m128 * __restrict const frustumPlanes )
{
	const __m128 zeroXYZ = { 0.0f,  0.0f,  0.0f,  1.0f};
	const __m128 zeroW	= { 1.0f,  1.0f,  1.0f,  0.0f};
	const __m128 positionW1	= _mm_add_ps( _mm_mul_ps( pos, zeroW ), zeroXYZ );
	__m128 outDp;

	DOT_PRODUCT( outDp, frustumPlanes[EFrustumPlanes::RIGHT_PLANE], positionW1 );
	if ( _mm_comigt_ss( outDp, radius ) ) return 1;

	DOT_PRODUCT( outDp, frustumPlanes[EFrustumPlanes::LEFT_PLANE], positionW1 );
	if ( _mm_comigt_ss( outDp, radius ) ) return 1;

	DOT_PRODUCT( outDp, frustumPlanes[EFrustumPlanes::NEAR_PLANE], positionW1 );
	if ( _mm_comigt_ss( outDp, radius ) ) return 1;

	DOT_PRODUCT( outDp, frustumPlanes[EFrustumPlanes::BOTTOM_PLANE], positionW1 );
	if ( _mm_comigt_ss( outDp, radius ) ) return 1;

	DOT_PRODUCT( outDp, frustumPlanes[EFrustumPlanes::TOP_PLANE], positionW1 );
	if ( _mm_comigt_ss( outDp, radius ) ) return 1;

	// Fully within the frustum
	return 0;
}

///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::Update3dTreeInstanceBuffers

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::Update3dTreeInstanceBuffers(st_int32 nNumLods, const T3dTreeInstanceLodArray& aInstanceLods/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer)
{
	st_bool bSuccess = true;

	if (!aInstanceLods.empty( ))
	{
		AdvanceMgrIndex( );

        st_int32 nBlockHandle = -1;
        S3dTreeInstanceVertex* pInstances = (S3dTreeInstanceVertex*) CCore::TmpHeapBlockLock(aInstanceLods.size( ) * sizeof(S3dTreeInstanceVertex), "CInstancingMgrRI_t::UpdateGrassInstanceBuffers", nBlockHandle);
        assert(pInstances);

		for (st_int32 nLod = 0; nLod < nNumLods; ++nLod)
		{
			S3dTreeInstanceVertex* pInstanceVertex = pInstances;

            {
                ScopeTrace("InstMgr::MakeInstList");
			    for (st_int32 i = 0; i < st_int32(aInstanceLods.size( )); ++i)
			    {
				    const S3dTreeInstanceLod& sInstanceLod = aInstanceLods.at(i);
					// LAVA++
					//const CInstance* pInstance = sInstanceLod.m_pInstance;
					const CTreeInstance* pInstance = sInstanceLod.m_pInstance;
				    // LAVA--

				    if (sInstanceLod.m_nLodLevel == nLod)
				    {
					    // first float4
					    pInstanceVertex->m_vPos = pInstance->GetPos( );
					    pInstanceVertex->m_fScalar = pInstance->GetScalar( );

					    // second float4
					    pInstanceVertex->m_vUpVector = pInstance->GetUpVector( );
						
						// LAVA++
					    //pInstanceVertex->m_fLodTransition = sInstanceLod.m_fLodTransition;

						// HACK : Store instance fade data in the UserData... reinterpret_cast the pointer itself, instead of just pointing it to some value,
						// so we can avoid cache misses (the fading is stored right with the other instance data).
						size_t userData = reinterpret_cast< size_t >( pInstance->GetUserData() );

						// Fade is stored as a Float, so we need to converts the bits in userData. We could store it as an integer value, and scale it
						// appropriately, but we might get odd results at high or low frame times -- not enough precision for short frames, or long frames
						// result in inaccurate deltas; either way, we could end up with a less smooth transition, or maybe even get stuck.
						st_uint32 fadeBits = ( st_uint32 )( userData & 0xffffffff );
						st_float32 fade = *( st_float32* )( &fadeBits );

						// HACK : We can't really add new data to the vertex buffer inputs, but need to pass in a fade value! So, we'll pack
						// it together with LodTransition. LodTransition is in [0,1], so we shrink it a bit, so it's [0,1). Then we add our
						// fade value as a whole number. The result is that the whole portion is a scaled fade, and fraction is scaled transition.
						// The shader will unpack these, unscale them, and everyone's happy!
						// The fade value is set up so that when the tree is fully visible it is 0. This way, a fully visible tree transitions
						// the same as it would without this fading rubbish. When we fade out, we could potentially lose some precision on both
						// the fade value and the transition, but we're quickly becoming totally invisible so it won't be noticeable.
						st_float32 ifade = floor( fade * 255 );
						if ( ifade < 0 ) ifade = 0;
						if ( ifade > 255 ) ifade = 255;

						pInstanceVertex->m_fLodTransition = ifade + 0.75f*sInstanceLod.m_fLodTransition;
						//LAVA--

					    // third float4
					    pInstanceVertex->m_vRightVector = pInstance->GetRightVector( );
					    pInstanceVertex->m_fLodValue = sInstanceLod.m_fLod;

					    ++pInstanceVertex;
				    }
			    }
            }

            {
                ScopeTrace("InstMgr::CopyInstList");
			    st_int32 nNumVertices = st_int32(pInstanceVertex - pInstances);
			    if (nNumVertices > 0)
				    bSuccess &= m_atInstanceMgrPolicies[m_nActiveMgrIndex].Update(nLod, (st_byte*) pInstances, nNumVertices/*LAVA++*/, instanceRingBuffer);
			    else
				    bSuccess &= m_atInstanceMgrPolicies[m_nActiveMgrIndex].Update(nLod, NULL, 0/*LAVA++*/, instanceRingBuffer);
            }
		}

        CCore::TmpHeapBlockUnlock(nBlockHandle);
	}

	return bSuccess;
}

CInstancingMgrRI_TemplateList
	inline st_bool CInstancingMgrRI_t::PreUpdate3dTreeInstanceBuffers(st_int32 nNumLods, const T3dTreeInstanceLodArray& aInstanceLods)
{
	st_bool bSuccess = true;

	AdvanceMgrIndex( );
	
	Int32* numInstancesPerLod = (Int32*) alloca(nNumLods * sizeof(Int32));
	Red::MemorySet(numInstancesPerLod, 0, nNumLods * sizeof(Int32));


	for (st_int32 i = 0; i < st_int32(aInstanceLods.size( )); ++i)
	{
		const S3dTreeInstanceLod& sInstanceLod = aInstanceLods.at(i);
		numInstancesPerLod[sInstanceLod.m_nLodLevel]++;
	}

	for(int nLod = 0; nLod < nNumLods; nLod++)
	{
		m_atInstanceMgrPolicies[m_nActiveMgrIndex].LockInstanceBufferForWrite(nLod, numInstancesPerLod[nLod]);
		bSuccess = true;
	}

	//Flips AdvanceMgrIndex() back to original index
	AdvanceMgrIndex();

	return bSuccess;
}

CInstancingMgrRI_TemplateList
	inline st_bool CInstancingMgrRI_t::PostUpdate3dTreeInstanceBuffers(st_int32 nNumLods)
{
	st_bool bSuccess = false;
	for(int i=0; i < nNumLods; i++)
	{
		bSuccess = m_atInstanceMgrPolicies[m_nActiveMgrIndex].UnlockInstanceBufferFromWrite(i);
	}

	return bSuccess;
}

///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::UpdateGrassInstanceBuffers

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::UpdateGrassInstanceBuffers(const TRowColCellPtrMap& mCells/*LAVA++*/, const Vec4 * const frustumPlanes, st_float32 cullingRadius, CSpeedTreeInstanceRingBuffer& instanceRingBuffer)
{
	st_bool bSuccess = true;

	AdvanceMgrIndex( );

	// do a quick scan to figure out how large of a buffer we need
	st_int32 nNumGrassInstances = 0;

	// get a pointer to the GPU instance buffer
	const Uint32 initialRingBufferOffset = instanceRingBuffer.m_currentOffset;

	// rebuild a single inst-buffer for all grass instances of this type; some of these cells have been around for a while,
	// some have just been added, but they all contribute to the single inst-buffer
	for (TRowColCellPtrMap::const_iterator i = mCells.begin( ); i != mCells.end( ); ++i)
	{
		const TGrassInstArray& aInstances = i->second->GetGrassInstances( );
		if (!aInstances.empty( ))
		{
			st_int32 outGrassInstances = 0;
			bSuccess = m_atInstanceMgrPolicies[m_nActiveMgrIndex].UpdateWithFrustumTest( 0, aInstances, frustumPlanes, cullingRadius, instanceRingBuffer, outGrassInstances );
			nNumGrassInstances += outGrassInstances;
		}
	}

	m_atInstanceMgrPolicies[m_nActiveMgrIndex].OverwriteOffsetAndCount( 0, initialRingBufferOffset, nNumGrassInstances );

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::UpdateBillboardInstanceBuffers
//
//	This function is marked for deprecation; replaced by GetBillboardVboData() + CopyVboDataToGpu() -- example usage
//	in MyPopulate.cpp in the reference application.

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::UpdateBillboardInstanceBuffers(SBillboardVboBuffer& sBuffer, const CTree* pBaseTree, const TRowColCellPtrMap& aCells)
{
	st_bool bSuccess = true;

	assert(sBuffer.m_pBuffer);
	SBillboardInstanceVertex* pInstanceVertex = sBuffer.m_pBuffer;

	for (TRowColCellPtrMap::const_iterator i = aCells.begin( ); i != aCells.end( ); ++i)
	{
		CCell* cell = i->second;

		// get the instances of pBaseTree from this cell
		const TTreeInstConstPtrArray& aInstances = cell->GetTreeInstances( );
		if (!aInstances.empty( ))
		{
			st_int32 nStart = -1, nEnd = -1;
			SearchInstancesFromClumpedBounds((const CTreeInstance**) &aInstances[0], st_int32(aInstances.size( )), pBaseTree, nStart, nEnd);

			//for (st_int32 i = 0; i < st_int32(aInstances.size( )); ++i)
			for (st_int32 i = nStart; i < nEnd; ++i)
			{
				const CTreeInstance* pInstance = aInstances[i];
                st_assert(pInstance->InstanceOf( ),  "Every instance should know which base tree it's an instance of");

				pInstanceVertex->m_vPos = pInstance->GetPos( );
				pInstanceVertex->m_fScalar = pInstance->GetScalar( );
				pInstanceVertex->m_vUpVector = pInstance->GetUpVector( );
				pInstanceVertex->m_vRightVector = pInstance->GetRightVector( );

				++pInstanceVertex;
			}
		}
		
 		sBuffer.m_nNumInstances = st_int32(pInstanceVertex - sBuffer.m_pBuffer);

	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::CopyInstancesToBillboardInstanceBuffer

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::CopyInstancesToBillboardInstanceBuffer(SBillboardVboBuffer& sBuffer, const CTree* pBaseTree, const TRowColCellPtrMap& mVisibleCells)
{
	st_bool bSuccess = true;

	assert(sBuffer.m_pBuffer);
	SBillboardInstanceVertex* pInstanceVertex = sBuffer.m_pBuffer;

#ifdef SPEEDTREE_FAST_BILLBOARD_STREAMING
		for (TRowColCellPtrMap::const_iterator i = mVisibleCells.begin( ); i != mVisibleCells.end( ); ++i)
		{
			CMap<const CTree*, CArray<SBillboardInstanceVertex> >::const_iterator iFind = i->second->m_mBaseTreesToBillboardVboStreamsMap.find(pBaseTree);
			if (iFind != i->second->m_mBaseTreesToBillboardVboStreamsMap.end( ))
			{
				const size_t c_nNumBillboardsInCell = iFind->second.size( );

				memcpy(pInstanceVertex, &(iFind->second[0]), sizeof(SBillboardInstanceVertex) * c_nNumBillboardsInCell);
				pInstanceVertex += c_nNumBillboardsInCell;
			}
		}
#else
		for (TRowColCellPtrMap::const_iterator i = mVisibleCells.begin( ); i != mVisibleCells.end( ); ++i)
		{
			// get the instances of pBaseTree from this cell
			const TTreeInstConstPtrArray& aInstances = i->second->GetTreeInstances( );
			if (!aInstances.empty( ))
			{
				// the instances are grouped by base tree association; use this function to quickly find the bounds for this base tree
				st_int32 nStart = -1, nEnd = -1;
				SearchInstancesFromClumpedBounds((const CTreeInstance**) &aInstances[0], st_int32(aInstances.size( )), pBaseTree, nStart, nEnd);

				// was it found?
				if (nStart > -1 && nEnd > -1)
				{
					// copy the instances into the buffer
					const CTreeInstance* const* ppInstance = &aInstances[nStart];
					for (st_int32 i = nStart; i < nEnd; ++i)
					{
						pInstanceVertex->m_vPos = (*ppInstance)->GetPos( );
						pInstanceVertex->m_fScalar = (*ppInstance)->GetScalar( );
						pInstanceVertex->m_vUpVector = (*ppInstance)->GetUpVector( );
						(pInstanceVertex++)->m_vRightVector = (*ppInstance++)->GetRightVector( );
					}
				}
			}
		}
#endif

	sBuffer.m_nNumInstances = st_int32(pInstanceVertex - sBuffer.m_pBuffer);
	return bSuccess;
}

///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::CopyVboDataToGpu

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::CopyVboDataToGpu(const SBillboardVboBuffer& sBuffer/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer)
{
	st_bool bSuccess = false;

	if (sBuffer.m_pBuffer)
	{
		AdvanceMgrIndex( );

		bSuccess = m_atInstanceMgrPolicies[m_nActiveMgrIndex].Update(0, (st_byte*) sBuffer.m_pBuffer, sBuffer.m_nNumInstances/*LAVA++*/, instanceRingBuffer);
	}

	return bSuccess;
}

CInstancingMgrRI_TemplateList
inline SpeedTree::st_bool CInstancingMgrRI_t::LAVACopyInstancesToBillboardInstanceBufferDoneRight(const CTree* pBaseTree, const TRowColCellPtrMap& mCells, const Vec4 * const frustumPlanes, st_float32 cullingRadius, CSpeedTreeInstanceRingBuffer& instanceRingBuffer)
{
	const Uint32 initialRingBufferOffset = instanceRingBuffer.m_currentOffset;
	Uint32 numInstancesWritten = 0;

	for (TRowColCellPtrMap::const_iterator i = mCells.begin( ); i != mCells.end( ); ++i)
	{
		CMap<const CTree*, CArray<SBillboardInstanceVertex> >::const_iterator iFind = i->second->m_mBaseTreesToBillboardVboStreamsMap.find(pBaseTree);
		if (iFind != i->second->m_mBaseTreesToBillboardVboStreamsMap.end( ))
		{
			const size_t c_nNumBillboardsInCell = iFind->second.size( );
			if( c_nNumBillboardsInCell > 0 )
			{
				st_int32 outBillboardInstances = 0;
				m_atInstanceMgrPolicies[m_nActiveMgrIndex].UpdateWithFrustumTest( 0, iFind->second, frustumPlanes, cullingRadius, instanceRingBuffer, outBillboardInstances );
				numInstancesWritten += outBillboardInstances;
			}
			/*m_atInstanceMgrPolicies[m_nActiveMgrIndex].Update( 0, (st_byte*)&(iFind->second[0]), (Int32)c_nNumBillboardsInCell, instanceRingBuffer );
			numInstancesWritten += (Uint32)c_nNumBillboardsInCell;*/
		}
	}

	m_atInstanceMgrPolicies[m_nActiveMgrIndex].OverwriteOffsetAndCount( 0, initialRingBufferOffset, numInstancesWritten );

	return true;
}

///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::Render3dTrees

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::Render3dTrees(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer) const
{
	return m_atInstanceMgrPolicies[m_nActiveMgrIndex].Render(nGeometryBufferIndex, nLod, sStats/*LAVA++*/, instanceRingBuffer);
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::RenderGrass

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::RenderGrass(st_int32 nGeometryBufferIndex, SInstancedDrawStats& sStats/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer) const
{
	return m_atInstanceMgrPolicies[m_nActiveMgrIndex].Render(nGeometryBufferIndex, 0, sStats/*LAVA++*/, instanceRingBuffer);
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::RenderBillboards

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::RenderBillboards(SInstancedDrawStats& sStats/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer) const
{
	return m_atInstanceMgrPolicies[m_nActiveMgrIndex].Render(0, 0, sStats/*LAVA++*/, instanceRingBuffer);
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::IsInitialized

CInstancingMgrRI_TemplateList
inline st_bool CInstancingMgrRI_t::IsInitialized(void) const
{
	return m_bInitialized;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::NumInstances

CInstancingMgrRI_TemplateList
inline st_int32 CInstancingMgrRI_t::NumInstances(st_int32 nLod) const
{
	return IsInitialized( ) ? m_atInstanceMgrPolicies[m_nActiveMgrIndex].NumInstances(nLod) : 0;
}


///////////////////////////////////////////////////////////////////////  
//  CInstancingMgrRI::AdvanceMgrIndex

CInstancingMgrRI_TemplateList
inline void CInstancingMgrRI_t::AdvanceMgrIndex(void)
{
	assert(c_nNumInstBuffers > 0);

	m_nActiveMgrIndex = (m_nActiveMgrIndex + 1) % c_nNumInstBuffers;
}
