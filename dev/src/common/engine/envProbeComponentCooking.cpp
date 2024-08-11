/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "envProbeComponent.h"
#include "textureCache.h"
#include "../core/cooker.h"

// Including from renderer for some helpful functions and enums. Otherwise, code duplication. Only for cooking stuff, so shouldn't be a problem :)
#include "../renderer/renderEnvProbeManager.h"


#ifndef NO_TEXTURECACHE_COOKER

//////////////////////////////////////////////////////////////////////////
// Simple pass-through texture source for a single face in one of the envprobe textures.
class CTextureBakerSourceEnvProbe : public ITextureBakerSource, public Red::System::NonCopyable
{
private:
	eEnvProbeBufferTexture _type;
	const void* _data;

public:
	CTextureBakerSourceEnvProbe( eEnvProbeBufferTexture type, const void* sourceData )
		: _type( type )
		, _data( sourceData )
	{
	}

	virtual Uint16 GetSliceCount() const { return 6; }
	virtual Uint16 GetMipCount() const { return 1; }

	virtual const void* GetMipData( Uint16 /*mip*/, Uint16 slice ) const
	{
		const Uint32 faceSize = GetMipDataSize( 0, 0 );
		return OffsetPtr( _data, faceSize * slice );
	}

	virtual Uint32 GetMipDataSize( Uint16 /*mip*/, Uint16 /*slice*/ ) const
	{
		return GetMipPitch( 0, 0 ) * GetBaseHeight();
	}

	virtual Uint32 GetMipPitch( Uint16 /*mip*/, Uint16 /*slice*/ ) const
	{
		return GpuApi::CalculateTexturePitch( GetBaseWidth(), GetTextureFormat() );
	}

	virtual Uint16 GetBaseWidth() const
	{
		return (Uint16)GetEnvProbeDataSourceResolution();
	}

	virtual Uint16 GetBaseHeight() const
	{
		return (Uint16)GetEnvProbeDataSourceResolution();
	}

	virtual GpuApi::eTextureFormat GetTextureFormat() const
	{
		return ENVPROBEBUFFERTEX_Depth == _type ? GpuApi::TEXFMT_Float_R16 : GpuApi::TEXFMT_R8G8B8A8;
	}

	virtual GpuApi::eTextureType GetTextureType() const { return GpuApi::TEXTYPE_ARRAY; }

	virtual Bool IsLooseFileTexture() const { return false; }
};



Bool CEnvProbeComponent::ExtractEnvProbesToTextureCache( ECookingPlatform platform, const BufferHandle& loadedData ) const
{
	CAsyncTextureBaker::CookFunctionPtr cookerFunction = GTextureCacheCooker->GetDefaultCookFunction( platform );
	if ( cookerFunction == nullptr )
	{
		RED_HALT( "Could not select appropriate texture cook function" );
		return false;
	}


	const Uint32 res = GetEnvProbeDataSourceResolution();
	TDynArray< Uint8, MC_Temporary > tempBuffer;

	String path = GenerateTextureCachePath();

	for ( Uint32 texType = 0; texType < ENVPROBEBUFFERTEX_MAX; ++texType )
	{
		const Uint32 elementSize = ENVPROBEBUFFERTEX_Depth == texType ? 2 : 4;

		const Uint32 faceSize = elementSize * res * res;

		tempBuffer.ResizeFast( faceSize * 6 );

		for ( Uint32 face = 0; face < 6; ++face )
		{
			Uint8* tempFacePtr = tempBuffer.TypedData() + faceSize * face;

			const void* sourceData = OffsetPtr( loadedData->GetData(), GetEnvProbeDataSourceBufferOffset( face, (eEnvProbeBufferTexture)texType ) );

			if ( ENVPROBEBUFFERTEX_Albedo == texType )
			{
				const Uint8* sourceBytes = (const Uint8*)sourceData;
				// ace_ibl_optimize
				for ( Uint32 i=0, num=res*res; i<num; ++i )
				{
					tempFacePtr[4*i]   = sourceBytes[2*i];
					tempFacePtr[4*i+1] = sourceBytes[2*i+1];
					tempFacePtr[4*i+2] = 0;
					tempFacePtr[4*i+3] = 0;
				}
			}
			else if ( ENVPROBEBUFFERTEX_Normals == texType )
			{
				const Uint8* sourceBytes = (const Uint8*)sourceData;
				// ace_ibl_optimize
				for ( Uint32 i=0, num=res*res; i<num; ++i )
				{
					tempFacePtr[4*i]   = sourceBytes[3*i];
					tempFacePtr[4*i+1] = sourceBytes[3*i+1];
					tempFacePtr[4*i+2] = sourceBytes[3*i+2];
					tempFacePtr[4*i+3] = 0;
				}
			}
			else
			{
				Red::System::MemoryCopy( tempFacePtr, sourceData, faceSize );
			}
		}

		CTextureBakerSourceEnvProbe textureSource( (eEnvProbeBufferTexture)texType, tempBuffer.Data() );

		Uint32 hash = GenerateTextureCacheHash( (eEnvProbeBufferTexture)texType );
		GTextureCacheCooker->StoreTextureData( hash, path, textureSource, cookerFunction );
	}

	return true;
}

void CEnvProbeComponent::DumpToTextureCache( ECookingPlatform platform ) const
{
	if ( GTextureCacheCooker == nullptr )
	{
		RED_HALT( "No TextureCacheCooker" );
		return;
	}

	BufferHandle handle = m_facesData.AcquireBufferHandleSync();

	if ( CRenderEnvProbeManager::GetEnvProbeDataSourceSize() == m_facesData.GetSize() && handle )
	{
		if ( !ExtractEnvProbesToTextureCache( platform, handle ) )
		{
			ERR_ENGINE( TXT("Failed to extract envprobe data") );
		}
	}
}


String CEnvProbeComponent::GenerateTextureCachePath() const
{
	// Resource path for whatever resource owns this envprobe (probably a CLayer, but maybe not?).
	String resourcePath;
	CObject* parent = GetParent();
	while ( parent != nullptr )
	{
		if ( CResource* parentResource = Cast< CResource >( parent ) )
		{
			resourcePath = parentResource->GetDepotPath();
			break;
		}
		parent = parent->GetParent();
	}

	RED_ASSERT( !resourcePath.Empty(), TXT("Couldn't find a CResource parent for envprobe component: %ls"), GetFriendlyName().AsChar() );

	return resourcePath;
}

Uint32 CEnvProbeComponent::GenerateTextureCacheHash( eEnvProbeBufferTexture tex ) const
{
	// Needs to be unique.
	String uniqueName = String::Printf( TXT("{{env:%ls}}:%u"), GetFriendlyName().AsChar(), (Uint32)tex );
	return GetHash( uniqueName );
}

#endif // !NO_TEXTURECACHE_COOKER


#ifndef NO_RESOURCE_COOKING

void CEnvProbeComponent::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

#ifndef NO_TEXTURECACHE_COOKER
	// We don't cook the texture data here, but leave it for the texture cache. We could do the cooking here directly,
	// and store it locally in a deferred buffer of some sort, except that DeferredDataBuffer isn't sufficient for in-place
	// texture creation (which is required for cooked textures on Xbox), and GpuDataBuffer assumes the data should be loaded
	// immediately (which we don't want, we want to stream it in later as needed). GpuDataBuffer could be expanded to support
	// not loading at creation, but this is perhaps good enough.

	// Store hashes used for texture cache? Or should we just generate them at runtime?
	m_textureCacheHashes.Resize( ENVPROBEBUFFERTEX_MAX );
	for ( Uint32 texType = 0; texType < ENVPROBEBUFFERTEX_MAX; ++texType )
	{
		m_textureCacheHashes[texType] = GenerateTextureCacheHash( (eEnvProbeBufferTexture)texType );
	}
#endif
}

#endif // !NO_RESOURCE_COOKING
