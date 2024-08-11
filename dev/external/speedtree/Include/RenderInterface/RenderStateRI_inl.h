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


////////////////////////////////////////////////////////////
// CRenderStateRI_t::CRenderStateRI

CRenderStateRI_TemplateList
inline CRenderStateRI_t::CRenderStateRI( ) :
	// LAVA++
	m_isDirty( true ),
	m_forcedTextureAlphaScalar( 1 )
	// LAVA--
{
}


////////////////////////////////////////////////////////////
// CRenderStateRI_t::~CRenderStateRI

CRenderStateRI_TemplateList
inline CRenderStateRI_t::~CRenderStateRI( )
{
}


////////////////////////////////////////////////////////////
//	Helper function: StringToLongHash

inline st_int64 StringToLongHash(const CFixedString& strInput)
{
	st_int64 lHash = 0;

	for (size_t i = 0; i < strInput.length( ); ++i)
		lHash = strInput[i] + (lHash << 6) + (lHash << 16) - lHash;

	return lHash;
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::InitGfx

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::InitGfx(const SAppState& sAppState,
										 const CArray<CFixedString>& aSearchPaths,
										 st_int32 nMaxAnisotropy,
										 st_float32 fTextureAlphaScalar,
										 const CFixedString& strVertexShaderBaseName,
										 const CFixedString& strPixelShaderBaseName,
										 const CWind* pWind,
										 st_bool isInteractive)		// LAVA edit: add isInteractive flag
{
	// init textures
	st_bool bSuccess = LoadTextures(aSearchPaths, nMaxAnisotropy);

	// init constant buffer
	bSuccess &= InitConstantBuffer(sAppState, fTextureAlphaScalar, pWind);

	// init state block
	bSuccess &= m_cStateBlock.Init(sAppState, *this, isInteractive);	// LAVA edit: add isInteractive flag

	// init shaders
	bSuccess &= LoadShaders(aSearchPaths, strVertexShaderBaseName, strPixelShaderBaseName);

	// load fallback textures if not yet done (shared by all render state objects)
	// LAVA++ Let's not do that here, we are likely in a loading thread.
	bSuccess &= true;
	// LAVA--

	// set sort key value based on unique shader pair
	// todo
	// LAVA edit: we need to state block in hash string to distinguish interactive foliage
	CFixedString strHashKey = CFixedString("v") + strVertexShaderBaseName + CFixedString("p") + strPixelShaderBaseName + CFixedString::Format("%d", m_eFaceCulling) + CFixedString::Format("%i", m_cStateBlock.GetStateBlockPolicy()->GetDrawContextRefValue() );
	m_lSortKey = StringToLongHash(strHashKey);

	return bSuccess;
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::ReleaseGfxResources

CRenderStateRI_TemplateList
inline void CRenderStateRI_t::ReleaseGfxResources(void)
{
	// release shaders
	m_cTechnique.ReleaseGfxResources( );

	// release state block
	m_cStateBlock.ReleaseGfxResources( );

	// release textures
	for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
		m_atTextureObjects[i].ReleaseGfxResources( );

	// LAVA++-- removing release of fallback textures

	m_cConstantBuffer.ReleaseGfxResources( );
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::GetTechnique

CRenderStateRI_TemplateList
inline const TShaderTechniqueClass& CRenderStateRI_t::GetTechnique(void) const
{
	return m_cTechnique;
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::GetStateBlock

CRenderStateRI_TemplateList
inline const TStateBlockClass& CRenderStateRI_t::GetStateBlock(void) const
{
	return m_cStateBlock;
}


// LAVA++
////////////////////////////////////////////////////////////
//  CRenderStateRI_t::GetTextureClass

CRenderStateRI_TemplateList
	inline const TTextureClass& CRenderStateRI_t::GetTextureClass( st_uint32 uiIndex ) const
{
	return m_atTextureObjects[ uiIndex ];
}
// LAVA--

////////////////////////////////////////////////////////////
//	CRenderStateRI_t::BindConstantBuffer

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindConstantBuffer(void) const
{
	// LAVA++
	if( m_isDirty )
	{
		m_cConstantBuffer.Update();
		m_isDirty = false;
	}
	// LAVA--

	return m_cConstantBuffer.Bind( );
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::BindShader

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindShader(void) const
{
	return m_cTechnique.Bind( );
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::BindTextures

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindTextures(ERenderPass ePass, ETextureBindMode eTextureBindMode/*LAVA:Adding distance*/, Float distance) const
{
	st_bool bSuccess = true;

	if (eTextureBindMode != TEXTURE_BIND_DISABLED)
	{
		// always bind diffuse texture
		const TTextureClass& cTextureToBind = ((eTextureBindMode == TEXTURE_BIND_ENABLED) && m_atTextureObjects[TL_DIFFUSE].IsValid( )) ? m_atTextureObjects[TL_DIFFUSE] : m_atFallbackTextures[TL_DIFFUSE];
		bSuccess &= BindTexture(TL_DIFFUSE, cTextureToBind, distance);

		// only bind the render of the layers if lighting pass is used (diffuse is always needed in case of alpha transparency)
		if (ePass == RENDER_PASS_MAIN)
		{
			for (st_int32 nLayer = TL_DIFFUSE + 1; nLayer < TL_NUM_TEX_LAYERS; ++nLayer)
			{
				const TTextureClass& cNonDiffuseTextureToBind = ((eTextureBindMode == TEXTURE_BIND_ENABLED) && m_atTextureObjects[nLayer].IsValid( )) ? m_atTextureObjects[nLayer] : m_atFallbackTextures[nLayer];
				bSuccess &= BindTexture(nLayer, cNonDiffuseTextureToBind, distance);
			}
		}		

		TShaderConstantClass::SubmitSetTexturesInBatch( );
	}

	return bSuccess;
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::BindStateBlock

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindStateBlock(void) const
{
	return m_cStateBlock.Bind( );
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::Bind

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindMaterialWhole(ERenderPass ePass, ETextureBindMode eTextureBindMode/*LAVA:Adding distance*/, Float distance) const
{
	return BindTextures(ePass, eTextureBindMode/*LAVA:Adding distance*/, distance) && 
		   BindConstantBuffer( ) &&
		   BindShader( ) && 
		   BindStateBlock( );
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::UnBind

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::UnBind(void)
{
	return TShaderTechniqueClass::UnBind( );
}

/////////////////////////////////////////////////////////////
//CRenderStateRI_t::ReleaseTextures

CRenderStateRI_TemplateList
inline void CRenderStateRI_t::ReleaseTextures()
{
	for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
		m_atTextureObjects[i].ReleaseGfxResources( );
}

////////////////////////////////////////////////////////////
// CRenderStateRI_t::LoadTextures

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::LoadTextures(const CArray<CFixedString>& aSearchPaths, st_int32 nMaxAnisotropy)
{
	st_bool bSuccess = true;

	for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
	{
		// if texture is blank, don't attempt to load
		const char* pFilename = m_apTextures[i];
		if (pFilename && strlen(pFilename) > 0)
		{
			if (!InitTexture(pFilename, m_atTextureObjects[i], aSearchPaths, nMaxAnisotropy))
				CCore::SetError("Failed to load texture [%s]; using fallback", pFilename);
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CRenderStateRI_t::InitTexture

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::InitTexture(const char* pFilename,
											 TTextureClass& tTextureObject,
											 const CArray<CFixedString>& aSearchPaths, 
											 st_int32 nMaxAnisotropy)
{
	st_bool bSuccess = false;

	if (pFilename && strlen(pFilename) > 0)
	{
		// LAVA: Search path no. 0 is the one we want to have textures in.
		const CFixedString& searchPath = aSearchPaths.at(0);

		// build complete filename, adjusting for per-platform requirements
		CFixedString strSearchLocation = CFileSystem::CleanPlatformFilename(searchPath + c_szFolderSeparator + CFixedString(pFilename).NoPath( ));
			
		// if the Load() call succeeds, the texture was found
		bSuccess = tTextureObject.Load(strSearchLocation.c_str( ), nMaxAnisotropy);
	}

	return bSuccess;
}

///////////////////////////////////////////////////////////////////////
//  CRenderStateRI_t::SetTextureAlphaScalar

// LAVA++
CRenderStateRI_TemplateList
inline void CRenderStateRI_t::SetTextureAlphaScalar( st_float32 fTextureAlphaScalar )
{	
	if ( m_forcedTextureAlphaScalar == fTextureAlphaScalar )
	{
		return;
	}

	m_forcedTextureAlphaScalar = fTextureAlphaScalar;

	const Float combinedAlphaScalar = m_fAlphaScalar * fTextureAlphaScalar;
	if ( m_sConstantBufferLayout.m_fAlphaScalar != combinedAlphaScalar )
	{
		m_sConstantBufferLayout.m_fAlphaScalar = combinedAlphaScalar;
		m_isDirty = true;
	}
}
// LAVA--

///////////////////////////////////////////////////////////////////////
//  CRenderStateRI_t::InitConstantBuffer

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::InitConstantBuffer(const SAppState& sAppState, st_float32 /*fTextureAlphaScalar*/, const CWind* pWind)
{
	st_bool bSuccess = false;

	if (m_cConstantBuffer.Init(&m_sConstantBufferLayout, sizeof(m_sConstantBufferLayout), CONST_BUF_REGISTER_MATERIAL))
	{
		const Vec3 c_vOneThird(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);

		m_sConstantBufferLayout.m_vAmbientColor = m_vAmbientColor;
		m_sConstantBufferLayout.m_vDiffuseColor = m_vDiffuseColor * m_fDiffuseScalar;
		m_sConstantBufferLayout.m_vSpecularColor = m_vSpecularColor;
		m_sConstantBufferLayout.m_vTransmissionColor = (m_vTransmissionColor - c_vOneThird) * 3.0f;
		m_sConstantBufferLayout.m_fShininess = m_fShininess;
		m_sConstantBufferLayout.m_BranchSeamWeight = m_fBranchSeamWeight;
		m_sConstantBufferLayout.m_fOneMinusAmbientContrastFactor = 1.0f - m_fAmbientContrastFactor;
		m_sConstantBufferLayout.m_fTransmissionShadowBrightness = m_fTransmissionShadowBrightness;
		m_sConstantBufferLayout.m_fTransmissionViewDependency = m_fTransmissionViewDependency;

		/*
		if (sAppState.m_bAlphaToCoverage)
		{
			// m_fAlphaScalar = alpha scalar (controls fullness of transparent textures; tuned by artist in Modeler)
			m_sConstantBufferLayout.m_fAlphaScalar = m_fAlphaScalar;
		}
		else 
		*/
		assert( !sAppState.m_bAlphaToCoverage );
		{
			// m_fAlphaScalar = alpha scalar (controls fullness of transparent textures; tuned by artist in Modeler)
			m_sConstantBufferLayout.m_fAlphaScalar = m_fAlphaScalar * m_forcedTextureAlphaScalar;
		}

		// effect flags
		//
		// this table must match the c_asEffects table in ShaderSourceGenerator.cpp in the SRT Exporter source
		const bool c_bPerVertexLighting = (m_eLightingModel == LIGHTING_MODEL_PER_VERTEX) || (m_eLightingModel == LIGHTING_MODEL_PER_VERTEX_X_PER_PIXEL);
		const bool c_bPerPixelLighting = (m_eLightingModel == LIGHTING_MODEL_PER_PIXEL) || (m_eLightingModel == LIGHTING_MODEL_PER_VERTEX_X_PER_PIXEL);
		const bool c_bLightingTransition = (m_eLightingModel == LIGHTING_MODEL_PER_VERTEX_X_PER_PIXEL);

		float* pEffectFlag = &(m_sConstantBufferLayout.m_avEffectConfigFlags[0].x);

		// FORWARD_LIGHTING_PER_VERTEX
		*pEffectFlag++ = st_float32(c_bPerVertexLighting);

		// FORWARD_LIGHTING_PER_PIXEL	
		*pEffectFlag++ = st_float32(c_bPerPixelLighting);

		// FORWARD_LIGHTING_TRANSITION
		*pEffectFlag++ = st_float32(c_bLightingTransition);

		// AMBIENT_OCCLUSION
		*pEffectFlag++ = st_float32(m_bAmbientOcclusion);

		// AMBIENT_CONTRAST			
		*pEffectFlag++ = st_float32(m_eAmbientContrast);

		// DETAIL_LAYER				
		*pEffectFlag++ = st_float32(m_eDetailLayer);

		// DETAIL_NORMAL_LAYER
		*pEffectFlag++ = st_float32(m_eDetailLayer != EFFECT_OFF && IsTextureLayerPresent(TL_DETAIL_DIFFUSE));

		// SPECULAR					
		*pEffectFlag++ = st_float32(m_eSpecular);

		// TRANSMISSION				
		*pEffectFlag++ = st_float32(m_eTransmission);

		// BRANCH_SEAM_SMOOTHING		
		*pEffectFlag++ = st_float32(m_eBranchSeamSmoothing);

		// SMOOTH_LOD					
		*pEffectFlag++ = (m_eLodMethod == LOD_METHOD_SMOOTH ? 1.0f : 0.0f);

		// FADE_TO_BILLBOARD
		*pEffectFlag++ = st_float32(m_bFadeToBillboard ? 1.0f : 0.0f);

		// HAS_HORZ_BB
		*pEffectFlag++ = st_float32(m_bHorzBillboard);

		// BACKFACE_CULLING			
		*pEffectFlag++ = st_float32(m_eFaceCulling);

		// AMBIENT_IMAGE_LIGHTING		
		*pEffectFlag++ = st_float32(m_eAmbientImageLighting);

		// HUE_VARIATION				
		*pEffectFlag++ = st_float32(m_eHueVariation);

		// SHADOW_SMOOTHING			
		*pEffectFlag++ = st_float32(m_bShadowSmoothing ? 1.0f : 0.0f);

		// DIFFUSE_MAP_OPAQUE			
		*pEffectFlag++ = st_float32(m_bDiffuseAlphaMaskIsOpaque ? 1.0f : 0.0f);

		assert((pEffectFlag - &m_sConstantBufferLayout.m_avEffectConfigFlags[0].x) <= NUM_EFFECT_CONFIG_FLOAT4S * 4);

		// various wind flags
		if (pWind)
		{
			// wind config flags
			float* pWindConfigFlag = &(m_sConstantBufferLayout.m_avWindConfigFlags[0].x);
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::GLOBAL_WIND)					&& IsGlobalWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::GLOBAL_PRESERVE_SHAPE)			&& IsGlobalWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_SIMPLE_1)				&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_DIRECTIONAL_1)			&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_DIRECTIONAL_FROND_1)		&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_TURBULENCE_1)			&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_WHIP_1)					&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_OSC_COMPLEX_1)			&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_SIMPLE_2)				&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_DIRECTIONAL_2)			&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_DIRECTIONAL_FROND_2)		&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_TURBULENCE_2)			&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_WHIP_2)					&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::BRANCH_OSC_COMPLEX_2)			&& IsBranchWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_RIPPLE_VERTEX_NORMAL_1)	&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_RIPPLE_COMPUTED_1)			&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_TUMBLE_1)					&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_TWITCH_1)					&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_OCCLUSION_1)				&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_RIPPLE_VERTEX_NORMAL_2)	&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_RIPPLE_COMPUTED_2)			&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_TUMBLE_2)					&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_TWITCH_2)					&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::LEAF_OCCLUSION_2)				&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::FROND_RIPPLE_ONE_SIDED)			&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::FROND_RIPPLE_TWO_SIDED)			&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::FROND_RIPPLE_ADJUST_LIGHTING)	&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			*pWindConfigFlag++ = (pWind->IsOptionEnabled(CWind::ROLLING)						&& IsFullWindEnabled( )) ? 1.0f: 0.0f;
			assert((pWindConfigFlag - &m_sConstantBufferLayout.m_avWindConfigFlags[0].x) <= NUM_WIND_CONFIG_FLOAT4S * 4);

			// wind LOD options
			float* pWindLodFlag = &(m_sConstantBufferLayout.m_avWindLodFlags[0].x);
			*pWindLodFlag++ = (m_eWindLod == WIND_LOD_GLOBAL)			? 1.0f : 0.0f;
			*pWindLodFlag++ = (m_eWindLod == WIND_LOD_BRANCH)			? 1.0f : 0.0f;
			*pWindLodFlag++ = (m_eWindLod == WIND_LOD_FULL)				? 1.0f : 0.0f;
			*pWindLodFlag++ = (m_eWindLod == WIND_LOD_NONE_X_GLOBAL)	? 1.0f : 0.0f;
			*pWindLodFlag++ = (m_eWindLod == WIND_LOD_NONE_X_BRANCH)	? 1.0f : 0.0f;
			*pWindLodFlag++ = (m_eWindLod == WIND_LOD_NONE_X_FULL)		? 1.0f : 0.0f;
			*pWindLodFlag++ = (m_eWindLod == WIND_LOD_GLOBAL_X_BRANCH)	? 1.0f : 0.0f;
			*pWindLodFlag++ = (m_eWindLod == WIND_LOD_GLOBAL_X_FULL)	? 1.0f : 0.0f;
			*pWindLodFlag++ = (m_eWindLod == WIND_LOD_BRANCH_X_FULL)	? 1.0f : 0.0f;
			*pWindLodFlag++ = (m_eWindLod == WIND_LOD_NONE)				? 1.0f : 0.0f;

			// is full wind in transition? if so, set flag for rolling wind to fade with it
			*pWindLodFlag++ = ((m_eWindLod == WIND_LOD_NONE_X_FULL) || 
							   (m_eWindLod == WIND_LOD_GLOBAL_X_FULL) ||
							   (m_eWindLod == WIND_LOD_BRANCH_X_FULL)) ? 1.0f : 0.0f;

			// is branch wind on at all?
			*pWindLodFlag++ = IsBranchWindEnabled( );

			assert((pWindLodFlag - &m_sConstantBufferLayout.m_avWindLodFlags[0].x) <= NUM_WIND_LOD_FLOAT4S * 4);

			// LAVA ++
			m_sConstantBufferLayout.m_vLavaCustomMaterialParams.x = 10.f * (m_fTransmissionViewDependency - 0.5f);
			m_sConstantBufferLayout.m_vLavaCustomMaterialParams.y = 10.f * m_fTransmissionShadowBrightness;
			m_sConstantBufferLayout.m_vLavaCustomMaterialParams.z = 0.0f;
			m_sConstantBufferLayout.m_vLavaCustomMaterialParams.w = 0.0f;
			// LAVA --
		}
		else
		{
			memset(m_sConstantBufferLayout.m_avWindConfigFlags, 0, sizeof(m_sConstantBufferLayout.m_avWindConfigFlags));
			memset(m_sConstantBufferLayout.m_avWindLodFlags, 0, sizeof(m_sConstantBufferLayout.m_avWindLodFlags));
		}

		// LAVA++
		bSuccess = true;
		// LAVA--
	}

	return bSuccess;
}


////////////////////////////////////////////////////////////
// CRenderStateRI_t::BindTexture

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::BindTexture(st_int32 nLayer, const TTextureClass& tTexture/*LAVA:Adding distance*/, Float distance) const
{
	st_bool bSuccess = false;

	if (tTexture.IsValid( ))
	{
		bSuccess = true;

		if (tTexture != m_atLastBoundTextures[nLayer])
		{
			bSuccess = TShaderConstantClass::SetTexture(TEXTURE_REGISTER_DIFFUSE + nLayer, tTexture, false, distance);
			m_atLastBoundTextures[nLayer] = tTexture;
		}
	}

	return bSuccess;
}


////////////////////////////////////////////////////////////
// CRenderStateRI_t::LoadShaders

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::LoadShaders(const CArray<CFixedString>& aSearchPaths,
											 const CFixedString& strVertexShaderBaseName,
											 const CFixedString& strPixelShaderBaseName)
{
	st_bool bSuccess = false;

	// get file system pointer from Core lib
	CFileSystem* pFileSystem = CFileSystemInterface::Get( );
	assert(pFileSystem);

	// determine platform-specific shader filenames (no path)
	const CFixedString c_strVertexShaderFilename = CFixedString::Format("%s_vs.%s", strVertexShaderBaseName.c_str( ), TShaderTechniqueClass::GetCompiledShaderExtension( ).c_str( ));
	const CFixedString c_strPixelShaderFilename = CFixedString::Format("%s_ps.%s", strPixelShaderBaseName.c_str( ), TShaderTechniqueClass::GetCompiledShaderExtension( ).c_str( ));

	// search through the paths until first occurance of both files
	st_bool bFilesFound = false;
	CFixedString strVertexFullPath, strPixelFullPath;
	
	// LAVA: We know the shaders path it's at aSearchPaths[1]
	const CFixedString& searchPath = aSearchPaths[1];
	// look for shader files
	strVertexFullPath = pFileSystem->CleanPlatformFilename(searchPath + c_szFolderSeparator + c_strVertexShaderFilename);
	strPixelFullPath = pFileSystem->CleanPlatformFilename(searchPath + c_szFolderSeparator + c_strPixelShaderFilename);

	// load pre-compiled shaders (or compile then at run-time is GLSL)
	if (m_cTechnique.Load(strVertexFullPath.c_str( ), strPixelFullPath.c_str( ), m_cStateBlock.GetAppState( ), *this))
	{
		bSuccess = true;
	}
	else
	{
		CCore::SetError("Failed to load/compile shader pair [%s & %s]", strVertexFullPath.c_str( ), strPixelFullPath.c_str( ));
	}

	return bSuccess;
}


////////////////////////////////////////////////////////////
// CRenderStateRI_t::InitFallbackTextures

CRenderStateRI_TemplateList
inline st_bool CRenderStateRI_t::InitFallbackTextures(void)
{
	st_bool bSuccess = true;

	++m_nFallbackTextureRefCount;

	if (!m_atFallbackTextures[0].IsValid( ))
	{
		for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
		{
			TTextureClass& tTex = m_atFallbackTextures[i];

			switch (i)
			{
			case TL_DIFFUSE:
			case TL_SPECULAR_MASK:
			case TL_TRANSMISSION_MASK:
				bSuccess &= tTex.LoadColor(0xffffffff);	
				break;
			case TL_NORMAL:
			case TL_DETAIL_NORMAL:
				bSuccess &= tTex.LoadColor(0x7f7fffff);
				break;
			case TL_DETAIL_DIFFUSE:
			case TL_AUX_ATLAS1:
			case TL_AUX_ATLAS2:
				bSuccess &= tTex.LoadColor(0x00000000);
				break;
			}
		}

		// LAVA++ Why not do it here? O_o
		m_bFallbackTexturesInited = bSuccess;
		// LAVA--
	}

	return bSuccess;
}

// LAVA++
CRenderStateRI_TemplateList
inline void	CRenderStateRI_t::ReleaseFallbackTextures( void )
{
	// release fallback textures
	if (m_bFallbackTexturesInited && --m_nFallbackTextureRefCount == 0)
	{
		for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
			if (m_atFallbackTextures[i].IsValid( ))
				m_atFallbackTextures[i].ReleaseGfxResources( );
	}
}
// LAVA--


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::ClearLastBoundTextures

CRenderStateRI_TemplateList
inline void CRenderStateRI_t::ClearLastBoundTextures(void)
{
	for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
		m_atLastBoundTextures[i] = TTextureClass( );
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::operator=

CRenderStateRI_TemplateList
inline CRenderStateRI_t& CRenderStateRI_t::operator=(const CRenderStateRI_t& sRight)
{
	return operator=((SRenderState) sRight);
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::operator=
//
//	todo: check that all SRenderState members are accounted for

CRenderStateRI_TemplateList
inline CRenderStateRI_t& CRenderStateRI_t::operator=(const SRenderState& sRight)
{
	for (st_int32 i = 0; i < TL_NUM_TEX_LAYERS; ++i)
		m_apTextures[i] = sRight.m_apTextures[i];
	m_eLightingModel = sRight.m_eLightingModel;
	m_vAmbientColor = sRight.m_vAmbientColor;
	m_eAmbientContrast = sRight.m_eAmbientContrast;
	m_fAmbientContrastFactor = sRight.m_fAmbientContrastFactor;
	m_bAmbientOcclusion = sRight.m_bAmbientOcclusion;
	m_vDiffuseColor = sRight.m_vDiffuseColor;
	m_fDiffuseScalar = sRight.m_fDiffuseScalar;
	m_bDiffuseAlphaMaskIsOpaque = sRight.m_bDiffuseAlphaMaskIsOpaque;
	m_eDetailLayer = sRight.m_eDetailLayer;
	m_eSpecular = sRight.m_eSpecular;
	m_fShininess = sRight.m_fShininess;
	m_vSpecularColor = sRight.m_vSpecularColor;
	m_eTransmission = sRight.m_eTransmission;
	m_vTransmissionColor = sRight.m_vTransmissionColor;
	m_fTransmissionShadowBrightness = sRight.m_fTransmissionShadowBrightness;
	m_fTransmissionViewDependency = sRight.m_fTransmissionViewDependency;
	m_eBranchSeamSmoothing = sRight.m_eBranchSeamSmoothing;
	m_fBranchSeamWeight = sRight.m_fBranchSeamWeight;
	m_eLodMethod = sRight.m_eLodMethod;
	m_bFadeToBillboard = sRight.m_bFadeToBillboard;
	m_bVertBillboard = sRight.m_bVertBillboard;
	m_bHorzBillboard = sRight.m_bHorzBillboard;
	m_eHueVariation = sRight.m_eHueVariation;
	m_eShaderGenerationMode = sRight.m_eShaderGenerationMode;
	m_bUsedAsGrass = sRight.m_bUsedAsGrass;
	m_eFaceCulling = sRight.m_eFaceCulling;
	m_bBlending = sRight.m_bBlending;
	m_eAmbientImageLighting = sRight.m_eAmbientImageLighting;
	m_eFogCurve = sRight.m_eFogCurve;
	m_eFogColorStyle = sRight.m_eFogColorStyle;
	m_bCastsShadows = sRight.m_bCastsShadows;
	m_bReceivesShadows = sRight.m_bReceivesShadows;
	m_bShadowSmoothing = sRight.m_bShadowSmoothing;
	m_fAlphaScalar = sRight.m_fAlphaScalar;
	m_eWindLod = sRight.m_eWindLod;
	m_eRenderPass = sRight.m_eRenderPass;
	m_bBranchesPresent = sRight.m_bBranchesPresent;
	m_bFrondsPresent = sRight.m_bFrondsPresent;
	m_bLeavesPresent = sRight.m_bLeavesPresent;
	m_bFacingLeavesPresent = sRight.m_bFacingLeavesPresent;
	m_bRigidMeshesPresent = sRight.m_bRigidMeshesPresent;
	m_sVertexDecl = sRight.m_sVertexDecl;
	m_pDescription = sRight.m_pDescription;
	m_pUserData = sRight.m_pUserData;

	return *this;
}


////////////////////////////////////////////////////////////
//	CRenderStateRI_t::GetHashKey

CRenderStateRI_TemplateList
inline st_int64 CRenderStateRI_t::GetHashKey(void) const
{
	return m_lSortKey;
}

