/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderCube.h"
#include "renderHelpers.h"
#include "../../common/engine/cubeTexture.h"


// Not actually "cooked"... it could be fully cooked, or it might be the uncooked/cached cube data...
static GpuApi::TextureRef LoadFromCookedData( const CCubeTexture::CookedData& cookedData )
{
	// Decode texture format
	GpuApi::eTextureFormat cubeFormat = cookedData.m_header.GetTextureFormat();
	if ( cubeFormat == GpuApi::TEXFMT_Max )
	{
		LOG_RENDERER( TXT("Invalid cube format") );
		return GpuApi::TextureRef();
	}

	// Create texture
	const Uint16 residentMipCount = cookedData.m_header.m_mipCount - cookedData.m_header.m_residentMip;

	GpuApi::TextureDesc desc;
	desc.type		= GpuApi::TEXTYPE_CUBE;
	desc.width		= GpuApi::CalculateTextureMipDimension( cookedData.m_header.m_edgeSize, (Uint8)cookedData.m_header.m_residentMip, cubeFormat );
	desc.height		= GpuApi::CalculateTextureMipDimension( cookedData.m_header.m_edgeSize, (Uint8)cookedData.m_header.m_residentMip, cubeFormat );
	desc.initLevels	= residentMipCount;
	desc.format		= cubeFormat;
	desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;

	GpuApi::TextureRef texture;
	GpuApi::TextureInitData initData;

	TDynArray< GpuApi::TextureLevelInitData > initMipData;

	GpuDataBuffer& nonConstBuffer = const_cast< GpuDataBuffer& >( cookedData.m_deviceData );

	if ( cookedData.m_header.IsCooked() )
	{
		if ( !nonConstBuffer.GetDataHandle().IsValid() )
		{
			WARN_RENDERER( TXT("Cooked data is not loaded. Loading now.")/*, engineTexture->GetFriendlyName().AsChar()*/ );
			nonConstBuffer.Load();
		}

		initData.m_isCooked = true;
		initData.m_cookedData = nonConstBuffer.GetDataHandle();

		desc.inPlaceType = GpuApi::INPLACE_Texture;
	}
	else
	{
		if ( !cookedData.m_deviceData.GetDataHandle().IsValid() )
		{
			LOG_RENDERER( TXT("Cube has no data") );
			return GpuApi::TextureRef();
		}

		const void* dataPtr = cookedData.m_deviceData.GetDataHandle().GetRawPtr();

		initMipData.Resize( 6 * residentMipCount );

		for ( Uint16 face_i = 0; face_i < 6; ++face_i )
		{
			for ( Uint16 mip_i = 0; mip_i < residentMipCount; ++mip_i )
			{
				const Uint32 dataIndex = mip_i + face_i * residentMipCount;
				initMipData[dataIndex].m_isCooked = cookedData.m_header.IsCooked();
				initMipData[dataIndex].m_data = dataPtr;

				const Uint32 mipWidth	= GpuApi::CalculateTextureMipDimension( desc.width, (Uint8)mip_i, cubeFormat );
				const Uint32 mipHeight	= GpuApi::CalculateTextureMipDimension( desc.height, (Uint8)mip_i, cubeFormat );
				const Uint32 mipDataSize = GpuApi::CalculateTextureSize( mipWidth, mipHeight, cubeFormat );

				dataPtr = OffsetPtr( dataPtr, mipDataSize );
			}
		}

		initData.m_isCooked = false;
		initData.m_mipsInitData = initMipData.TypedData();
	}

	texture = GpuApi::CreateTexture( desc, GpuApi::TEXG_Generic, &initData );

	if ( texture )
	{
		if ( cookedData.m_header.IsCooked() )
		{
			nonConstBuffer.UnlinkDataHandle();
		}
	}
	else
	{
		WARN_RENDERER( TXT("Failed to create cube texture %ux%ux%u"), cookedData.m_header.m_edgeSize, cookedData.m_header.m_edgeSize, cookedData.m_header.m_edgeSize );
	}

	return texture;
}


CRenderCubeTexture::CRenderCubeTexture( const GpuApi::TextureRef &texture, const String& displayableName )
	: CRenderTextureBase( texture )
	, m_displayableName( displayableName )
{
}

CRenderCubeTexture::~CRenderCubeTexture()
{
}

CName CRenderCubeTexture::GetCategory() const
{
	return RED_NAME( RenderCubeTexture );
}


CRenderCubeTexture* CRenderCubeTexture::Create( const CCubeTexture* engineTexture, Uint64 partialRegistrationHash )
{
	TScopedRenderResourceCreationObject< CRenderCubeTexture > texture ( partialRegistrationHash );

	RED_WARNING( engineTexture != nullptr, "Cannot create a render cube without engine texture." );
	if ( engineTexture == nullptr )
	{
		return nullptr;
	}

	texture.InitResource( new CRenderCubeTexture( GpuApi::TextureRef::Null(), engineTexture->GetFriendlyName() ) );
	
	const CCubeTexture::CookedData& cookedData = engineTexture->GetCookedData();

	Uint8 residentMip = (Uint8)cookedData.m_header.m_residentMip;
	Uint8 maxStreamMip = 0;

	// The texture's cooked data always contains at least uncooked cached data, so we can just load it directly
	GpuApi::TextureRef gpuTexture = LoadFromCookedData( cookedData );

	if ( gpuTexture.isNull() )
	{
		WARN_RENDERER( TXT("GpuApi: Failed to create render cube, %ls"), engineTexture->GetFriendlyName().AsChar() );
		return nullptr;
	}

	texture->InitWithGpuTexture( gpuTexture );
	texture->SetDepotPath( engineTexture->GetDepotPath() );
	texture->InitStreaming( residentMip, maxStreamMip, engineTexture->GetStreamingSource() );

	return texture.RetrieveSuccessfullyCreated();
}


IRenderResource* CRenderInterface::UploadCube( const CCubeTexture* texture )
{
	ASSERT( !IsDeviceLost(), TXT("Unable to create new render resources when device is lost") );

	if ( IsDeviceLost() )
	{
		return nullptr;
	}

	CRenderCubeTexture* renderTexture = nullptr;

	if ( CanUseResourceCache() )
	{
		const Uint64 hash = CRenderCubeTexture::CalcResourceHash( texture );
		if ( CRenderCubeTexture::ResourceCacheRequestPartialCreate( hash, renderTexture ) )
		{
			renderTexture = CRenderCubeTexture::Create( texture, hash );
		}
	}
	else
	{
		renderTexture = CRenderCubeTexture::Create( texture, 0 );
	}

	return renderTexture;
}
