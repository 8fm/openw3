///////////////////////////////////////////////////////////////////////  
//  Shaders.inl
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
//  CShaderConstantOrbis::Init

inline st_bool CShaderConstantOrbis::Init(void)
{
	st_bool bSuccess = true;

	Reset( );

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantOrbis::ReleaseGfxResources

inline void CShaderConstantOrbis::ReleaseGfxResources(void)
{
	// intentionally blank
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantOrbis::Reset

inline void CShaderConstantOrbis::Reset(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantOrbis::Set4f

inline st_bool CShaderConstantOrbis::Set4f(const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y, st_float32 z, st_float32 w)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantOrbis::Set4fv

inline st_bool CShaderConstantOrbis::Set4fv(const SStaticShaderConstant& sRegister, const st_float32 afValues[4])
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantOrbis::Set4fvArray

inline st_bool CShaderConstantOrbis::Set4fvArray(const SStaticShaderConstant& sRegister, st_int32 nNum4fValues, const st_float32* p4fValues)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantOrbis::SetMatrix

inline st_bool CShaderConstantOrbis::SetMatrix(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16])
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantOrbis::SetMatrixTranspose

inline st_bool CShaderConstantOrbis::SetMatrixTranspose(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16])
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantOrbis::SetTexture

inline st_bool CShaderConstantOrbis::SetTexture(st_int32 nRegister, const CTexture& cTexture, st_bool bSubmitImmediately)
{
	Orbis::Context( )->setTextures(sce::Gnm::kShaderStageVs, nRegister, 1, cTexture.m_tTexturePolicy.GetTextureObject( ));
	Orbis::Context( )->setTextures(sce::Gnm::kShaderStagePs, nRegister, 1, cTexture.m_tTexturePolicy.GetTextureObject( ));

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOrbis::CShaderConstantBufferOrbis

inline CShaderConstantBufferOrbis::CShaderConstantBufferOrbis( ) :
	m_pLayout(NULL),
	m_siSizeOfLayout(0),
	m_nRegister(0),
	m_pData(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOrbis::~CShaderConstantBufferOrbis

inline CShaderConstantBufferOrbis::~CShaderConstantBufferOrbis( )
{
	if (m_pData != NULL)
	{
		delete [] m_pData;
		m_pData = NULL;
	}
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOrbis::Init

inline st_bool CShaderConstantBufferOrbis::Init(void* pLayout, size_t siSizeOfLayout, st_int32 nRegister)
{
	if (m_pData == NULL || siSizeOfLayout != m_siSizeOfLayout)
	{
		if (m_pData != NULL)
		{
			delete [] m_pData;
			m_pData = NULL;
		}

		if (siSizeOfLayout > 0)
		{
			m_pData = new st_float32[siSizeOfLayout];
			memset(m_pData, 0, siSizeOfLayout);
		}

		m_siSizeOfLayout = siSizeOfLayout;
	}

	m_pLayout = pLayout;
	m_nRegister = nRegister;

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOrbis::ReleaseGfxResources

inline void CShaderConstantBufferOrbis::ReleaseGfxResources(void)
{
	if (m_pData != NULL)
	{
		delete [] m_pData;
		m_pData = NULL;
	}
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOrbis::Update

inline st_bool CShaderConstantBufferOrbis::Update(void) const
{
	st_bool bSuccess = false;

	if (m_pData != NULL)
	{
		memcpy(m_pData, m_pLayout, m_siSizeOfLayout);
		bSuccess = true;
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferOrbis::Bind

inline st_bool CShaderConstantBufferOrbis::Bind(void) const
{
	bool bReturn = false;

	if (m_pData != NULL)
	{
		void* pConstantData = Orbis::Context( )->allocateFromCommandBuffer(m_siSizeOfLayout, sce::Gnm::kEmbeddedDataAlignment4);
		memcpy(pConstantData, m_pData, m_siSizeOfLayout);

		sce::Gnm::Buffer cConstBuffer;
		cConstBuffer.initAsConstantBuffer(pConstantData, m_siSizeOfLayout);
		cConstBuffer.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO);
			
		Orbis::Context( )->setConstantBuffers(sce::Gnm::kShaderStageVs, m_nRegister, 1, &cConstBuffer);
		Orbis::Context( )->setConstantBuffers(sce::Gnm::kShaderStagePs, m_nRegister, 1, &cConstBuffer);

		bReturn = true;
	}

	return bReturn;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::CVertexShader::CVertexShader

inline CShaderTechniqueOrbis::CVertexShader::CVertexShader( ) :
	m_pShader(NULL),
	m_uiShaderModifier(0),
	m_pFetchShader(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::CVertexShader::Load

inline st_bool CShaderTechniqueOrbis::CVertexShader::Load(const char* pFilename, const SAppState&, const SRenderState& sRenderState)
{
	st_bool bSuccess = false;

	if (pFilename)
	{
		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);

		// read the shader source code into a buffer
		const size_t c_siBufferSize = pFileSystem->FileSize(pFilename);
		if (c_siBufferSize > 0)
		{
			st_byte* pLoadedData = pFileSystem->LoadFile(pFilename);
			if (pLoadedData)
			{
				sce::Gnmx::ShaderInfo sShaderInfo;
				sce::Gnmx::parseShader(&sShaderInfo, pLoadedData);

				unsigned int uiShaderEnd = Orbis::RoundUpToAlignment(sShaderInfo.m_vsShader->computeSize( ), sce::Gnm::kAlignmentOfShaderInBytes);
				unsigned int uiShaderSize = uiShaderEnd + sShaderInfo.m_gpuShaderCodeSize;

				m_pShader = (sce::Gnmx::VsShader*)Orbis::Allocate(uiShaderSize, sce::Gnm::kAlignmentOfShaderInBytes, true);
				memcpy(m_pShader, sShaderInfo.m_vsShader, sShaderInfo.m_vsShader->computeSize( ));
				memcpy((char*)m_pShader + uiShaderEnd, sShaderInfo.m_gpuShaderCode, sShaderInfo.m_gpuShaderCodeSize);
				m_pShader->patchShaderGpuAddress((char*)m_pShader + uiShaderEnd);

				bSuccess = (m_pShader != NULL);

				if (bSuccess)
				{
					uint32_t uiFetchShaderSize = sce::Gnmx::computeVsFetchShaderSize(m_pShader);
					m_pFetchShader = Orbis::Allocate(uiFetchShaderSize, sce::Gnm::kAlignmentOfShaderInBytes, true);
				
					if (sRenderState.GetInstanceType( ) == SVertexDecl::INSTANCES_NONE)
					{
						m_uiShaderModifier = 0;
						sce::Gnmx::generateVsFetchShader(m_pFetchShader, &m_uiShaderModifier, m_pShader, 0);
					}
					else
					{
						sce::Gnm::FetchShaderInstancingMode aInstancingModes[16];
						for (st_int32 i = 0; i < m_pShader->m_numInputSemantics; ++i)
						{
							if (i < 3)
							{
								aInstancingModes[i] = sce::Gnm::kFetchShaderUseInstanceId;
							}
							else
							{
								aInstancingModes[i] = sce::Gnm::kFetchShaderUseVertexIndex;
							}
						}
						
						m_uiShaderModifier = 0;
						sce::Gnmx::generateVsFetchShader(m_pFetchShader, &m_uiShaderModifier, m_pShader, aInstancingModes);
					}
				}
				pFileSystem->Release(pLoadedData);
			}
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::CVertexShader::IsValid

inline st_bool CShaderTechniqueOrbis::CVertexShader::IsValid(void) const
{
	return (m_pShader != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::CVertexShader::ReleaseGfxResources

inline void CShaderTechniqueOrbis::CVertexShader::ReleaseGfxResources(void)
{
	if (m_pShader != NULL)
	{
		Orbis::Release(m_pShader);
		m_pShader = NULL;

		Orbis::Release(m_pFetchShader);
		m_pFetchShader = NULL;

		m_uiShaderModifier = 0;
	}
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::CPixelShader::CPixelShader

inline CShaderTechniqueOrbis::CPixelShader::CPixelShader( ) :
	m_pShader(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::CPixelShader::Load

inline st_bool CShaderTechniqueOrbis::CPixelShader::Load(const char* pFilename, const SAppState&, const SRenderState&)
{
	st_bool bSuccess = false;
	
	if (pFilename)
	{
		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);

		// read the shader source code into a buffer
		const size_t c_siBufferSize = pFileSystem->FileSize(pFilename);
		if (c_siBufferSize > 0)
		{
			st_byte* pLoadedData = pFileSystem->LoadFile(pFilename);
			if (pLoadedData)
			{
				sce::Gnmx::ShaderInfo sShaderInfo;
				sce::Gnmx::parseShader(&sShaderInfo, pLoadedData);

				unsigned int uiShaderEnd = Orbis::RoundUpToAlignment(sShaderInfo.m_psShader->computeSize( ), sce::Gnm::kAlignmentOfShaderInBytes);
				unsigned int uiShaderSize = uiShaderEnd + sShaderInfo.m_gpuShaderCodeSize;

				m_pShader = (sce::Gnmx::PsShader*)Orbis::Allocate(uiShaderSize, sce::Gnm::kAlignmentOfShaderInBytes, true);
				memcpy(m_pShader, sShaderInfo.m_psShader, sShaderInfo.m_psShader->computeSize( ));
				memcpy((char*)m_pShader + uiShaderEnd, sShaderInfo.m_gpuShaderCode, sShaderInfo.m_gpuShaderCodeSize);
				m_pShader->patchShaderGpuAddress((char*)m_pShader + uiShaderEnd);
			
				bSuccess = (m_pShader != NULL);

				pFileSystem->Release(pLoadedData);
			}
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::CPixelShader::IsValid

inline st_bool CShaderTechniqueOrbis::CPixelShader::IsValid(void) const
{
	return (m_pShader != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::CPixelShader::ReleaseGfxResources

inline void CShaderTechniqueOrbis::CPixelShader::ReleaseGfxResources(void)
{
	Orbis::Release(m_pShader);
	m_pShader = NULL;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::Link

inline st_bool CShaderTechniqueOrbis::Link(const CVertexShader&, const CPixelShader&)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::Bind

inline st_bool CShaderTechniqueOrbis::Bind(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader) const
{
	st_bool bSuccess = true;
	
	// vertex shader
	#ifndef NDEBUG
		if (cVertexShader.IsValid( ))
		{
	#endif

			Orbis::Context( )->setVsShader(cVertexShader.m_pShader, cVertexShader.m_uiShaderModifier, cVertexShader.m_pFetchShader);
			bSuccess = true;

	#ifndef NDEBUG
		}
		else
		{
			CCore::SetError("CShaderTechniqueOrbis::Bind, vertex shader was not valid");
			bSuccess = false;
		}
	#endif

	// pixel shader
	#ifndef NDEBUG
		if (cPixelShader.IsValid( ))
		{
	#endif

			Orbis::Context( )->setPsShader(cPixelShader.m_pShader);
			bSuccess = true;

	#ifndef NDEBUG
		}
		else
		{
			CCore::SetError("CShaderTechniqueOrbis::Bind, pixel shader was not valid");
			bSuccess = false;
		}
	#endif
	
	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::UnBind

inline void CShaderTechniqueOrbis::UnBind(void)
{
	// intentionally blank
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::ReleaseGfxResources

inline st_bool CShaderTechniqueOrbis::ReleaseGfxResources(void)
{
	// intentionally blank
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::GetCompiledShaderExtension

inline CFixedString CShaderTechniqueOrbis::GetCompiledShaderExtension(void)
{
	return "sb";
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::GetCompiledShaderFolder

inline CFixedString CShaderTechniqueOrbis::GetCompiledShaderFolder(void)
{
	return "shaders_orbis";
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueOrbis::VertexDeclNeedsInstancingAttribs

inline st_bool CShaderTechniqueOrbis::VertexDeclNeedsInstancingAttribs(void)
{
	return true;
}
