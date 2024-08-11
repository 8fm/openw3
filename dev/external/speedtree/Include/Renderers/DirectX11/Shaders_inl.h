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

// todo: lots of antiquated functions here

///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::Init

inline st_bool CShaderConstantDirectX11::Init(void)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  Helper function: ReleaseConstantBuffer

inline void ReleaseConstantBuffer(ID3D11Buffer*& pBuffer)
{
	if (pBuffer)
	{
		const st_int32 c_nMaxNumBuffers = SHADER_CONSTANT_GROUP_COUNT + 1; // +1 to cover the instancing constant buffer

		// before releasing, unbind vertex shader constant buffer if currently bound
		ID3D11Buffer* apCurrentBuffers[c_nMaxNumBuffers] = { NULL };
		DX11::DeviceContext( )->VSGetConstantBuffers(0, c_nMaxNumBuffers, apCurrentBuffers);
		for (st_int32 i = 0; i < c_nMaxNumBuffers; ++i)
		{
			if (apCurrentBuffers[i] == pBuffer)
			{
				ID3D11Buffer* pNullBuffer = NULL;
				DX11::DeviceContext( )->VSSetConstantBuffers(i, 1, &pNullBuffer);
			}

			// VSGetConstantBuffers incremented their ref counts, so decrement
			ST_SAFE_RELEASE(apCurrentBuffers[i]);
		}

		// before releasing, unbind pixel shader constant buffer if currently bound
		DX11::DeviceContext( )->PSGetConstantBuffers(0, c_nMaxNumBuffers, apCurrentBuffers);
		for (st_int32 i = 0; i < c_nMaxNumBuffers; ++i)
		{
			if (apCurrentBuffers[i] == pBuffer)
			{
				ID3D11Buffer* pNullBuffer = NULL;
				DX11::DeviceContext( )->PSSetConstantBuffers(i, 1, &pNullBuffer);
			}

			// PSGetConstantBuffers incremented their ref counts, so decrement
			ST_SAFE_RELEASE(apCurrentBuffers[i]);
		}

		// now that it's not bound anywhere, release buffer
		ST_SAFE_RELEASE(pBuffer);
	}

	assert(!pBuffer);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::ReleaseGfxResources

inline void CShaderConstantDirectX11::ReleaseGfxResources(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::Reset

inline void CShaderConstantDirectX11::Reset(void)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::Set4f

inline st_bool CShaderConstantDirectX11::Set4f(const SStaticShaderConstant&, st_float32, st_float32, st_float32, st_float32)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::Set4fv

inline st_bool CShaderConstantDirectX11::Set4fv(const SStaticShaderConstant&, const st_float32 afValues[4])
{
	ST_UNREF_PARAM(afValues);

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::Set4fvArray

inline st_bool CShaderConstantDirectX11::Set4fvArray(const SStaticShaderConstant&, st_int32, const st_float32*)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::SetMatrix

inline st_bool CShaderConstantDirectX11::SetMatrix(const SStaticShaderConstant&, const st_float32 afMatrix[16])
{
	ST_UNREF_PARAM(afMatrix);

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::SetMatrixTranspose

inline st_bool CShaderConstantDirectX11::SetMatrixTranspose(const SStaticShaderConstant&, const st_float32 afMatrix[16])
{
	ST_UNREF_PARAM(afMatrix);

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::CommitConstants

inline st_bool CShaderConstantDirectX11::CommitConstants(void)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::FlagConstantBufferAsUpdated

inline void CShaderConstantDirectX11::FlagConstantBufferAsUpdated(const SStaticShaderConstant&)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX11::CShaderConstantBufferDirectX11

inline CShaderConstantBufferDirectX11::CShaderConstantBufferDirectX11( ) :
	m_pLayout(NULL),
	m_siSizeOfLayout(0),
	m_nRegister(0),
	m_pConstantBuffer(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX11::~CShaderConstantBufferDirectX11

inline CShaderConstantBufferDirectX11::~CShaderConstantBufferDirectX11( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX11::Init

inline st_bool CShaderConstantBufferDirectX11::Init(void* pLayout, size_t siSizeOfLayout, st_int32 nRegister)
{
	st_bool bSuccess = false;

	// init only if necessary
	if (!m_pConstantBuffer)
	{
		// copy input parameters
		m_pLayout = pLayout;
		m_siSizeOfLayout = siSizeOfLayout;
		m_nRegister = nRegister;

		// setup constant buffer descriptor
		D3D11_BUFFER_DESC sBufferDesc;
		sBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		sBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		sBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		sBufferDesc.MiscFlags = 0;
		sBufferDesc.ByteWidth = UINT(siSizeOfLayout);

		// round siSizeInBytes up to multiple of 16 if necessary (D3D11 requirement for constant buffers)
		if (sBufferDesc.ByteWidth % 16 != 0)
			sBufferDesc.ByteWidth += 16 - (sBufferDesc.ByteWidth % 16);

		// create constant buffer
		bSuccess = SUCCEEDED(DX11::Device( )->CreateBuffer(&sBufferDesc, NULL, &m_pConstantBuffer));

		if (bSuccess)
			ST_NAME_DX11_OBJECT(m_pConstantBuffer, "speedtree constant buffer");
	}
	else
		bSuccess = true; // already done

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX11::ReleaseGfxResources

inline void CShaderConstantBufferDirectX11::ReleaseGfxResources(void)
{
	ReleaseConstantBuffer(m_pConstantBuffer);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX11::Update

inline st_bool CShaderConstantBufferDirectX11::Update(void) const
{
	st_bool bSuccess = false;

	if (m_pConstantBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE sConstantData;
		if (SUCCEEDED(DX11::DeviceContext( )->Map(m_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, NULL, &sConstantData)))
		{
			memcpy(sConstantData.pData, m_pLayout, m_siSizeOfLayout);

			DX11::DeviceContext( )->Unmap(m_pConstantBuffer, 0);

			bSuccess = true;
		}
		else
			CCore::SetError("CShaderConstantBufferDirectX11::Update(), DX11::DeviceContext( )->Map() failed");
	}
	else
		CCore::SetError("CShaderConstantBufferDirectX11::Update(), constant buffer wasn't successfully initialized");

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantBufferDirectX11::Bind

inline st_bool CShaderConstantBufferDirectX11::Bind(void) const
{
	st_bool bSuccess = false;

	if (m_pConstantBuffer)
	{
		// bind vertex shader constants
		DX11::DeviceContext( )->VSSetConstantBuffers(m_nRegister, 1, &m_pConstantBuffer);

		// bind pixel shader constants
		DX11::DeviceContext( )->PSSetConstantBuffers(m_nRegister, 1, &m_pConstantBuffer);

		bSuccess = true;
	}
	else
		CCore::SetError("CShaderConstantBufferDirectX11::Bind(), constant buffer wasn't successfully initialized");

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::SetTexture

inline st_bool CShaderConstantDirectX11::SetTexture(st_int32 nRegister, const CTexture& cTexture, st_bool bSubmitImmediately)
{
	// texture
	ID3D11ShaderResourceView* pTextureView = cTexture.m_tTexturePolicy.GetTextureObject( );

	if (bSubmitImmediately)
	{
		DX11::DeviceContext( )->PSSetShaderResources(nRegister, 1, &pTextureView);
	}
	else
	{
		assert(nRegister > -1 && nRegister < TEXTURE_REGISTER_COUNT);
		c_apCachedTextureSets[nRegister] = pTextureView;

		m_anTextureRegisterRange[0] = st_min(m_anTextureRegisterRange[0], nRegister);
		m_anTextureRegisterRange[1] = st_max(m_anTextureRegisterRange[1], nRegister);
	}

	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderConstantDirectX11::SubmitSetTexturesInBatch

inline void CShaderConstantDirectX11::SubmitSetTexturesInBatch(void)
{
	st_int32 nNumRegistersSet = (m_anTextureRegisterRange[1] - m_anTextureRegisterRange[0]) + 1;
	if (nNumRegistersSet > 0)
	{
		DX11::DeviceContext( )->PSSetShaderResources(m_anTextureRegisterRange[0], nNumRegistersSet, &c_apCachedTextureSets[m_anTextureRegisterRange[0]]);
		DX11::DeviceContext( )->VSSetShaderResources(m_anTextureRegisterRange[0], nNumRegistersSet, &c_apCachedTextureSets[m_anTextureRegisterRange[0]]);

		// reset range for next batch
		m_anTextureRegisterRange[0] = TEXTURE_REGISTER_COUNT;
		m_anTextureRegisterRange[1] = 0;
		// todo: restore?
		memset(c_apCachedTextureSets, 0, sizeof(c_apCachedTextureSets));
	}
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::CVertexShader::CVertexShader

inline CShaderTechniqueDirectX11::CVertexShader::CVertexShader( ) :
	m_pShader(NULL),
	m_pCompiledShaderCode(NULL),
	m_uiCompiledShaderCodeSize(0)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::CVertexShader::Load

inline st_bool CShaderTechniqueDirectX11::CVertexShader::Load(const char* pFilename, const SAppState&, const SRenderState&)
{
	st_bool bSuccess = false;

	if (pFilename)
	{
		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);

		// load the pre-compiled shader object file
		m_uiCompiledShaderCodeSize = st_uint32(pFileSystem->FileSize(pFilename));
		if (m_uiCompiledShaderCodeSize > 0)
		{
			m_pCompiledShaderCode = (DWORD*) pFileSystem->LoadFile(pFilename, CFileSystem::LONG_TERM);
			if (m_pCompiledShaderCode)
			{
				// create the vertex shader if compiled shader code is available
				bSuccess = SUCCEEDED(DX11::Device( )->CreateVertexShader(m_pCompiledShaderCode, m_uiCompiledShaderCodeSize, NULL, &m_pShader));

				if (bSuccess)
					ST_NAME_DX11_OBJECT(m_pShader, "speedtree vertex shader");
			}
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::CVertexShader::IsValid

inline st_bool CShaderTechniqueDirectX11::CVertexShader::IsValid(void) const
{
	return (m_pShader != NULL);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::CVertexShader::ReleaseGfxResources

inline void CShaderTechniqueDirectX11::CVertexShader::ReleaseGfxResources(void)
{
	if (m_pCompiledShaderCode)
	{
		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);

		pFileSystem->Release((st_byte*) m_pCompiledShaderCode);
		m_pCompiledShaderCode = NULL;
	}

	if (m_pShader)
	{
		// before releasing, unbind vertex shader if currently bound
		ID3D11VertexShader* pCurrentShader = NULL;
		DX11::DeviceContext( )->VSGetShader(&pCurrentShader, NULL, 0);
		if (pCurrentShader == m_pShader)
			DX11::DeviceContext( )->VSSetShader(NULL, NULL, 0);

		// VSGetShader increments references, so decrement
		ST_SAFE_RELEASE(pCurrentShader);

		// now that it's not bound anywhere, release
		ST_SAFE_RELEASE(m_pShader);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::CPixelShader::CPixelShader

inline CShaderTechniqueDirectX11::CPixelShader::CPixelShader( ) :
	m_pShader(NULL)
{
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::CPixelShader::Load

inline st_bool CShaderTechniqueDirectX11::CPixelShader::Load(const char* pFilename, const SAppState&, const SRenderState&)
{
	st_bool bSuccess = false;

	if (pFilename)
	{
		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);

		size_t siCompiledCodeSize = pFileSystem->FileSize(pFilename);
		if (siCompiledCodeSize > 0)
		{
			DWORD* pCompiledShaderCode = (DWORD*) pFileSystem->LoadFile(pFilename);
			if (pCompiledShaderCode)
			{
				// create the pixel shader if compiled shader code is available
				bSuccess = SUCCEEDED(DX11::Device( )->CreatePixelShader(pCompiledShaderCode, siCompiledCodeSize, NULL, &m_pShader));
			
				if (bSuccess)
					ST_NAME_DX11_OBJECT(m_pShader, "speedtree pixel shader");

				pFileSystem->Release((st_byte*) pCompiledShaderCode);
			}
		}
	}

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::CPixelShader::IsValid

inline st_bool CShaderTechniqueDirectX11::CPixelShader::IsValid(void) const
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::CPixelShader::ReleaseGfxResources

inline void CShaderTechniqueDirectX11::CPixelShader::ReleaseGfxResources(void)
{
	if (m_pShader)
	{
		// before releasing, unbind vertex shader if currently bound
		ID3D11PixelShader* pCurrentShader = NULL;
		DX11::DeviceContext( )->PSGetShader(&pCurrentShader, NULL, 0);
		if (pCurrentShader == m_pShader)
			DX11::DeviceContext( )->PSSetShader(NULL, NULL, 0);

		// PSGetShader adds references, so decrement
		ST_SAFE_RELEASE(pCurrentShader);

		// now that it's not bound to anything, release
		ST_SAFE_RELEASE(m_pShader);
	}
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::Link

inline st_bool CShaderTechniqueDirectX11::Link(const CVertexShader&, const CPixelShader&)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::Bind

inline st_bool CShaderTechniqueDirectX11::Bind(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader) const
{
	st_bool bSuccess = false;

	#ifndef NDEBUG
		if (cVertexShader.IsValid( ) && cPixelShader.IsValid( ))
		{
	#endif
			// vertex shader
			DX11::DeviceContext( )->VSSetShader(cVertexShader.m_pShader, NULL, 0);

			// pixel shader
			DX11::DeviceContext( )->PSSetShader(cPixelShader.m_pShader, NULL, 0);

			// geometry shader (inactive)
			DX11::DeviceContext( )->GSSetShader(NULL, NULL, 0);

			bSuccess = CShaderConstantDirectX11::CommitConstants( );

	#ifndef NDEBUG
		}
		else
			CCore::SetError("CShaderTechniqueDirectX11::Bind, either vertex or pixel shader was not valid");
	#endif

	return bSuccess;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::UnBind

inline void CShaderTechniqueDirectX11::UnBind(void)
{
	DX11::DeviceContext( )->VSSetShader(NULL, NULL, 0);
	DX11::DeviceContext( )->PSSetShader(NULL, NULL, 0);
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::ReleaseGfxResources

inline st_bool CShaderTechniqueDirectX11::ReleaseGfxResources(void)
{
	return true;
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::GetCompiledShaderExtension

inline CFixedString CShaderTechniqueDirectX11::GetCompiledShaderExtension(void)
{
	return "fx11obj";
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::GetCompiledShaderFolder

inline CFixedString CShaderTechniqueDirectX11::GetCompiledShaderFolder(void)
{
	return "shaders_directx11";
}


///////////////////////////////////////////////////////////////////////  
//  CShaderTechniqueDirectX11::VertexDeclNeedsInstancingAttribs

inline st_bool CShaderTechniqueDirectX11::VertexDeclNeedsInstancingAttribs(void)
{
	// this should be done in the SRT exporter maybe? though it could screw up users who don't use the rendering api
	return true;
}
