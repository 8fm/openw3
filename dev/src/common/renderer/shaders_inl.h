/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "speedTreeRenderInterface.h"

const st_int32 c_nSizeOfRegister = sizeof(st_float32) * 4;


inline st_bool CShaderConstantGPUAPI::Init()
{
	return true;
}

inline void CShaderConstantGPUAPI::ReleaseGfxResources()
{
	
}

inline void CShaderConstantGPUAPI::Reset()
{
	
}

inline st_bool CShaderConstantGPUAPI::Set4f(const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y, st_float32 z, st_float32 w)
{
	return true;
}

inline st_bool CShaderConstantGPUAPI::Set4fv(const SStaticShaderConstant& sRegister, const st_float32 afValues[4])
{
	return true;
}

inline st_bool CShaderConstantGPUAPI::Set4fvArray(const SStaticShaderConstant& sRegister, st_int32 nNum4fValues, const st_float32* p4fValues)
{
	return true;
}

inline st_bool CShaderConstantGPUAPI::SetMatrix(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16])
{
	return true;
}

inline st_bool CShaderConstantGPUAPI::SetMatrixTranspose(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16])
{
	return true;
}

inline st_bool CShaderConstantGPUAPI::CommitConstants()
{
	return true;
}

inline void CShaderConstantGPUAPI::SubmitSetTexturesInBatch(void)
{
	// batched on gpuapi-side
}

inline void CShaderConstantGPUAPI::FlagConstantBufferAsUpdated(const SStaticShaderConstant& sRegister)
{
}

inline st_bool CShaderConstant::SetTexture(st_int32 nSampler, const CTexture& cTexture, st_bool bSubmitImmediately/*LAVA:Adding distance*/, Float distance )
{	
	CRenderTexture* texObject = cTexture.m_tTexturePolicy.GetRenderTexture();
	RED_ASSERT( texObject != nullptr, TXT("SpeedTree texture does not have a valid texture object") );
	if ( texObject == nullptr )
	{
		return false;
	}

	texObject->BindNoSampler( nSampler, RST_PixelShader, distance );
	return true;
}

inline CShaderTechniqueGPUAPI::CVertexShader::CVertexShader( ) 
	: m_compiledShaderCode( NULL )
	, m_compiledShaderCodeSize( 0 )
{
}


inline st_bool CShaderTechniqueGPUAPI::CVertexShader::Load(const char* pFilename, const SAppState&, const SRenderState&/*Make sure you see how we're dealing in PixelShader with potential cache error in case of using renderState for conditional compilation*/)
{
	st_bool bSuccess = false;

	if (pFilename)
	{
		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);

		// load the pre-compiled shader object file
		m_compiledShaderCodeSize = st_uint32(pFileSystem->FileSize(pFilename));
		if (m_compiledShaderCodeSize > 0)
		{
			m_compiledShaderCode = (void*) pFileSystem->LoadFile(pFilename, CFileSystem::LONG_TERM);
			if ( m_compiledShaderCode )
			{
				m_shaderRef = GpuApi::CreateShaderFromBinary( GpuApi::VertexShader, m_compiledShaderCode, m_compiledShaderCodeSize );
				if ( m_shaderRef )
				{
					bSuccess = true;
				}
				else
				{
					pFileSystem->Release((st_byte*) m_compiledShaderCode);
					m_compiledShaderCode = NULL;
					m_compiledShaderCodeSize = 0;
				}
			}
		}
	}

	return bSuccess;
}


inline st_bool CShaderTechniqueGPUAPI::CVertexShader::IsValid(void) const
{
	return !m_shaderRef.isNull();
}


inline void CShaderTechniqueGPUAPI::CVertexShader::ReleaseGfxResources()
{
	GpuApi::SafeRelease( m_shaderRef );

	if ( m_compiledShaderCode )
	{
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);
		pFileSystem->Release((st_byte*) m_compiledShaderCode);
		m_compiledShaderCode = NULL;
		m_compiledShaderCodeSize = 0;
	}
}


inline CShaderTechniqueGPUAPI::CPixelShader::CPixelShader( )
	: m_forcedNull( false )
{
}

inline st_bool CShaderTechniqueGPUAPI::CPixelShader::IsForcedNullShader( const SRenderState& sRenderState )
{
	// If this shader is only for the (solid) trunks on shadow passes, we don't actually need it. Just need to write to depth
	// buffer, and the trunks shouldn't need any discards.
	// NOTE : Distant billboard trees might be caught by this, but those don't seem to be used for shadows anyways, so it's okay.
	//Bool noClipping = !sRenderState.m_bFrondsPresent && !sRenderState.m_bLeavesPresent && !sRenderState.m_bFacingLeavesPresent;
	//return noClipping && sRenderState.m_eRenderPass == RENDER_PASS_SHADOW_CAST;

	// Revived pixel shader even for branches since we need a smooth fade
	return false;
}

inline void CShaderTechniqueGPUAPI::CPixelShader::AdaptCacheFilename( CFixedString &refCacheFilename, const SRenderState& sRenderState )
{
	if ( IsForcedNullShader( sRenderState ) )
	{
		refCacheFilename = "LavaForcedNullPS";
	}	
}

inline st_bool CShaderTechniqueGPUAPI::CPixelShader::Load(const char* pFilename, const SAppState& , const SRenderState& sRenderState)
{
	if ( IsForcedNullShader( sRenderState ) )
	{
		m_forcedNull = true;
		return true;
	}

	st_bool bSuccess = false;

	if (pFilename)
	{
		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get( );
		assert(pFileSystem);

		size_t siCompiledCodeSize = pFileSystem->FileSize(pFilename);
		if (siCompiledCodeSize > 0)
		{
			void* pCompiledShaderCode = (void*) pFileSystem->LoadFile(pFilename);
			if (pCompiledShaderCode)
			{
				// create the pixel shader if compiled shader code is available
				m_shaderRef = GpuApi::CreateShaderFromBinary( GpuApi::PixelShader, pCompiledShaderCode, static_cast< Uint32 >( siCompiledCodeSize ) );
				if ( m_shaderRef )
				{
					bSuccess = true;
				}

				pFileSystem->Release((st_byte*) pCompiledShaderCode);
			}
		}
	}

	return bSuccess;
}


inline st_bool CShaderTechniqueGPUAPI::CPixelShader::IsValid(void) const
{
	return !m_shaderRef.isNull() || m_forcedNull;
}


inline void CShaderTechniqueGPUAPI::CPixelShader::ReleaseGfxResources(void)
{
	GpuApi::SafeRelease( m_shaderRef );
}


inline st_bool CShaderTechniqueGPUAPI::Link(const CVertexShader&, const CPixelShader&)
{
	return true;
}

inline st_bool CShaderTechniqueGPUAPI::ReleaseGfxResources(void)
{
	return true;
}


inline CFixedString CShaderTechniqueGPUAPI::GetCompiledShaderExtension(void)
{
#if defined( RED_PLATFORM_WINPC )
	return "fx11obj";
#elif defined( RED_PLATFORM_DURANGO )
	return "durobj";
#elif defined( RED_PLATFORM_ORBIS )
	return "sb";
#endif

}


inline CFixedString CShaderTechniqueGPUAPI::GetCompiledShaderFolder(void)
{
#if defined( RED_PLATFORM_WINPC )
	return "shaders_directx11";
#elif defined( RED_PLATFORM_DURANGO )
	return "shaders_durango";
#elif defined( RED_PLATFORM_ORBIS )
	return "shaders_orbis";
#endif
}


inline st_bool CShaderTechniqueGPUAPI::VertexDeclNeedsInstancingAttribs(void)
{
	return true;
}


inline CShaderConstantBufferGPUAPI::CShaderConstantBufferGPUAPI( )
	: m_data( nullptr )
	, m_dataSize( 0 )
	, m_register( 0 )
{
}

inline CShaderConstantBufferGPUAPI::~CShaderConstantBufferGPUAPI( )
{
	GpuApi::SafeRelease( m_constantBuffer );
}

inline st_bool CShaderConstantBufferGPUAPI::Init(void* pLayout, size_t siSizeOfLayout, st_int32 nRegister)
{
	// Don't actually create anything. Quite often SpeedTree will initialize some buffer, but then not use it for anything.
	// Kind of a waste of memory to allocate all of these, so just wait until it's actually used.

	// copy input parameters
	m_data = pLayout;
	m_dataSize = siSizeOfLayout;
	m_register = nRegister;

	// round dataSize up to multiple of 16 if necessary (D3D11 requirement for constant buffers)
	if (m_dataSize % 16 != 0)
		m_dataSize += 16 - (m_dataSize % 16);

	return true;
}

inline void CShaderConstantBufferGPUAPI::ReleaseGfxResources(void)
{
	GpuApi::SafeRelease( m_constantBuffer );
}

inline st_bool CShaderConstantBufferGPUAPI::Update(void) const
{
	st_bool bSuccess = false;

	if ( m_constantBuffer.isNull() )
	{
		GpuApi::BufferInitData initData;
		initData.m_buffer = m_data;
		m_constantBuffer = GpuApi::CreateBuffer( (GpuApi::Uint32)m_dataSize, GpuApi::BCC_Constant, GpuApi::BUT_Dynamic, GpuApi::BAF_CPUWrite, &initData );
		if ( !m_constantBuffer.isNull())
		{
			GpuApi::SetBufferDebugPath( m_constantBuffer, "speedtree constant buffer" );
			bSuccess = true;
		}
		else
		{
			CCore::SetError("CShaderConstantBufferGPUAPI::Update(), failed to create constant buffer on first update");
		}
	}
	else if ( !m_constantBuffer.isNull() )
	{
		void* data =  GpuApi::LockBuffer( m_constantBuffer, GpuApi::BLF_Discard, 0, (GpuApi::Uint32)m_dataSize );
		if (data != nullptr)
		{
			Red::System::MemoryCopy( data, m_data, m_dataSize );
			bSuccess = true;
			GpuApi::UnlockBuffer( m_constantBuffer );
		}
		else
		{
			CCore::SetError("CShaderConstantBufferGPUAPI::Update(), DX11::DeviceContext( )->Map() failed");
		}
	}
	else
	{
		CCore::SetError("CShaderConstantBufferGPUAPI::Update(), constant buffer wasn't successfully initialized");
	}

	return bSuccess;
}

inline st_bool CShaderConstantBufferGPUAPI::Bind(void) const
{
	st_bool bSuccess = false;

	if ( !m_constantBuffer.isNull() )
	{
		GpuApi::BindConstantBuffer( m_register, m_constantBuffer, GpuApi::VertexShader );
		GpuApi::BindConstantBuffer( m_register, m_constantBuffer, GpuApi::PixelShader );

		bSuccess = true;
	}
	else
	{
		CCore::SetError("CShaderConstantBufferGPUAPI::Bind(), constant buffer wasn't successfully initialized");
	}

	return bSuccess;
}
