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

//#define SHADER_CACHE_USES_FULL_PATHNAMES


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::CShaderTechniqueRI

CShaderTechniqueRI_TemplateList
inline CShaderTechniqueRI_t::CShaderTechniqueRI( )
{
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::~CShaderTechniqueRI

CShaderTechniqueRI_TemplateList
inline CShaderTechniqueRI_t::~CShaderTechniqueRI( )
{
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::Bind

CShaderTechniqueRI_TemplateList
inline st_bool CShaderTechniqueRI_t::Bind(void) const
{
	st_bool bSuccess = false;

	#if !defined(NDEBUG) && !defined(SPEEDTREE_OPENGL)
		if (IsValid( ))
		{
	#endif

			bSuccess = m_tShaderTechniquePolicy.Bind(m_tVertexShader, m_tPixelShader);

	#if !defined(NDEBUG) && !defined(SPEEDTREE_OPENGL)
		}
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::UnBind

CShaderTechniqueRI_TemplateList
inline st_bool CShaderTechniqueRI_t::UnBind(void)
{
	TShaderTechniquePolicy::UnBind( );

	return true;
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::IsValid

CShaderTechniqueRI_TemplateList
inline st_bool CShaderTechniqueRI_t::IsValid(void) const
{
	return m_tVertexShader.IsValid( ) &&
		   m_tPixelShader.IsValid( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::Init

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::Init(void)
{
	return TShaderConstantPolicy::Init( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::ReleaseGfxResources

CShaderConstantRI_TemplateList
inline void CShaderConstantRI_t::ReleaseGfxResources(void)
{
	TShaderConstantPolicy::ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::Reset

CShaderConstantRI_TemplateList
inline void CShaderConstantRI_t::Reset(void)
{
	TShaderConstantPolicy::Reset( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::Set1f

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::Set1f(const SStaticShaderConstant& sRegister, st_float32 x)
{
	return TShaderConstantPolicy::Set4f(sRegister, x, 0.0f, 0.0f, 0.0f);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::Set2f

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::Set2f(const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y)
{
	return TShaderConstantPolicy::Set4f(sRegister, x, y, 0.0f, 0.0f);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::Set2fv

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::Set2fv(const SStaticShaderConstant& sRegister, const st_float32 afValues[2])
{
	const st_float32 c_afValues4[4] = { afValues[0], afValues[1], 0.0f, 0.0f };

	return TShaderConstantPolicy::Set4fv(sRegister, c_afValues4);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::Set3f

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::Set3f(const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y, st_float32 z)
{
	return TShaderConstantPolicy::Set4f(sRegister, x, y, z, 0.0f);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::Set3fv

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::Set3fv(const SStaticShaderConstant& sRegister, const st_float32 afValues[3])
{
	const st_float32 c_afValues4[4] = { afValues[0], afValues[1], afValues[2], 0.0f };

	return TShaderConstantPolicy::Set4fv(sRegister, c_afValues4);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::Set4f

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::Set4f(const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y, st_float32 z, st_float32 w)
{
	return TShaderConstantPolicy::Set4f(sRegister, x, y, z, w);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::Set4fv

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::Set4fv(const SStaticShaderConstant& sRegister, const st_float32 afValues[4])
{
	return TShaderConstantPolicy::Set4fv(sRegister, afValues);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::Set4fvArray

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::Set4fvArray(const SStaticShaderConstant& sRegister, st_int32 nNum4fValues, const st_float32* p4fValues)
{
	return TShaderConstantPolicy::Set4fvArray(sRegister, nNum4fValues, p4fValues);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::SetMatrix

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::SetMatrix(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16])
{
	return TShaderConstantPolicy::SetMatrix(sRegister, afMatrix);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::SetMatrixTranspose

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::SetMatrixTranspose(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16])
{
	return TShaderConstantPolicy::SetMatrixTranspose(sRegister, afMatrix);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::SetTexture

CShaderConstantRI_TemplateList
inline st_bool CShaderConstantRI_t::SetTexture(st_int32 nRegister, const TTextureClass& cTexture, st_bool bSubmitImmediately)
{
	return TShaderConstantPolicy::SetTexture(nRegister, cTexture, bSubmitImmediately);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantRI::SubmitSetTexturesInBatch

CShaderConstantRI_TemplateList
inline void CShaderConstantRI_t::SubmitSetTexturesInBatch(void)
{
	TShaderConstantPolicy::SubmitSetTexturesInBatch( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::Init

CShaderConstantBufferRI_TemplateList
inline st_bool CShaderConstantBufferRI_t::Init(void* pLayout, size_t siSizeOfLayout, st_int32 nRegister)
{
	return m_tShaderConstantBufferPolicy.Init(pLayout, siSizeOfLayout, nRegister);
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::ReleaseGfxResources

CShaderConstantBufferRI_TemplateList
inline void CShaderConstantBufferRI_t::ReleaseGfxResources(void)
{
	m_tShaderConstantBufferPolicy.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::Update

CShaderConstantBufferRI_TemplateList
inline st_bool CShaderConstantBufferRI_t::Update(void) const
{
	return m_tShaderConstantBufferPolicy.Update( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderConstantBufferRI::Bind

CShaderConstantBufferRI_TemplateList
inline st_bool CShaderConstantBufferRI_t::Bind( ) const
{
	return m_tShaderConstantBufferPolicy.Bind( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::Load

CShaderTechniqueRI_TemplateList
inline st_bool CShaderTechniqueRI_t::Load(const char* pVertexFilename, 
										  const char* pPixelFilename, 
										  const SAppState& sAppState,
										  const SRenderState& sRenderState)
{
	assert(pVertexFilename && strlen(pVertexFilename) > 0);
	assert(pPixelFilename && strlen(pPixelFilename) > 0);

	st_bool bSuccess = true;

	if (m_tShaderTechniquePolicy.LoadProgramBinary(pVertexFilename, pPixelFilename))
	{
		m_strVertexShaderName = pVertexFilename;
		m_strPixelShaderName = pPixelFilename;
		Report("  technique from program binary [%s & %s] OK", m_strVertexShaderName.NoPath( ).c_str( ), m_strPixelShaderName.NoPath( ).c_str( ));
	}
	else
	{
		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);

		// get vertex shader
        st_bool bFoundInVSCache = false;
		{
			// create the cache if first use
			if (!m_pVertexShaderCache)
				m_pVertexShaderCache = st_new(CVertexShaderCache, "CVertexShaderCache")(GFX_RESOURCE_VERTEX_SHADER, c_nShaderCacheInitialReserve);
			assert(m_pVertexShaderCache);
			m_strVertexShaderName = pVertexFilename;

			// cache lookup name
			#ifdef SHADER_CACHE_USES_FULL_PATHNAMES
				const CFixedString strCacheFilename = m_strVertexShaderName;
			#else
				CFixedString strCacheFilename = m_strVertexShaderName.NoPath( );
			#endif

			// is this vertex shader already cached? 
			TVertexShader* pShader = m_pVertexShaderCache->Retrieve(strCacheFilename);
			if (!pShader)
			{
				// since a series of paths are scanned, check that the file exists first so that an error is registered only
				// when the load fails for other reasons
				// LAVA++ Redundant call
				//if (pFileSystem->FileSize(m_strVertexShaderName.c_str( )) > 0)
				//{
					// wasn't in cache, so create it
					if (m_tVertexShader.Load(m_strVertexShaderName.c_str( ), sAppState, sRenderState))
					{
						// and add to cache
						const size_t c_siUnknownSize = 0;
						m_pVertexShaderCache->Add(strCacheFilename, m_tVertexShader, c_siUnknownSize);
					}
					else
						bSuccess = false;
				//}
				//else
				//	bSuccess = false;
				// LAVA--
			}
			else
            {
				m_tVertexShader = *pShader;
                bFoundInVSCache = true;
            }
		}

		// get pixel shader
		st_bool bFoundInPSCache = false;
		{
			// create the cache if first use
			if (!m_pPixelShaderCache)
				m_pPixelShaderCache = st_new(CPixelShaderCache, "CPixelShaderCache")(GFX_RESOURCE_PIXEL_SHADER, c_nShaderCacheInitialReserve);
			assert(m_pPixelShaderCache);
			m_strPixelShaderName = pPixelFilename;

			// cache lookup name
			#ifdef SHADER_CACHE_USES_FULL_PATHNAMES
				// LAVA++
				//const CFixedString strCacheFilename = m_strPixelShaderName;
				CFixedString strCacheFilename = m_strPixelShaderName;
				// LAVA--
			#else
				CFixedString strCacheFilename = m_strPixelShaderName.NoPath( );
			#endif

			// LAVA++
			// Adapt filename (before cache retrieval!)
			TShaderTechniquePolicy::CPixelShader::AdaptCacheFilename( strCacheFilename, sRenderState );
			m_strPixelShaderCacheName = strCacheFilename;
			// LAVA--

			// is this pixel shader already cached? 
			TPixelShader* pShader = m_pPixelShaderCache->Retrieve(strCacheFilename);
			if (!pShader)
			{
				// since a series of paths are scanned, check that the file exists first so that an error is registered only
				// when the load fails for other reasons
				// LAVA++ Redundant call
				//if (pFileSystem->FileExists(m_strPixelShaderName.c_str( )))
				//{
					// wasn't in cache, so create it
					if (m_tPixelShader.Load(m_strPixelShaderName.c_str( ), sAppState, sRenderState))
					{
						// and add to cache
						const size_t c_siUnknownSize = 0;
						m_pPixelShaderCache->Add(strCacheFilename, m_tPixelShader, c_siUnknownSize);
					}
					else
						bSuccess = false;
				//}
				//else
				//	bSuccess = false;
				// LAVA--
			}
			else
            {
				m_tPixelShader = *pShader;
                bFoundInPSCache = true;
            }
		}

		if (bSuccess)
		{
			bSuccess = m_tShaderTechniquePolicy.Link(m_tVertexShader, m_tPixelShader);

			if (bSuccess)
			{
				m_tShaderTechniquePolicy.SaveProgramBinary(pVertexFilename, pPixelFilename);

				if (!bFoundInVSCache || !bFoundInPSCache)
					Report("  vsc: %d, psc: %d, technique [%s & %s] OK", 
						m_pVertexShaderCache->Size( ), m_pPixelShaderCache->Size( ),
						m_strVertexShaderName.NoPath( ).c_str( ), m_strPixelShaderName.NoPath( ).c_str( ));
			}
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::ReleaseGfxResources

CShaderTechniqueRI_TemplateList
inline void CShaderTechniqueRI_t::ReleaseGfxResources(void)
{
	if (m_pVertexShaderCache)
	{
		#ifdef SHADER_CACHE_USES_FULL_PATHNAMES
			const CFixedString strCacheFilename = m_strVertexShaderName;
		#else
			CFixedString strCacheFilename = m_strVertexShaderName.NoPath( );
		#endif

		st_int32 nRefCount = m_pVertexShaderCache->Release(strCacheFilename);
		if (nRefCount == 0)
			m_tVertexShader.ReleaseGfxResources( );

		// delete cache if empty
		if (m_pVertexShaderCache->Size( ) == 0)
			st_delete<CVertexShaderCache>(m_pVertexShaderCache);
	}

	if (m_pPixelShaderCache)
	{
		// LAVA++
		/*
		#ifdef SHADER_CACHE_USES_FULL_PATHNAMES
			const CFixedString strCacheFilename = m_strPixelShaderName;
		#else
			CFixedString strCacheFilename = m_strPixelShaderName.NoPath( );
		#endif

		st_int32 nRefCount = m_pPixelShaderCache->Release(strCacheFilename);
		*/
		st_int32 nRefCount = m_pPixelShaderCache->Release(m_strPixelShaderCacheName);
		// LAVA--

		if (nRefCount == 0)
			m_tPixelShader.ReleaseGfxResources( );

		// delete cache if empty
		if (m_pPixelShaderCache->Size( ) == 0)
			st_delete<CPixelShaderCache>(m_pPixelShaderCache);
	}

	(void) m_tShaderTechniquePolicy.ReleaseGfxResources( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::GetCompiledShaderExtension

CShaderTechniqueRI_TemplateList
inline CFixedString CShaderTechniqueRI_t::GetCompiledShaderExtension(void)
{
	return TShaderTechniquePolicy::GetCompiledShaderExtension( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::GetCompiledShaderFolder

CShaderTechniqueRI_TemplateList
inline CFixedString CShaderTechniqueRI_t::GetCompiledShaderFolder(void)
{
	return TShaderTechniquePolicy::GetCompiledShaderFolder( );
}


///////////////////////////////////////////////////////////////////////
//  CShaderTechniqueRI::VertexDeclNeedsInstancingAttribs

CShaderTechniqueRI_TemplateList
inline st_bool CShaderTechniqueRI_t::VertexDeclNeedsInstancingAttribs(void)
{
	return TShaderTechniquePolicy::VertexDeclNeedsInstancingAttribs( );
}

