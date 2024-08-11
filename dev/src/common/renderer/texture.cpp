/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "speedTreeRenderInterface.h"
#include "renderTexture.h"
#include "../gpuApiUtils/gpuApiMemory.h"
#include "../../common/engine/textureCache.h"
#include "../../common/engine/texture.h"
#include "../../common/core/depot.h"

#ifdef USE_SPEED_TREE

#include "Core/Random.h"
#include "Core/PerlinNoiseKernel.h"

using namespace SpeedTree;

st_bool CTextureGPUAPI::Load(const char* pFilename, st_int32 nMaxAnisotropy)
{
	if ( pFilename == nullptr || Red::System::StringLength(pFilename) == 0 )
	{
		return false;
	}

	ASSERT( GpuApi::IsInit() );

	GpuApi::eInternalTexture fallback = GpuApi::INTERTEX_Blank2D;

	// HACK :	Textures that are ended with xyz_n.dds are normal maps and they need kinda 
	//			different fallback texture - flat normal one.
	if( const char* dotPoint = strrchr( pFilename, '.' ) )
	{
		if( dotPoint > pFilename+2 && *(dotPoint-2) == '_' && *(dotPoint-1) == 'n'  )
		{
			fallback = GpuApi::INTERTEX_FlatNormal2D;
		}
	}

	// HACK : SpeedTree fixes up paths on PS4 by prepending a '/' to anything...
	if ( pFilename[0] == '/' )
	{
		++pFilename;
	}

	// Try to create a texture with a streaming source
	{
		const String fixedPath = TextureCacheHelpers::FixNonResourcePath( ANSI_TO_UNICODE( pFilename ) );
		IBitmapTextureStreamingSource* streamingSource = new CTextureCacheStreamingSourcePC( GetHash( fixedPath ) );
		if ( streamingSource->IsReady() )
		{
			m_texture = CRenderTexture::Create( streamingSource, RED_NAME( SpeedTree ), 0, fallback );
		}

		// Release our reference, the texture will keep one itself
		streamingSource->Release();
		streamingSource = nullptr;
	}
	
	if ( m_texture == nullptr )
	{
		String depotRoot;
		GDepot->GetAbsolutePath( depotRoot );

		StringAnsi absolutePathAnsi = UNICODE_TO_ANSI( depotRoot.AsChar() );
		absolutePathAnsi += pFilename;

		// get file system pointer from Core lib
		CFileSystem* pFileSystem = CFileSystemInterface::Get();
		assert(pFileSystem);

		// set up a temporary buffer for reading the texture
		size_t siTextureFileSize = pFileSystem->FileSize(absolutePathAnsi.AsChar());
		if (siTextureFileSize > 0)
		{
			st_byte* pTextureBuffer = pFileSystem->LoadFile(absolutePathAnsi.AsChar());
			if (pTextureBuffer)
			{
				GpuApi::TextureRef gpuTexture = GpuApi::CreateTextureFromMemoryFile( pTextureBuffer, static_cast< GpuApi::Uint32 >( siTextureFileSize ), GpuApi::TEXG_Generic );

				if ( gpuTexture )
				{
					m_texture = CRenderTexture::Create( gpuTexture, 0 );
					m_texture->SetDepotPath( ANSI_TO_UNICODE( pFilename ) );
				}

				pFileSystem->Release(pTextureBuffer);
			}
		}
	}

	if ( m_texture != nullptr )
	{
		m_bIsGeneratedUniformColor = false;

		// Store dimensions
		Float w, h;
		m_texture->GetSize( w, h );
		m_sInfo.m_nWidth = static_cast< st_int32 >( w );
		m_sInfo.m_nHeight = static_cast< st_int32 >( h );
	}

	return m_texture != nullptr;
}

st_bool CTextureGPUAPI::LoadColor(st_uint32 uiColor)
{
	ASSERT( GpuApi::IsInit() );

	st_bool bSuccess = false;

	// Describe texture to create
	GpuApi::TextureDesc desc;
	desc.width = 4;
	desc.height = 4;
	desc.format = GpuApi::TEXFMT_R8G8B8X8;
	desc.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;
	desc.initLevels = 1;
	
	// Create image
	const Uint32 pitch = 4 * 4;  // width * four chars
	Uint8* texels = new Uint8[ 4 * pitch ]; // pitch * height

	// Write color data
	for (Uint32 uiRow = 0; uiRow < 4; ++uiRow)
	{
		Uint32 uiRowStart = uiRow * pitch;
		for (Uint32 uiCol = 0; uiCol < 4; ++uiCol)
		{
			Uint32 uiColStart = uiCol * 4;
			texels[uiRowStart + uiColStart + 0] = (Uint8)((uiColor & 0xff000000) >> 24); // Red
			texels[uiRowStart + uiColStart + 1] = (Uint8)((uiColor & 0x00ff0000) >> 16); // Green
			texels[uiRowStart + uiColStart + 2] = (Uint8)((uiColor & 0x0000ff00) >> 8);  // Blue
			texels[uiRowStart + uiColStart + 3] = (Uint8)((uiColor & 0x000000ff) >> 0);  // Alpha
		}
	}

	// Create texture
	GpuApi::TextureLevelInitData initMipData;
	initMipData.m_isCooked = false;
	initMipData.m_data = texels;

	GpuApi::TextureInitData initData;
	initData.m_isCooked = false;
	initData.m_mipsInitData = &initMipData;

	GpuApi::TextureRef gpuTexture = GpuApi::CreateTexture( desc, GpuApi::TEXG_Generic, &initData );

	// Delete image
	delete [] texels;
	texels = NULL;

	if ( gpuTexture )
	{
		// Set debug name
		GpuApi::SetTextureDebugPath(gpuTexture, "colorTexture(ST)");

		//Store texture info
		m_sInfo.m_nWidth = desc.width;
		m_sInfo.m_nHeight = desc.height;

		m_texture = CRenderTexture::Create( gpuTexture, 0 );

		//// Ensure proper sampler existence
		//if ( !m_pPointNoMipMapSampler )
		//{
		//	CreateSharedPointNoMipMapSampler();
		//}
		//else
		//{
		//	GpuApi::AddRef( m_pPointNoMipMapSampler );
		//}

		bSuccess = true;
	}

	m_bIsGeneratedUniformColor = true;

	return bSuccess;
}

st_bool CTextureGPUAPI::LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise)
{
	st_bool bSuccess = false;

	if (nWidth > 4 && nHeight > 4 && nWidth <= 4096 && nHeight <= 4096)
	{
		ASSERT( GpuApi::IsInit() );
		CRandom cRandom;

		// Describe texture to create
		GpuApi::TextureDesc desc;
		desc.width = nWidth;
		desc.height = nHeight;
		desc.format = GpuApi::TEXFMT_A8;
		desc.usage = GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;
		desc.initLevels = 1;


		// Lock for write
		const Uint32 pitch = nWidth;
		Uint8* texels = new Uint8[ nWidth * nHeight ];

		// Write noise
		for ( Int32 uiRow = 0; uiRow < nHeight; ++uiRow )
		{
			Uint32 uiRowStart = uiRow * pitch;
			for ( Int32 uiCol = 0; uiCol < nWidth; ++uiCol )
			{
				Uint32 uiColStart = uiCol * 1;
				texels[uiRowStart + uiColStart + 0] = (Uint8)(cRandom.GetInteger(st_int32(fLowNoise * 255), st_int32(fHighNoise * 255)));
			}
		}


		// Create texture
		GpuApi::TextureLevelInitData initMipData;
		initMipData.m_isCooked = false;
		initMipData.m_data = texels;

		GpuApi::TextureInitData initData;
		initData.m_isCooked = false;
		initData.m_mipsInitData = &initMipData;

		GpuApi::TextureRef gpuTexture = GpuApi::CreateTexture( desc, GpuApi::TEXG_Generic, &initData );

		// Delete image
		delete [] texels;
		texels = NULL;

		if ( gpuTexture )
		{
			// Set debug name
			GpuApi::SetTextureDebugPath(gpuTexture, "noiseTexture(ST)");

			m_sInfo.m_nWidth = desc.width;
			m_sInfo.m_nHeight = desc.height;

			m_texture = CRenderTexture::Create( gpuTexture, 0 );

			//// Ensure proper sampler existence
			//if ( !m_pPointNoMipMapSampler )
			//{
			//	CreateSharedPointNoMipMapSampler();
			//}
			//else
			//{
			//	GpuApi::AddRef( m_pPointNoMipMapSampler );
			//}

			bSuccess = true;
		}
	}

	m_bIsGeneratedUniformColor = true;

	return bSuccess;
}

st_bool	CTextureGPUAPI::LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth)
{
	if (nWidth > 4 && nHeight > 4 && nWidth <= 4096 && nHeight <= 4096)
	{
		st_int32 rowPitch = nWidth * 4;

		// speedtree todo: comment this stuff
		CPerlinNoiseKernel cKernel(nWidth);

#define XXX 1.0 // speedtree todo
		const st_float32 c_afSampleOffsets[4][2] = 
		{
			// speedtree todo
			{ -0.5f * XXX, -0.5f * XXX },
			{ -0.5f * XXX,  0.5f * XXX },
			{  0.5f * XXX, -0.5f * XXX },
			{  0.5f * XXX,  0.5f * XXX }
		};

		Uint8* data = new Uint8[ nWidth * nHeight * 4 ];

		for (st_int32 uiRow = 0; uiRow < nHeight; ++uiRow)
		{
			Uint8* pTexels = data + uiRow * rowPitch;
			for (st_int32 uiCol = 0; uiCol < nWidth; ++uiCol)
			{
				*pTexels++ = Uint8(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[0][0], uiRow + c_afSampleOffsets[0][1])); // red
				*pTexels++ = Uint8(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[1][0], uiRow + c_afSampleOffsets[1][1])); // green
				*pTexels++ = Uint8(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[2][0], uiRow + c_afSampleOffsets[2][1])); // blue
				*pTexels++ = Uint8(255 * cKernel.BilinearSample(uiCol + c_afSampleOffsets[3][0], uiRow + c_afSampleOffsets[3][1])); // alpha
			}
		}

		GpuApi::TextureDesc textureDesc;
		textureDesc.width = nWidth;
		textureDesc.height = nHeight;
		textureDesc.initLevels = 1;
		textureDesc.format = GpuApi::TEXFMT_R8G8B8A8;
		textureDesc.msaaLevel = 0;
		textureDesc.usage = GpuApi::TEXUSAGE_Samplable;
		textureDesc.type = GpuApi::TEXTYPE_2D;
		textureDesc.sliceNum = 1;

		GpuApi::TextureLevelInitData initData;
		initData.m_data = data;
		initData.m_isCooked = false;

		GpuApi::TextureRef gpuTexture  = CreateTexture( textureDesc, GpuApi::TEXG_Generic );
		GpuApi::LoadTextureData2D( gpuTexture, 0, 0, nullptr, data, rowPitch );
		GpuApi::SetTextureDebugPath( gpuTexture, "perlinNoise" );

		delete [] data;

		if ( !gpuTexture.isNull() )
		{
			m_texture = CRenderTexture::Create( gpuTexture, 0 );
			return true;
		}
	}

	return false;
}

void CTextureGPUAPI::SetSamplerStates(void)
{
	GpuApi::SetSamplerStatePreset( 0 , GpuApi::SAMPSTATEPRESET_SpeedTreeStandardSampler ,			GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 1 , GpuApi::SAMPSTATEPRESET_SpeedTreeShadowMapCompareSampler ,	GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 2 , GpuApi::SAMPSTATEPRESET_SpeedTreePointSampler ,				GpuApi::PixelShader );
	GpuApi::SetSamplerStatePreset( 3 , GpuApi::SAMPSTATEPRESET_SpeedTreeLinearClampSampler ,		GpuApi::PixelShader );

	GpuApi::SetSamplerStatePreset( 0 , GpuApi::SAMPSTATEPRESET_SpeedTreeStandardSampler ,			GpuApi::VertexShader );
	GpuApi::SetSamplerStatePreset( 1 , GpuApi::SAMPSTATEPRESET_SpeedTreeShadowMapCompareSampler ,	GpuApi::VertexShader );
	GpuApi::SetSamplerStatePreset( 2 , GpuApi::SAMPSTATEPRESET_SpeedTreePointSampler ,				GpuApi::VertexShader );
	GpuApi::SetSamplerStatePreset( 3 , GpuApi::SAMPSTATEPRESET_SpeedTreeLinearClampSampler ,		GpuApi::VertexShader );
}

st_bool CTextureGPUAPI::ReleaseGfxResources(void)
{
	SAFE_RELEASE( m_texture );

	return true;
}

//void CTextureGPUAPI::CreateSharedFilteredMipMapSampler(st_int32 nMaxAnisotropy)
//{
//	GpuApi::SamplerStateDesc desc;
//	if ( nMaxAnisotropy > 0 )
//	{
//		desc.filterMag = GpuApi::TEXFILTERMAG_Linear;
//		desc.filterMin = GpuApi::TEXFILTERMIN_Aniso;
//		desc.filterMip = GpuApi::TEXFILTERMIP_Linear;
//	}
//	else
//	{
//		desc.filterMag = GpuApi::TEXFILTERMAG_Linear;
//		desc.filterMin = GpuApi::TEXFILTERMIN_Linear;
//		desc.filterMip = GpuApi::TEXFILTERMIP_Linear;
//	}
//	
//	desc.addressU = GpuApi::TEXADDR_Wrap;
//	desc.addressV = GpuApi::TEXADDR_Wrap;
//	desc.addressW = GpuApi::TEXADDR_Wrap;
//	
//	m_pFilteredMipMapSampler = GpuApi::RequestSamplerState( desc );
//}
//
//void CTextureGPUAPI::CreateSharedPointNoMipMapSampler()
//{
//	GpuApi::SamplerStateDesc desc;
//	desc.filterMag = GpuApi::TEXFILTERMAG_Point;
//	desc.filterMin = GpuApi::TEXFILTERMIN_Point;
//	desc.filterMip = GpuApi::TEXFILTERMIP_Point;
//	desc.addressU = GpuApi::TEXADDR_Wrap;
//	desc.addressV = GpuApi::TEXADDR_Wrap;
//	desc.addressW = GpuApi::TEXADDR_Wrap;
//
//	m_pPointNoMipMapSampler = GpuApi::RequestSamplerState( desc );
//}
#endif
