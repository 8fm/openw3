/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "gpuDataBuffer.h"
#include "../core/resource.h"
#include "bitmapTexture.h"
#include "textureCache.h"

/// Atlas entry
class CTextureArrayEntry
{
	DECLARE_RTTI_STRUCT( CTextureArrayEntry );

public:
	TSoftHandle< CBitmapTexture >	m_texture;			//!< Source texture
	
public:
	// Constructor
	RED_INLINE CTextureArrayEntry() {}

	RED_INLINE CTextureArrayEntry( CBitmapTexture* texture ) { m_texture = TSoftHandle< CBitmapTexture >( texture ); }
};

BEGIN_CLASS_RTTI( CTextureArrayEntry );
PROPERTY_EDIT_NAME( m_texture, TXT("m_texture"), TXT("Source texture") );
END_CLASS_RTTI();

/// Bitmap based texture
class CTextureArray : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CTextureArray, CResource, "texarray", "Texture Array" );

public:
	//! Resource factory data
	class FactoryInfo : public CResource::FactoryInfo< CTextureArray >
	{
	public:
		ETextureRawFormat				m_format;			//!< Source data format ( should match data stored in mipmaps )
		ETextureCompression				m_compression;		//!< Target compression

	public:
		RED_INLINE FactoryInfo()
			: m_format( TRF_TrueColor )
			, m_compression( TCM_DXTNoAlpha )
		{}
	};


	struct CookedDataHeader
	{
		Uint32		m_textureCacheKey;			//!< Key for lookup in texture cache.
		Uint16		m_encodedFormat;
		Uint16		m_width;					//!< Width of mip 0.
		Uint16		m_height;					//!< Height of mip 0.
		Uint16		m_textureCount;				//!< Number of texture slices.
		Uint16		m_mipCount;					//!< Number of mip levels.
		Uint16		m_residentMip;				//!< Highest mip which is resident. Anything higher must come from texture cache.

		//! Get the actual GpuApi format for this texture.
		GpuApi::eTextureFormat GetTextureFormat() const;

#ifndef NO_TEXTURECACHE_COOKER
		Bool SetTextureFormat( GpuApi::eTextureFormat format );
#endif
	};

	struct CookedData
	{
		// TODO : Possibly switch this for a DataBuffer, so it can be empty on uncooked arrays?
		CookedDataHeader	m_header;				//!< Header, containing information about the cooked data.
		GpuDataBuffer		m_deviceData;			//!< The actual texture data. If empty, then texture is uncooked.

		CookedData()
			: m_deviceData( GpuApi::INPLACE_Texture )
		{
		}
	};



protected:
	IRenderResource*					m_renderResource;				// Rendering resource
	TDynArray< CTextureArrayEntry >		m_bitmaps;
	CName								m_textureGroup;
	Int32								m_pcDownscaleBias;				// Bias used when downscaling a texture
	Int32								m_xboneDownscaleBias;			// Bias used when downscaling a texture
	Int32								m_ps4DownscaleBias;				// Bias used when downscaling a texture
#ifndef NO_TEXTURECACHE_COOKER
	ETextureCompression					m_compression;					// Target compression
	Bool								m_dirty;
#endif
	GpuApi::eTextureFormat				m_platformSpecificCompression;

	IBitmapTextureStreamingSource*		m_streamingSource;				// Streaming source


	CookedData							m_cookedData;


public:
	// Create texture array
	static CTextureArray* Create( const FactoryInfo& data );


	CTextureArray();
	virtual ~CTextureArray();

	RED_FORCE_INLINE GpuApi::eTextureFormat GetPlatformSpecificCompression() const { return m_platformSpecificCompression; }

	RED_INLINE Uint32 CalcTextureCacheKey() const { return GetHash( GetDepotPath() ); }

	RED_FORCE_INLINE IBitmapTextureStreamingSource* GetStreamingSource() const { return m_streamingSource; }

	//! Get downscale factor for PC
	RED_FORCE_INLINE Uint32 GetPCDownscaleBias() const { return m_pcDownscaleBias; }
	//! Get downscale factor for XBone
	RED_FORCE_INLINE Uint32 GetXBoneDownscaleBias() const { return m_xboneDownscaleBias; }
	//! Get downscale factor for PS4
	RED_FORCE_INLINE Uint32 GetPS4DownscaleBias() const { return m_ps4DownscaleBias; }

	//! Change downscale factor for PC
	RED_FORCE_INLINE void SetPCDownscaleBias( Uint32 value ) { m_pcDownscaleBias = value; }
	//! Change downscale factor for XBone
	RED_FORCE_INLINE void SetXBoneDownscaleBias( Uint32 value ) { m_xboneDownscaleBias = value; }
	//! Change downscale factor for PS4
	RED_FORCE_INLINE void SetPS4DownscaleBias( Uint32 value ) { m_ps4DownscaleBias = value; }

	//! Get additional resource info, displayed in editor
	virtual void GetAdditionalInfo( TDynArray< String >& info ) const override;

	//! Property was changed
	virtual void OnPropertyPostChange( IProperty* property ) override;

	//! Serialize data
	virtual void OnSerialize( IFile& file ) override;

#ifndef NO_RESOURCE_COOKING
	Bool CanCook( String& outReason, const TDynArray< CBitmapTexture* >* bitmapOverrides = nullptr ) const;
	virtual void OnCook( ICookerFramework& cooker ) override;
	virtual Bool UncookData( const TextureCacheEntry& textureEntry, BufferHandle textureData, const String& sourceFile, const String& depotPath );
#ifndef NO_TEXTURECACHE_COOKER
	void HandleCookError( CAsyncTextureBaker::CookFunctionPtr cookFunction );
#endif

	Uint32 GetMipCountForCookingPlatform( ECookingPlatform platform ) const;
	Uint32 GetHighestMipForCookingPlatform( ECookingPlatform platform ) const;
	// Get the mip level that should remain resident, relative to the highest cooked mip.
	Uint32 GetResidentMipForCookingPlatform( ECookingPlatform platform ) const;
#endif

	//! Resource was loaded from file
	virtual void OnPostLoad() override;

	virtual void OnSave() override;

#ifndef NO_DATA_VALIDATION
	virtual void OnCheckDataErrors() const override;
#endif


	RED_INLINE const CookedData& GetCookedData() const { return m_cookedData; }

	//! Get the associated rendering resource
	IRenderResource* GetRenderResource() const;

	//! Upload texture array to the renderer
	void CreateRenderResource();

	//! Release render resource
	void ReleaseRenderResource();

	Uint16 GetMipCount() const;
	Uint16 GetResidentMipIndex() const;
	Uint32 GetWidth() const;
	Uint32 GetHeight() const;
	RED_INLINE const CName& GetTextureGroupName() const { return m_textureGroup; }
	RED_INLINE Uint32 GetTextureCount() const { return IsCooked() ? m_cookedData.m_header.m_textureCount : m_bitmaps.Size(); }


	void GetTextureNames( TDynArray< CName >& textureNames ) const;
	void GetTextures( TDynArray< CBitmapTexture* >& textures ) const;

	// TEMP : Just to fix texarray cooking with minimal impact. We should be able to use GetTextures() above, but it filters out any
	// null textures. It should be fixed so that it can return nulls, and anything that's using it should handle that. In the interests
	// of a safe fix for cooking, a separate function...
	THandle< CBitmapTexture > GetTexture( Uint32 index ) const { return index < m_bitmaps.Size() ? m_bitmaps[index].m_texture.Get() : nullptr; }
	const String& GetTexturePath( Uint32 index ) const { return index < m_bitmaps.Size() ? m_bitmaps[index].m_texture.GetPath() : String::EMPTY; }


#ifndef NO_TEXTURECACHE_COOKER
	//! Set "Recreating" state
	RED_INLINE void SetDirty( Bool dirty ) { m_dirty = dirty; }

	void SetTextures( const TDynArray< CBitmapTexture* >& textures );

	void AddTexture( CBitmapTexture* texture );
	void InsertTexture( CBitmapTexture* texture, Uint32 index );
	void RemoveTextureAt( Uint32 index );

	Bool Contains( const CBitmapTexture* texture ) const;
	static Bool IsTextureValidForArray( const CBitmapTexture* texture, const CTextureArray* array, String& errorMessage /*out*/ );
private:
	Bool CheckIfSizeFits( const CBitmapTexture* texture, CBitmapTexture** firstNonMatchingTexture = NULL ) const;
	Bool CheckIfMipMapLevelsFit( const CBitmapTexture* texture, CBitmapTexture** firstNonMatchingTexture = NULL ) const;
	Bool CheckIfCompressionFits( const CBitmapTexture* texture, CBitmapTexture** firstNonMatchingTexture = NULL ) const;
	Bool CheckIfDownscaleBiasFits( const CBitmapTexture* texture, CBitmapTexture** firstNonMatchingTexture = NULL ) const;
#endif

private:
	void CreateUncookedStreamingSource();
	void CreateCookedStreamingSource();

#ifndef NO_TEXTURECACHE_COOKER
	void UpdateTextureGroup();
#endif
};

BEGIN_CLASS_RTTI( CTextureArray );
PARENT_CLASS( CResource );
// What to do about this? Shouldn't be cooked, but having mix of cooked/uncooked data might not work (need to be able to get texture names
// for terrain collision). For now leaving this in -- it's just handles to the bitmaps, so they don't get loaded until they're used.
PROPERTY_EDIT_NOT_COOKED( m_bitmaps, TXT("Textures in the array") );
PROPERTY( m_textureGroup );
END_CLASS_RTTI();



#ifndef NO_TEXTURECACHE_COOKER

class CTextureBakerSourceTextureArray : public ITextureBakerSource, public Red::System::NonCopyable
{
private:
	const CTextureArray*					m_texture;		//! The texture array we're cooking.
	TDynArray< THandle< CBitmapTexture > >	m_bitmaps;		//! Cached from m_texture, so we don't have to repeatedly call GetTextures()
	GpuApi::eTextureFormat					m_format;
	Uint16									m_mipCount;
	Uint16									m_baseMip;		//! Base mip index. This is treated as mip 0.

public:
	CTextureBakerSourceTextureArray( const CTextureArray* texture, Uint16 baseMip, const TDynArray< CBitmapTexture* >* bitmapOverrides = nullptr );

	virtual Uint16 GetSliceCount() const override;
	virtual Uint16 GetMipCount() const override;

	virtual const void* GetMipData( Uint16 mip, Uint16 slice ) const override;
	virtual Uint32 GetMipDataSize( Uint16 mip, Uint16 slice ) const override;

	virtual Uint32 GetMipPitch( Uint16 mip, Uint16 slice ) const override;

	virtual Uint16 GetBaseWidth() const override;
	virtual Uint16 GetBaseHeight() const override;

	virtual GpuApi::eTextureFormat GetTextureFormat() const override;
	virtual GpuApi::eTextureType GetTextureType() const override;

	virtual Bool IsLooseFileTexture() const { return false; }
};
#endif //!NO_TEXTURECACHE_COOKER
