///////////////////////////////////////////////////////////////////////  
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

// LAVA++
CTreeRI_TemplateList
	inline CTreeRI_t::SCachedUserData::SCachedUserData ()
{
	Reset();
}

CTreeRI_TemplateList
	inline void CTreeRI_t::SCachedUserData::Init( st_int32 numStrings, const char * const *strings )
{
	Reset();

	for ( st_int32 i=0; i<numStrings; ++i )
	{
		const char *str = strings[i];
		if ( !str )
		{
			continue;
		}

		if ( strstr( str, "BillboardRandomOff" ) )		m_billboardColorType = BCT_None;
		if ( strstr( str, "BillboardRandomGrass" ) )	m_billboardColorType = BCT_Grass;
		if ( strstr( str, "BillboardRandomBranch" ) )	m_billboardColorType = BCT_Branches;
		if ( strstr( str, "BillboardRandomTrees" ) )	m_billboardColorType = BCT_Trees;

		if ( strstr( str, "RandomOn" ) )		m_grassRandomColorEnabled = true;
		if ( strstr( str, "RandomOff" ) )		m_grassRandomColorEnabled = false;

		if ( strstr( str, "TerrainNormalsOn" ) )		m_grassTerrainNormalsEnabled = true;
		if ( strstr( str, "TerrainNormalsOff" ) )		m_grassTerrainNormalsEnabled = false;

		if ( strstr( str, "PigmentFloodOn" ) )		m_grassFloodPigmentEnabled = true;
		if ( strstr( str, "PigmentFloodOff" ) )		m_grassFloodPigmentEnabled = false;

		if ( strstr( str, "PigmentOn" ) )		m_grassPigmentEnabled = true;
		if ( strstr( str, "PigmentOff" ) )		m_grassPigmentEnabled = false;

		if ( strstr( str, "InteractiveOn" ) )	m_interactiveEnabled = true;
		if ( strstr( str, "InteractiveOff" ) )	m_interactiveEnabled = false;

		//if ( strstr( str, "EnvMaterialSettingsOn" ) )		m_allowGlobalMaterialSettings = true;
		//if ( strstr( str, "EnvMaterialSettingsOff" ) )		m_allowGlobalMaterialSettings = false;
	}
}

CTreeRI_TemplateList
	inline void CTreeRI_t::SCachedUserData::Reset()
{
	m_billboardColorType = BCT_DEFAULT;
	m_grassRandomColorEnabled = true;
	m_grassTerrainNormalsEnabled = false;
	m_grassFloodPigmentEnabled = true;
	m_grassPigmentEnabled = true;
	m_interactiveEnabled = false;
	//m_allowGlobalMaterialSettings = true;
}
// LAVA--

///////////////////////////////////////////////////////////////////////
//  CTreeRI::CTreeRI

CTreeRI_TemplateList
inline CTreeRI_t::CTreeRI( ) :
	m_nMaxNumDrawCallsPerLod(0),
	m_tBillboardGeometryBuffer(false, false, "billboard shape"),
	m_pSearchPaths(NULL),
	// LAVA++
	m_bGraphicsInitialized(false),
	m_isDirty( true )
	// LAVA--
{
	for (st_int32 i = 0; i < RENDER_PASS_COUNT; ++i)
		m_aa3dRenderStates[i].SetHeapDescription("CTreeRI::m_aa3dRenderStates");
	m_atGeometryBuffers.SetHeapDescription("CTreeRI:m_atGeometryBuffers");
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::~CTreeRI

CTreeRI_TemplateList
inline CTreeRI_t::~CTreeRI( )
{
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::InitGfx

CTreeRI_TemplateList
inline st_bool CTreeRI_t::InitGfx(const SAppState& sAppState,
								  const CArray<CFixedString>& aSearchPaths, 
								  st_int32 nMaxAnisotropy,
								  st_float32 fTextureAlphaScalar)
{
	if (!m_bGraphicsInitialized)
	{
		// LAVA++
		const char *strings[USER_STRING_COUNT];
		for ( st_int32 i=0; i<USER_STRING_COUNT; ++i )
		{
			strings[i] = GetUserString( (EUserStringOrdinal)i );
		}

		m_cachedUserData.Init( USER_STRING_COUNT, strings );
		// LAVA--

		if (InitRenderStates(sAppState, aSearchPaths, nMaxAnisotropy, fTextureAlphaScalar, m_cachedUserData))
		{
			if (Init3dGeometry( ))
			{
				if (!IsGrassModel( ))
					m_bGraphicsInitialized = InitBillboardGeometry( );
				else
					m_bGraphicsInitialized = true;

				if (!InitConstantBuffers( ))
					CCore::SetError("CTreeRI_t::InitGfx, constant buffer initialization failed [%s]\n", GetFilename( ));
			}
			else
				CCore::SetError("CTreeRI_t::InitGfx, InitGeometry failed [%s]", GetFilename( ));
		}
		else
			CCore::SetError("CTreeRI_t::InitGfx, InitRenderStates failed [%s]\n", GetFilename( ));
	}
	else
		CCore::SetError("CTreeRI::InitGfx called redundantly [%s]", GetFilename( ));

	return m_bGraphicsInitialized;
}

///////////////////////////////////////////////////////////////////////
//  CTreeRI::ReleaseTextures
CTreeRI_TemplateList
inline void CTreeRI_t::ReleaseTextures()
{
	for (st_int32 nPass = 0; nPass < RENDER_PASS_COUNT; ++nPass)
	{
		for (size_t i = 0; i < m_aa3dRenderStates[nPass].size( ); ++i)
			m_aa3dRenderStates[nPass][i].ReleaseTextures( );

		m_aBillboardRenderStates[nPass].ReleaseTextures( );
	}
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::ReleaseGfxResources

CTreeRI_TemplateList
inline void CTreeRI_t::ReleaseGfxResources(void)
{
	// release render states
	for (st_int32 nPass = 0; nPass < RENDER_PASS_COUNT; ++nPass)
	{
		for (size_t i = 0; i < m_aa3dRenderStates[nPass].size( ); ++i)
			m_aa3dRenderStates[nPass][i].ReleaseGfxResources( );

		m_aBillboardRenderStates[nPass].ReleaseGfxResources( );
	}

	// release geometry
	for (size_t i = 0; i < m_atGeometryBuffers.size( ); ++i)
		m_atGeometryBuffers[i].ReleaseGfxResources( );

	m_tBillboardGeometryBuffer.ReleaseGfxResources( );

	m_cBaseTreeConstantBuffer.ReleaseGfxResources( );
	m_cWindDynamicsConstantBuffer.ReleaseGfxResources( );

	// LAVA++
	// reset cached user data
	m_cachedUserData.Reset();
	// LAVA--

	m_bGraphicsInitialized = false;
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::GraphicsAreInitialized

CTreeRI_TemplateList
inline st_bool CTreeRI_t::GraphicsAreInitialized(void) const
{
	return m_bGraphicsInitialized;
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::GetGeometryBuffer

CTreeRI_TemplateList
inline const TGeometryBufferClass* CTreeRI_t::GetGeometryBuffer(st_int32 nLodLevel, st_int32 nDrawCall) const
{
	st_int32 nOffset = GetGeometryBufferOffset(nLodLevel, nDrawCall);

	assert(!m_atGeometryBuffers.empty( ));
	assert(nOffset < st_int32(m_atGeometryBuffers.size( )));

	return &m_atGeometryBuffers[nOffset];
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::GetGeometryBufferOffset

CTreeRI_TemplateList
inline st_int32 CTreeRI_t::GetGeometryBufferOffset(st_int32 nLodLevel, st_int32 nDrawCall) const
{
	return nLodLevel * m_nMaxNumDrawCallsPerLod + nDrawCall;
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::GetGeometryBuffers

CTreeRI_TemplateList
inline const CArray<TGeometryBufferClass>& CTreeRI_t::GetGeometryBuffers(void) const
{
	return m_atGeometryBuffers;
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::GetVertexBufferUsage

CTreeRI_TemplateList
inline void CTreeRI_t::GetVertexBufferUsage(st_int32& nVertexBuffers, st_int32& nIndexBuffers) const
{
	// vertex buffers
	{
		nVertexBuffers = 0;

		// 3d geometry
		for (size_t i = 0; i < m_atGeometryBuffers.size( ); ++i)
			nVertexBuffers += m_atGeometryBuffers[i].VertexSize( ) * m_atGeometryBuffers[i].NumVertices( );

		// billboard
		nVertexBuffers += m_tBillboardGeometryBuffer.VertexSize( ) * m_tBillboardGeometryBuffer.NumVertices( );
	}

	// index buffers
	{
		nIndexBuffers = 0;

		// 3d geometry
		for (size_t i = 0; i < m_atGeometryBuffers.size( ); ++i)
			nIndexBuffers += m_atGeometryBuffers[i].IndexSize( ) * m_atGeometryBuffers[i].NumIndices( );

		// billboard
		nIndexBuffers += m_tBillboardGeometryBuffer.IndexSize( ) * m_tBillboardGeometryBuffer.NumIndices( );
	}
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::UpdateWindConstantBuffer

CTreeRI_TemplateList
inline st_bool CTreeRI_t::UpdateWindConstantBuffer(void) const
{
	// todo: skip if wind is disabled, but still call update?
	memcpy(&m_sWindDynamicsConstantBufferLayout, GetWindShaderTable( ), CWind::NUM_SHADER_VALUES * sizeof(st_float32));
	assert(CWind::NUM_SHADER_VALUES * sizeof(st_float32) == sizeof(m_sWindDynamicsConstantBufferLayout));

	// copy data to constant buffer
	return m_cWindDynamicsConstantBuffer.Update( ); // todo: if skipped due to no wind, return true
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::BindConstantBuffers

CTreeRI_TemplateList
inline st_bool CTreeRI_t::BindConstantBuffers(void) const
{
	// LAVA++
	if( m_isDirty )
	{
		m_cBaseTreeConstantBuffer.Update( );
		m_cWindDynamicsConstantBuffer.Update( );
		m_isDirty = false;
	}
	// LAVA--

	return (m_cBaseTreeConstantBuffer.Bind( ) && m_cWindDynamicsConstantBuffer.Bind( ));
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::Get3dRenderStates

CTreeRI_TemplateList
inline const CArray<CRenderStateRI_t>& CTreeRI_t::Get3dRenderStates(ERenderPass eShaderType) const
{
	return m_aa3dRenderStates[eShaderType];
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::GetBillboardRenderState

CTreeRI_TemplateList
inline const CRenderStateRI_t& CTreeRI_t::GetBillboardRenderState(ERenderPass eShaderType) const
{
	return m_aBillboardRenderStates[eShaderType];
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::GetBillboardGeometryBuffer

CTreeRI_TemplateList
inline const TGeometryBufferClass& CTreeRI_t::GetBillboardGeometryBuffer(void) const
{
	return m_tBillboardGeometryBuffer;
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::FindMaxNumDrawCallsPerLod

CTreeRI_TemplateList
inline st_int32 CTreeRI_t::FindMaxNumDrawCallsPerLod(void)
{
	st_int32 nMax = 0;

	const SGeometry* pGeometry = GetGeometry( );
	if (pGeometry && pGeometry->m_nNum3dRenderStates > 0)
	{
		st_int32 nNumLods = pGeometry->m_nNumLods;

		for (st_int32 nLod = 0; nLod < nNumLods; ++nLod)
		{
			const SLod* pLod = pGeometry->m_pLods + nLod;

			nMax = st_max(nMax, pLod->m_nNumDrawCalls);
		}
	}

	return nMax;
}


///////////////////////////////////////////////////////////////////////  
//	Helper Function: AdjustVertexDeclForInstancing

inline void AdjustVertexDeclForInstancing(SVertexDecl& sVertexDecl)
{
	SVertexDecl sAdjustedDecl = sVertexDecl;

	const st_int32 c_nNumInstanceAttributes = 3;
	const EVertexAttribute c_eFirstInstanceAttribute = VERTEX_ATTRIB_1; 
	assert(VERTEX_ATTRIB_1 + c_nNumInstanceAttributes < VERTEX_ATTRIB_COUNT);

	// clear attributes reserved for instancing so that SVertexDecl::MergeObjectAndInstanceVertexDecls will work later
	for (st_int32 i = c_eFirstInstanceAttribute; i < c_eFirstInstanceAttribute + c_nNumInstanceAttributes && i < VERTEX_ATTRIB_COUNT; ++i)
		sAdjustedDecl.m_asAttributes[i].Clear( );

	// copy the rest of the attributes, offset c_nNumInstanceAttributes slots
	for (st_int32 i = c_eFirstInstanceAttribute + c_nNumInstanceAttributes; i < VERTEX_ATTRIB_COUNT; ++i)
		sAdjustedDecl.m_asAttributes[i] = sVertexDecl.m_asAttributes[i - c_nNumInstanceAttributes];

	// copy back to render state
	sVertexDecl = sAdjustedDecl;
}

///////////////////////////////////////////////////////////////////////
//  CTreeRI::SetTextureAlphaScalars

// LAVA++
CTreeRI_TemplateList
inline void CTreeRI_t::SetTextureAlphaScalars( st_float32 alphaScalar3d, st_float32 alphaScalarGrass, st_float32 alphaScalarBillboards )
{
	const Bool isGrass = IsCompiledAsGrass(); 
	for ( st_int32 pass_i=0; pass_i<RENDER_PASS_COUNT; ++pass_i )
	{
		// Update grass/3d renderstates
		{
			CArray<CRenderStateRI_t> &arr = m_aa3dRenderStates[pass_i];
			for ( Uint32 i=0; i<arr.size(); ++i )
			{
				arr[i].SetTextureAlphaScalar( isGrass ? alphaScalarGrass : alphaScalar3d );
			}
		}

		// Update billboard renderstates
		if ( !isGrass )
		{
			m_aBillboardRenderStates[pass_i].SetTextureAlphaScalar( alphaScalarBillboards );
		}
	}
}
// LAVA--

///////////////////////////////////////////////////////////////////////
//  CTreeRI::InitRenderStates

CTreeRI_TemplateList
inline st_bool CTreeRI_t::InitRenderStates(const SAppState& sAppState, 
										   const CArray<CFixedString>& aSearchPaths, 
										   st_int32 nMaxAnisotropy,
										   st_float32 fTextureAlphaScalar,
										   const SCachedUserData& cachedUserData )
{
	st_bool bSuccess = false;

	const SGeometry* pGeometry = GetGeometry( );
	if (pGeometry && pGeometry->m_nNum3dRenderStates > 0)
	{
		bSuccess = true;

		// adjust vertex declaration for mesh instancing if the platform requires it
		if (TShaderTechniqueClass::VertexDeclNeedsInstancingAttribs( ))
		{
			for (st_int32 nPass = 0; nPass < RENDER_PASS_COUNT; ++nPass)
			{
				if (nPass == RENDER_PASS_DEPTH_PREPASS && !pGeometry->m_bDepthOnlyIncluded)
					continue;
				if (nPass == RENDER_PASS_SHADOW_CAST && !pGeometry->m_bShadowCastIncluded)
					continue;

				for (st_int32 i = 0; i < pGeometry->m_nNum3dRenderStates; ++i)
					AdjustVertexDeclForInstancing(pGeometry->m_p3dRenderStates[nPass][i].m_sVertexDecl);
			}
		}

		for (st_int32 nPass = 0; nPass < RENDER_PASS_COUNT; ++nPass)
		{
			m_aa3dRenderStates[nPass].resize(pGeometry->m_nNum3dRenderStates);

            if (nPass == RENDER_PASS_DEPTH_PREPASS && (!sAppState.m_bDepthPrepass || !pGeometry->m_bDepthOnlyIncluded))
                continue;
            if (nPass == RENDER_PASS_SHADOW_CAST && (sAppState.m_eShadowConfig == SRenderState::SHADOW_CONFIG_OFF || !pGeometry->m_bShadowCastIncluded))
                continue;

			// make a copy of sAppState to modify
			SAppState sLocalAppState = sAppState;
			if (nPass == RENDER_PASS_SHADOW_CAST)
				sLocalAppState.m_bDepthPrepass = false;

			for (st_int32 i = 0; i < pGeometry->m_nNum3dRenderStates; ++i)
			{
				CFixedString strVertexShaderBaseName = pGeometry->m_p3dRenderStates[nPass][i].VertexShaderHashName(m_cWind, sLocalAppState.m_eShadowConfig);
				CFixedString strPixelShaderBaseName = pGeometry->m_p3dRenderStates[nPass][i].PixelShaderHashName(sLocalAppState.m_eShadowConfig);

				m_aa3dRenderStates[nPass][i] = pGeometry->m_p3dRenderStates[nPass][i];
				bSuccess &= m_aa3dRenderStates[nPass][i].InitGfx(sLocalAppState,
																 aSearchPaths,				// folders to scan for assets
																 nMaxAnisotropy,			// texture quality
																 fTextureAlphaScalar,
																 strVertexShaderBaseName,
																 strPixelShaderBaseName,
																 &m_cWind,
																 cachedUserData.m_interactiveEnabled);
			}

			// billboard render states
			if (!IsGrassModel( )) // grass models don't have billboards
			{
				CFixedString strVertexShaderBaseName = pGeometry->m_aBillboardRenderStates[nPass].VertexShaderHashName(m_cWind, sLocalAppState.m_eShadowConfig);
				CFixedString strPixelShaderBaseName = pGeometry->m_aBillboardRenderStates[nPass].PixelShaderHashName(sLocalAppState.m_eShadowConfig);

				m_aBillboardRenderStates[nPass] = pGeometry->m_aBillboardRenderStates[nPass];
				bSuccess &= m_aBillboardRenderStates[nPass].InitGfx(sLocalAppState,
																	aSearchPaths,			// folders to scan for assets
																	nMaxAnisotropy,			// texture quality
																	fTextureAlphaScalar,
																	strVertexShaderBaseName,
																	strPixelShaderBaseName,
																	&m_cWind,
																	cachedUserData.m_interactiveEnabled);
			}
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::Init3dGeometry

CTreeRI_TemplateList
inline st_bool CTreeRI_t::Init3dGeometry(void)
{
	st_bool bSuccess = false;

	const SGeometry* pGeometry = GetGeometry( );
	if (pGeometry && pGeometry->m_nNum3dRenderStates > 0)
	{
		m_nMaxNumDrawCallsPerLod = FindMaxNumDrawCallsPerLod( );

		st_int32 nNumLods = pGeometry->m_nNumLods;
		st_int32 nNumGeometries = nNumLods * m_nMaxNumDrawCallsPerLod;

		if (nNumGeometries > 0)
		{
			bSuccess = true;

			m_atGeometryBuffers.resize(nNumGeometries, TGeometryBufferClass(false, false, "3d tree"));
			TGeometryBufferClass* pGeometryBuffer = &m_atGeometryBuffers[0];

			for (st_int32 nLod = 0; nLod < nNumLods; ++nLod)
			{
				assert(static_cast<const SLod*>(pGeometry->m_pLods));
				const SLod* pLod = pGeometry->m_pLods + nLod;

				for (st_int32 nDrawCall = 0; nDrawCall < pLod->m_nNumDrawCalls; ++nDrawCall)
				{
					assert(static_cast<const SDrawCall*>(pLod->m_pDrawCalls));
					const SDrawCall* pDrawCall = pLod->m_pDrawCalls + nDrawCall;

					if (pDrawCall->m_nNumVertices > 0 &&
						pDrawCall->m_nNumIndices > 0)
					{
						// get render state
						const CRenderStateRI_t& sRenderState = Get3dRenderStates(RENDER_PASS_MAIN)[pDrawCall->m_nRenderStateIndex];

						// modify vertex decl to include instance stream if applicable
						SVertexDecl sInstanceVertexDecl;
						if (TShaderTechniqueClass::VertexDeclNeedsInstancingAttribs( ))
						{
							if (!SVertexDecl::MergeObjectAndInstanceVertexDecls(sInstanceVertexDecl, sRenderState.m_sVertexDecl, SVertexDecl::INSTANCES_3D_TREES))
								CCore::SetError("SVertexDecl::MergeObjectAndInstanceVertexDecls() failed in CTreeRI_t::Init3dGeometry()");
						}

						if (pGeometryBuffer->SetVertexDecl(sRenderState.m_sVertexDecl, &sRenderState.GetTechnique( ), sInstanceVertexDecl) &&
							pGeometryBuffer->SetIndexFormat(pDrawCall->m_b32BitIndices ? INDEX_FORMAT_UNSIGNED_32BIT : INDEX_FORMAT_UNSIGNED_16BIT))
						{
							assert(pGeometryBuffer->VertexSize( ) == st_uint32(pDrawCall->m_pRenderState->m_sVertexDecl.m_uiVertexSize));

							// use reserve to keep internal heap allocations down
							bSuccess &= pGeometryBuffer->Reserve(pDrawCall->m_nNumVertices, pDrawCall->m_nNumIndices);

							// copy the raw vertex buffer directly into the VB
							bSuccess &= pGeometryBuffer->AppendVertices(pDrawCall->m_pVertexData, pDrawCall->m_nNumVertices);

							// copy the raw index buffer directly into the IB
							bSuccess &= pGeometryBuffer->AppendIndices(pDrawCall->m_pIndexData, pDrawCall->m_nNumIndices);

							// finalize buffers
							bSuccess &= pGeometryBuffer->EndVertices( );
							bSuccess &= pGeometryBuffer->EndIndices( );
						}
						else
						{
							CCore::SetError("CTreeRI_t::InitGeometry internal error, SetVertexDecl() || SetIndexFormat() failed");
							bSuccess = false;
						}
					}

					++pGeometryBuffer;
				}

				pGeometryBuffer += m_nMaxNumDrawCallsPerLod - pLod->m_nNumDrawCalls; // make sure next LOD starts with the right geometry buffer
			}

			assert(pGeometryBuffer - &m_atGeometryBuffers[0] == m_nMaxNumDrawCallsPerLod * nNumLods);
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  Structure SBillboardVertex
//
//	Holds vertex data needed to render a single instance billboard that
//	turns to face the camera. Note that "billboard" denotes an entire image
//	of a tree, not to be confused with "leaf card" which is the technique
//	of leaf textures in a 3D tree that rotate to face the camera.
//
//	This structure holds the main geomtery vertex definition and will be 
//	instanced in conjunction with the SBillboardInstanceVertex structure.

struct SBillboardVertex
{
		// first vector
		st_float32	m_afCorner[3];			// worldspace coordinates that define the shape of the billboard (can be a cutout
											// as defined in the Compiler application)
		st_float32	m_fBillboardFlag;		// 1.0 if the vertex is part of a vertical billboard, 0.0 if horizontal

		// second vector
		st_float32	m_afUnitTexCoords[2];	// billboards don't have to be rectangular, they can have form-fitting cutouts with
											// multiple triangles; these are the [0.0,1.0] coordinates for the cutout, used by
											// vertex shader when pulling a single billboard image from the billboard atlas
};


///////////////////////////////////////////////////////////////////////
//  CTreeRI::InitBillboardGeometry

CTreeRI_TemplateList
inline st_bool CTreeRI_t::InitBillboardGeometry(void)
{
	st_bool bSuccess = false;

	const SGeometry* pGeometry = GetGeometry( );
	if (pGeometry)
	{
		// modify vertex decl to include instance stream if applicable
		SVertexDecl sInstanceVertexDecl;
		if (TShaderTechniqueClass::VertexDeclNeedsInstancingAttribs( ))
		{
			if (!SVertexDecl::MergeObjectAndInstanceVertexDecls(sInstanceVertexDecl, m_aBillboardRenderStates[RENDER_PASS_MAIN].m_sVertexDecl, SVertexDecl::INSTANCES_BILLBOARDS))
				CCore::SetError("SVertexDecl::MergeObjectAndInstanceVertexDecls() failed in CTreeRI_t::InitBillboardGeometry()");
		}

		if (m_tBillboardGeometryBuffer.SetVertexDecl(m_aBillboardRenderStates[RENDER_PASS_MAIN].m_sVertexDecl, &m_aBillboardRenderStates[RENDER_PASS_MAIN].GetTechnique( ), sInstanceVertexDecl) &&
			m_tBillboardGeometryBuffer.SetIndexFormat(INDEX_FORMAT_UNSIGNED_16BIT))
		{
			const st_float32 c_fVerticalBillboardFlag = 1.0f;
			const st_float32 c_fHorizontalBillboardFlag = 0.0f;

			// vertices
			const SVerticalBillboards& sVertBBs = pGeometry->m_sVertBBs;
			const SHorizontalBillboard& sHorzBB = pGeometry->m_sHorzBB;
			assert(sVertBBs.m_nNumCutoutVertices > 0);
			const st_int32 c_nNumVerticalVertices = sVertBBs.m_nNumCutoutVertices;
			const st_int32 c_nNumHorizontalVertices = sHorzBB.m_bPresent ? 4 : 0;
			CStaticArray<SBillboardVertex> aVertices(c_nNumVerticalVertices + c_nNumHorizontalVertices, "CTreeRI_t::InitBillboardGeometry, vertices");
			{
				// vertical billboard vertices
				const st_float32 c_fWidth = sVertBBs.m_fWidth;
				const st_float32 c_fLeft = -0.5f * c_fWidth;
				const st_float32 c_fHeight = sVertBBs.m_fTopPos - sVertBBs.m_fBottomPos;
				const st_float32 c_fBottom = sVertBBs.m_fBottomPos;
				for (st_int32 i = 0; i < c_nNumVerticalVertices; ++i)
				{
					const st_float32* c_pTexCoords = sVertBBs.m_pCutoutVertices + i * 2;

					// corner
					aVertices[i].m_afCorner[0] = c_fLeft + c_pTexCoords[0] * c_fWidth;
					aVertices[i].m_afCorner[1] = 0.0f;
					aVertices[i].m_afCorner[2] = c_fBottom + c_pTexCoords[1] * c_fHeight;

					// flag this vertex as vertical
					aVertices[i].m_fBillboardFlag = c_fVerticalBillboardFlag;

					// texcoords
					aVertices[i].m_afUnitTexCoords[0] = c_pTexCoords[0];
					aVertices[i].m_afUnitTexCoords[1] = c_pTexCoords[1];
				}

				// horizontal
				if (sHorzBB.m_bPresent)
				{
					const SBillboardVertex c_asHorzVertices[4] =
					{
						{ { sHorzBB.m_avPositions[0].x, sHorzBB.m_avPositions[0].y, sHorzBB.m_avPositions[0].z }, c_fHorizontalBillboardFlag, { sHorzBB.m_afTexCoords[0], sHorzBB.m_afTexCoords[1] } },
						{ { sHorzBB.m_avPositions[1].x, sHorzBB.m_avPositions[1].y, sHorzBB.m_avPositions[1].z }, c_fHorizontalBillboardFlag, { sHorzBB.m_afTexCoords[2], sHorzBB.m_afTexCoords[3] } },
						{ { sHorzBB.m_avPositions[2].x, sHorzBB.m_avPositions[2].y, sHorzBB.m_avPositions[2].z }, c_fHorizontalBillboardFlag, { sHorzBB.m_afTexCoords[4], sHorzBB.m_afTexCoords[5] } },
						{ { sHorzBB.m_avPositions[3].x, sHorzBB.m_avPositions[3].y, sHorzBB.m_avPositions[3].z }, c_fHorizontalBillboardFlag, { sHorzBB.m_afTexCoords[6], sHorzBB.m_afTexCoords[7] } }
					};
					assert(aVertices.size( ) > size_t(c_nNumVerticalVertices));
					memcpy(&aVertices[c_nNumVerticalVertices], c_asHorzVertices, 4 * sizeof(SBillboardVertex));
				}
			}

			// indices
			const st_int32 c_nNumVerticalIndices = sVertBBs.m_nNumCutoutIndices;
			const st_int32 c_nNumHorizontalIndices = sHorzBB.m_bPresent ? 6 : 0;
			CStaticArray<st_uint16> aIndices(c_nNumVerticalIndices + c_nNumHorizontalIndices, "CTreeRI_t::InitBillboardGeometry, indices");
			{
				// vertical
				memcpy(&aIndices[0], sVertBBs.m_pCutoutIndices, sizeof(st_uint16) * c_nNumVerticalIndices);

				// determine max vertical bb index, needed as offset for possible adjacent horz bb indices
				st_uint16 uiMaxVertIndex = 0;
				for (st_int32 i = 0; i < c_nNumVerticalIndices; ++i)
					uiMaxVertIndex = st_max(uiMaxVertIndex, aIndices[i]);
				
				// horizontal
				if (sHorzBB.m_bPresent)
				{
					const st_uint16 c_auHorzIndices[6] = 
					{ 
						st_uint16(uiMaxVertIndex + 1), 
						st_uint16(uiMaxVertIndex + 2), 
						st_uint16(uiMaxVertIndex + 3), 
						st_uint16(uiMaxVertIndex + 1), 
						st_uint16(uiMaxVertIndex + 3), 
						st_uint16(uiMaxVertIndex + 4)
					};
					memcpy(&aIndices[c_nNumVerticalIndices], c_auHorzIndices, 6 * sizeof(st_uint16));
				}
			}

			// copy the raw vertex buffer directly into the VB
			const st_int32 c_nTotalVertices = c_nNumVerticalVertices + c_nNumHorizontalVertices;
			const st_int32 c_nTotalIndices = c_nNumVerticalIndices + c_nNumHorizontalIndices;
			bSuccess = m_tBillboardGeometryBuffer.Reserve(c_nTotalVertices, c_nTotalIndices);
			bSuccess &= m_tBillboardGeometryBuffer.AppendVertices(&aVertices[0], c_nTotalVertices);
			bSuccess &= m_tBillboardGeometryBuffer.AppendIndices(&aIndices[0], c_nTotalIndices);

			// finalize buffers
			bSuccess &= m_tBillboardGeometryBuffer.EndVertices( );
			bSuccess &= m_tBillboardGeometryBuffer.EndIndices( );
		}

		// create modified billboard texcoords
		//
		// to avoid having to upload a table of flags for the 360-degree billboard images that indicate if
		// their horizontally or vertically oriented, we negate the first texcoord to indicate that it is
		// "flipped" meaning that it appears with a sideway orientation in the billboard atlas
		if (m_sGeometry.m_sVertBBs.m_nNumBillboards > 0 && m_sGeometry.m_sVertBBs.m_pTexCoords)
		{
			m_aModifiedBillboardTexCoords.resize(m_sGeometry.m_sVertBBs.m_nNumBillboards * 4);
			
			// copy the texcoords, negating the first of every four
			const st_float32* pSrc = m_sGeometry.m_sVertBBs.m_pTexCoords;
			st_float32* pDest = &m_aModifiedBillboardTexCoords[0];
			for (st_int32 i = 0; i < m_sGeometry.m_sVertBBs.m_nNumBillboards * 4; i += 4)
			{
				// negate first texcoord if atlas entry is flipped
				if (m_sGeometry.m_sVertBBs.m_pRotated[i / 4])
					*pDest++ = -(*pSrc++);
				else
					*pDest++ = *pSrc++;

				*pDest++ = *pSrc++;
				*pDest++ = *pSrc++;
				*pDest++ = *pSrc++;
			}
		}
	}

	return bSuccess;
}

///////////////////////////////////////////////////////////////////////
//  CTreeRI::UpdateBaseTreeCBDataBasedOnLodProfile
CTreeRI_TemplateList
inline void CTreeRI_t::UpdateBaseTreeCBDataBasedOnLodProfile(void)
{
	// 3D LOD values
	const SLodProfile& c_sLodProfile = CTree::GetLodProfile( );
	m_sBaseTreeConstantBufferLayout.m_f3dLodHighDetailDist = c_sLodProfile.m_fHighDetail3dDistance;
	m_sBaseTreeConstantBufferLayout.m_f3dLodRange = c_sLodProfile.m_f3dRange;
	m_sBaseTreeConstantBufferLayout.m_f3dGrassStartDist = c_sLodProfile.m_fHighDetail3dDistance;
	m_sBaseTreeConstantBufferLayout.m_f3dGrassRange = (c_sLodProfile.m_fLowDetail3dDistance - c_sLodProfile.m_fHighDetail3dDistance);

	// billboard LOD values
	const st_float32 c_fHorzFadeValue = 0.15f; // to be tuned
	//const st_float32 c_fBillboardOverhang = m_sRenderInfo.m_sAppState.m_bAlphaToCoverage ? 0.65f : 1.0f; // todo: need to know a2c
	const st_float32 c_fBillboardOverhang = 0.65f;
	m_sBaseTreeConstantBufferLayout.m_fBillboardHorzFade = c_fHorzFadeValue; 
	m_sBaseTreeConstantBufferLayout.m_fOneMinusBillboardHorzFade = (1.0f - c_fHorzFadeValue);
	m_sBaseTreeConstantBufferLayout.m_fBillboardRange = c_sLodProfile.m_fBillboardRange * c_fBillboardOverhang;
	m_sBaseTreeConstantBufferLayout.m_fBillboardStartDist = c_sLodProfile.m_fBillboardStartDistance - m_sBaseTreeConstantBufferLayout.m_fBillboardRange;

	// hue variation
	const SHueVariationParams& c_sHueVariationParams = CTree::GetHueVariationParams( );
	m_sBaseTreeConstantBufferLayout.m_fHueVariationByPos = c_sHueVariationParams.m_fByPos;
	m_sBaseTreeConstantBufferLayout.m_fHueVariationByVertex = c_sHueVariationParams.m_fByVertex;
	m_sBaseTreeConstantBufferLayout.m_vHueVariationColor = c_sHueVariationParams.m_vColor;

	// ambient image
	m_sBaseTreeConstantBufferLayout.m_fAmbientImageScalar = CTree::GetAmbientImageScalar( );

	// billboards
	const st_int32 c_nNumBillboards = CTree::GetGeometry( )->m_sVertBBs.m_nNumBillboards;
	if (c_nNumBillboards <= MAX_NUM_BILLBOARDS_PER_BASE_TREE)
	{
		m_sBaseTreeConstantBufferLayout.m_fNumBillboards = st_float32(c_nNumBillboards);
		if (c_nNumBillboards > 0)
		{
			m_sBaseTreeConstantBufferLayout.m_fRadiansPerImage = (c_nNumBillboards == 0) ? c_fTwoPi : c_fTwoPi / c_nNumBillboards;

			// copy billboard texcoord table
			assert(m_aModifiedBillboardTexCoords.size( ) == size_t(c_nNumBillboards * 4));
			memcpy(m_sBaseTreeConstantBufferLayout.m_avBillboardTexCoords, &m_aModifiedBillboardTexCoords[0], c_nNumBillboards * sizeof(Vec4));
		}
	}
	else
		CCore::SetError("CTreeRI_t::InitConstantBuffer(), too many billboards in SRT file: %d (limit is %d)", c_nNumBillboards, MAX_NUM_BILLBOARDS_PER_BASE_TREE);
	
	m_isDirty = true;
}

///////////////////////////////////////////////////////////////////////
//  CTreeRI::InitConstantBuffers

CTreeRI_TemplateList
inline st_bool CTreeRI_t::InitConstantBuffers(void)
{
	st_bool bSuccess = false;

	// base tree constant buffer
	if (m_cBaseTreeConstantBuffer.Init(&m_sBaseTreeConstantBufferLayout, sizeof(m_sBaseTreeConstantBufferLayout), CONST_BUF_REGISTER_BASE_TREE))
	{
		UpdateBaseTreeCBDataBasedOnLodProfile();

		// LAVA++
		if ( IsCompiledAsGrass() )
		{
			m_sBaseTreeConstantBufferLayout.m_vLavaCustomBaseTreeParams.x = GetCachedUserData().m_grassPigmentEnabled ? 1.f : 0.f;
			m_sBaseTreeConstantBufferLayout.m_vLavaCustomBaseTreeParams.y = GetCachedUserData().m_grassFloodPigmentEnabled ? 1.f : 0.f;
			m_sBaseTreeConstantBufferLayout.m_vLavaCustomBaseTreeParams.z = GetCachedUserData().m_grassRandomColorEnabled ? 1.f : 0.f;
			m_sBaseTreeConstantBufferLayout.m_vLavaCustomBaseTreeParams.w = GetCachedUserData().m_grassTerrainNormalsEnabled ? 1.f : 0.f;
		}
		else
		{
			m_sBaseTreeConstantBufferLayout.m_vLavaCustomBaseTreeParams.x = 0.5f + 0.01f * (Float)GetCachedUserData().m_billboardColorType;
			m_sBaseTreeConstantBufferLayout.m_vLavaCustomBaseTreeParams.y = 0.0f;
			m_sBaseTreeConstantBufferLayout.m_vLavaCustomBaseTreeParams.z = 0.0f;
			m_sBaseTreeConstantBufferLayout.m_vLavaCustomBaseTreeParams.w = 0.0f;
		}
	
		// Don't want to update CBs here, as we are likely on a loading thread, and not on the rendering thread
		bSuccess = true;
		// LAVA--
	}

	if (bSuccess)
	{
		// wind dynamics constant buffer
		if (m_cWindDynamicsConstantBuffer.Init(&m_sWindDynamicsConstantBufferLayout, sizeof(m_sWindDynamicsConstantBufferLayout), CONST_BUF_REGISTER_WIND_DYNAMICS))
		{
			// write table directly from CWind class
			// LAVA++ Don't want to update CBs here, as we are likely on a loading thread, and not on the rendering thread
			bSuccess = true;
			// LAVA--
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CTreeRI::InitTexture

CTreeRI_TemplateList
inline st_bool CTreeRI_t::InitTexture(const char* pFilename,
									  TTextureClass& tTextureObject,
									  const CArray<CFixedString>& aSearchPaths, 
									  st_int32 nMaxAnisotropy)
{
	st_bool bSuccess = false;

	if (pFilename && strlen(pFilename) > 0)
	{
		bSuccess = true;

		// look for the texture at each location in the search path
		st_bool bFound = false;
		for (st_int32 nSearchPath = 0; nSearchPath < st_int32(aSearchPaths.size( )) && !bFound; ++nSearchPath)
		{
			CFixedString strSearchLocation = aSearchPaths.at(nSearchPath) + CFixedString(pFilename).NoPath( );

			// if the Load() call succeeds, the texture was found
			if (tTextureObject.Load(strSearchLocation.c_str( ), nMaxAnisotropy))
			{
				bFound = true;
				break;
			}
		}

		bSuccess &= bFound;
	}

	return bSuccess;
}
