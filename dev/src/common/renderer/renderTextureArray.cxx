/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderTextureArray.h"
#include "renderHelpers.h"
#include "../engine/textureArray.h"


static GpuApi::TextureRef LoadTextureArrayFromCookedEngineTexture( const CTextureArray* engineTexture, Uint8 residentMip )
{
	const CTextureArray::CookedData& cookedData = engineTexture->GetCookedData();

	GpuDataBuffer& nonConstBuffer = const_cast< GpuDataBuffer& >( cookedData.m_deviceData );
	if ( !nonConstBuffer.GetDataHandle().IsValid() )
	{
		WARN_RENDERER( TXT("Cooked data is not loaded. Loading now. %ls"), engineTexture->GetFriendlyName().AsChar() );
		nonConstBuffer.Load();
	}

	// Create texture
	GpuApi::TextureDesc desc;
	desc.type			= GpuApi::TEXTYPE_ARRAY;
	desc.format			= cookedData.m_header.GetTextureFormat();
	desc.width			= GpuApi::CalculateTextureMipDimension( cookedData.m_header.m_width, residentMip, desc.format );
	desc.height			= GpuApi::CalculateTextureMipDimension( cookedData.m_header.m_height, residentMip, desc.format );
	desc.initLevels		= cookedData.m_header.m_mipCount - residentMip;
	desc.sliceNum		= cookedData.m_header.m_textureCount;
	desc.usage			= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;
	desc.inPlaceType	= GpuApi::INPLACE_Texture;

	GpuApi::TextureInitData initData;
	initData.m_isCooked = true;
	initData.m_cookedData = nonConstBuffer.GetDataHandle();

	GpuApi::TextureRef texture = GpuApi::CreateTexture( desc, GpuApi::TEXG_Generic, &initData );
	if ( texture )
	{
		nonConstBuffer.UnlinkDataHandle();
	}
	else
	{
		WARN_RENDERER( TXT("Failed to create texture array %ix%i[%i], %ls"), desc.width, desc.height, desc.sliceNum, engineTexture->GetFriendlyName().AsChar() );
	}


	return texture;
}


static GpuApi::TextureRef LoadTextureArrayFromEngineTexture( const CTextureArray* engineTexture, Uint8 residentMip )
{
	// get source textures
	TDynArray< CBitmapTexture* > textures;
	engineTexture->GetTextures( textures );

	const Uint16 textureCount		= (Uint16)textures.Size();
	const Uint16 residentMipCount	= engineTexture->GetMipCount() - residentMip;
	
	// Create texture
	GpuApi::TextureDesc desc;
	desc.type		= GpuApi::TEXTYPE_ARRAY;
	desc.format		= engineTexture->GetPlatformSpecificCompression();
	desc.width		= GpuApi::CalculateTextureMipDimension( engineTexture->GetWidth(), residentMip, desc.format );
	desc.height		= GpuApi::CalculateTextureMipDimension( engineTexture->GetHeight(), residentMip, desc.format );
	desc.initLevels	= residentMipCount;
	desc.sliceNum	= textureCount;
	desc.usage		= GpuApi::TEXUSAGE_Samplable | GpuApi::TEXUSAGE_Immutable;


	TDynArray< GpuApi::TextureLevelInitData > initMipData( textureCount * residentMipCount );

	for ( Uint32 tex_i = 0; tex_i < textureCount; ++tex_i )
	{
		CBitmapTexture* tex = textures[ tex_i ];

		RED_WARNING( tex != nullptr, "Texture %d in texture array is null, %ls", tex_i, engineTexture->GetFriendlyName().AsChar() );
		if ( tex == nullptr )
		{
			continue;
		}

		// It's possible that an uncooked texture array references a cooked texture.
		if ( tex->GetCookedData().GetSize() > 0 )
		{
			Red::MemoryFramework::MemoryRegionHandle cookedData = tex->GetCookedData().GetDataHandle();
			for ( Uint32 mip_i = 0; mip_i < residentMipCount; ++mip_i )
			{
				Uint32 mipOffset = 0;
				RED_VERIFY( GpuApi::CalculateCookedTextureMipOffsetAndSize( desc, mip_i, tex_i, &mipOffset, nullptr ) );

				initMipData[mip_i + tex_i * residentMipCount].m_isCooked = true;
				initMipData[mip_i + tex_i * residentMipCount].m_data = OffsetPtr( cookedData.GetRawPtr(), mipOffset );
			}
		}
		else
		{
			// Get mipmaps
			const CBitmapTexture::MipArray& texMips = tex->GetMips();
			const CBitmapTexture::MipMap* lockMipChain = &texMips[ residentMip ];

			// Upload data
			for ( Uint32 mip_i = 0; mip_i < residentMipCount; ++mip_i )
			{
				// Get mipmap data
				CBitmapTexture::MipMap& mip = const_cast< CBitmapTexture::MipMap& >( lockMipChain[ mip_i ] );
				if ( !mip.m_data.Load() )
				{
					RED_HALT( "Failed to load mip %d, %ls", mip_i + residentMip, engineTexture->GetFriendlyName().AsChar() );
					continue;
				}


				void* mipData = mip.m_data.GetData();

				initMipData[ mip_i + tex_i * residentMipCount ].m_isCooked = false;
				initMipData[ mip_i + tex_i * residentMipCount ].m_data = mipData;
			}
		}
	}


	GpuApi::TextureInitData initData;
	initData.m_isCooked = false;
	initData.m_mipsInitData = initMipData.TypedData();


	GpuApi::TextureRef texture = GpuApi::CreateTexture( desc, GpuApi::TEXG_Generic, &initData );
	if ( texture.isNull() )
	{
		WARN_RENDERER( TXT("Failed to create texture array %ix%i[%i], %ls"), desc.width, desc.height, desc.sliceNum, engineTexture->GetFriendlyName().AsChar() );
	}

	return texture;
}


CRenderTextureArray::CRenderTextureArray( const GpuApi::TextureRef &texture )
	: CRenderTexture( texture )
{
}

CRenderTextureArray::~CRenderTextureArray()
{
}

IRenderResource* CRenderInterface::UploadTextureArray( const CTextureArray* textureArray )
{
	RED_WARNING( !IsDeviceLost(), "Unable to create new render resources when device is lost" );
	if ( IsDeviceLost() )
	{
		return nullptr;
	}

	CRenderTextureArray* renderTexture = nullptr;

	if ( CanUseResourceCache() )
	{			
		const Uint64 hash = CRenderTextureArray::CalcResourceHash( textureArray );
		if ( CRenderTextureArray::ResourceCacheRequestPartialCreate( hash, renderTexture ) )
		{
			renderTexture = CRenderTextureArray::Create( textureArray, hash );
		}
	}
	else
	{
		renderTexture = CRenderTextureArray::Create( textureArray, 0 );
	}

	return renderTexture;
}

CRenderTextureArray* CRenderTextureArray::Create( const CTextureArray* engineTexture, Uint64 partialRegistrationHash )
{
	TScopedRenderResourceCreationObject< CRenderTextureArray > texture ( partialRegistrationHash );

	RED_WARNING( engineTexture != nullptr, "Can't create a RenderTexture when we don't have an Engine Texture!" );
	if ( engineTexture == nullptr )
	{
		return nullptr;
	}

	RED_WARNING( GpuApi::IsInit(), "Can't create a render texture before GpuApi has been initialized! %ls", engineTexture->GetFriendlyName().AsChar() );
	if ( !GpuApi::IsInit() )
	{
		return nullptr;
	}

	texture.InitResource( new CRenderTextureArray( GpuApi::TextureRef::Null() ) );
	
	Uint8 residentMip = (Uint8)engineTexture->GetResidentMipIndex();
	Uint8 maxStreamMip = 0;

	const Uint16 mipCount = engineTexture->GetMipCount();

	{
		const Uint32 width		= engineTexture->GetWidth();
		const Uint32 height		= engineTexture->GetHeight();
		CName textureGroupName	= engineTexture->GetTextureGroupName();
		CRenderTextureBase::CalcMipStreaming( textureGroupName, width, height, mipCount, &residentMip, &maxStreamMip );
	}

	GpuApi::TextureRef gpuTexture;

	if ( engineTexture->IsCooked() )
	{
		RED_ASSERT( engineTexture->GetCookedData().m_deviceData.GetSize() > 0, TXT("Cooked array texture '%ls' has no cooked data"), engineTexture->GetDepotPath().AsChar() );

		// We can use the cooked resident mip.
		gpuTexture = LoadTextureArrayFromCookedEngineTexture( engineTexture, residentMip );
	}
	else
	{
		// Some basic validation of the texture data.
		{
			TDynArray< CBitmapTexture* > textures;
			engineTexture->GetTextures( textures );

			const Uint32 textureCount = textures.Size();
			const GpuApi::eTextureFormat texFormat = engineTexture->GetPlatformSpecificCompression();

			RED_WARNING( textureCount > 0, "Cannot create texture array with no textures, %ls", engineTexture->GetFriendlyName().AsChar() );
			if ( textureCount == 0 ) { return nullptr; }

			RED_WARNING( mipCount > 0, "Cannot create texture array with no mips, %ls", engineTexture->GetFriendlyName().AsChar() );
			if ( mipCount == 0 ) { return nullptr; }

			const Uint32 firstWidth = textures[0]->GetWidth();
			const Uint32 firstHeight = textures[0]->GetHeight();

			for ( Uint32 tex_i = 0; tex_i < textureCount; ++tex_i )
			{
				CBitmapTexture* tex = textures[ tex_i ];

				if ( tex == nullptr )
				{
					continue;
				}

				const GpuApi::eTextureFormat fmt = tex->GetPlatformSpecificCompression();
				RED_WARNING( texFormat == fmt, "Texture array format mismatch %d != %d, %ls", texFormat, fmt, engineTexture->GetFriendlyName().AsChar() );
				if ( texFormat != fmt ) { return nullptr; }

				RED_WARNING( tex->GetMipCount() == mipCount, "Texture array mipcount mismatch %u != %u, %ls", tex->GetMipCount(), mipCount, engineTexture->GetFriendlyName().AsChar() );
				if ( tex->GetMipCount() != mipCount ) { return nullptr; }

				RED_WARNING( tex->GetWidth() == firstWidth && tex->GetHeight() == firstHeight, "Texture array size mismatch %ux%u != %ux%u, %ls",
					tex->GetWidth(), tex->GetHeight(), firstWidth, firstHeight, engineTexture->GetFriendlyName().AsChar() );
				if ( tex->GetWidth() != firstWidth || tex->GetHeight() != firstHeight ) { return nullptr; }


				// Find the maximum resident mip index. This way, we're guaranteed that every texture will have resident data available.
				residentMip = Max( residentMip, tex->GetResidentMipIndex() );
			}
		}

		gpuTexture = LoadTextureArrayFromEngineTexture( engineTexture, residentMip );
	}

	if ( gpuTexture.isNull() )
	{
		return nullptr;
	}

	texture->InitWithGpuTexture( gpuTexture );
	texture->SetDepotPath( engineTexture->GetDepotPath() );
	texture->SetTextureGroup( engineTexture->GetTextureGroupName() );

	Bool doInitStreaming = true;
	// m_umbraId is calculated based on path in SetDepotPath
	static Uint32 brokenResourcePathHash = GetHash(String(TXT("dlc\\bob\\data\\characters\\models\\common\\textures\\detailmaps_pbr\\detail_maps.texarray")));
	if ( texture->m_umbraId == brokenResourcePathHash )
	{
		if (engineTexture->GetStreamingSource() && !engineTexture->GetStreamingSource()->IsReady())
		{
			texture->InitStreaming( residentMip, maxStreamMip, nullptr );
			doInitStreaming = false;
		}
	}
	if ( doInitStreaming )
	{
		texture->InitStreaming( residentMip, maxStreamMip, engineTexture->GetStreamingSource() );
	}

	return texture.RetrieveSuccessfullyCreated();
}
