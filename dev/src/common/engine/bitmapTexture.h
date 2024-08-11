/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/deferredDataBuffer.h"
#include "texture.h"
#include "gpuDataBuffer.h"
#include "bitmapDataBuffer.h"
#include "textureGroup.h"
#include "textureCache.h"

class CSourceTexture;
class TextureGroup;
enum ETextureCompression : CEnum::TValueType;
namespace GpuApi { enum eTextureFormat; }


/// Texture raw data format ( editor data )
enum ETextureRawFormat : CEnum::TValueType
{
	TRF_TrueColor,		// 32-bit true color texture
	TRF_Grayscale,		// Alpha only texture ( 8-bit )
	TRF_HDR,			// HDR texture ( 128-bit )
	TRF_AlphaGrayscale,	// Grayscale + alpha ( 16-bit )
	TRF_HDRGrayscale,	// HDR Grayscale ( 32-bit )

	TRF_Grayscale_Font,
};

BEGIN_ENUM_RTTI( ETextureRawFormat );
	ENUM_OPTION( TRF_TrueColor );
	ENUM_OPTION( TRF_Grayscale );
	ENUM_OPTION( TRF_HDR );
	ENUM_OPTION( TRF_AlphaGrayscale );
	ENUM_OPTION( TRF_HDRGrayscale );
END_ENUM_RTTI();

/// Bitmap based texture
class CBitmapTexture : public ITexture
{
	DECLARE_ENGINE_RESOURCE_CLASS( CBitmapTexture, ITexture, "xbm", "2D Texture" );

public:
	/// Texture mipmap level
	struct MipMap
	{
		Uint32							m_width;			// Width of the mipmap level
		Uint32							m_height;			// Height of the mipmap level
		Uint32							m_pitch;			// Data buffer pitch
		BitmapMipLatentDataBuffer		m_data;				// Data buffer. When cooked, this will be empty.

		RED_FORCE_INLINE MipMap()
			: m_width( 0 )
			, m_height( 0 )
			, m_pitch( 0 )
		{}

		RED_FORCE_INLINE MipMap( Uint32 width, Uint32 height, Uint32 pitch )
			: m_width( width )
			, m_height( height )
			, m_pitch( pitch )
			, m_data( height * pitch )
		{}

		RED_FORCE_INLINE TMemSize GetInternalMemSize() const
		{
			if ( m_data.GetData() )
			{
				return static_cast< TMemSize >( m_data.GetSize() );
			}
			return 0;
		}
	};
	// Mipmap array type
	typedef TDynArray< MipMap, MC_RenderData > MipArray;

	// Getting the stats that we want for data mining. World Resource DB
	struct SStats
	{
		Uint32							m_width;					// Width of the bitmap
		Uint32							m_height;					// Height of the bitmap
		MipArray						m_mips;						// Mipmaps
		THandle< CSourceTexture >		m_sourceData;				// Source artist data
		ETextureRawFormat				m_format;					// Artist data format
		String							m_importFile;				// Asset file when imported the resource
	};
	

protected:
	Uint32							m_width;						// Width of the texture
	Uint32							m_height;						// Height of the texture
	ETextureRawFormat				m_format;						// Artist data format
	ETextureCompression				m_compression;					// Compression type
	CName							m_textureGroup;					// ID of the texture group
	THandle< CSourceTexture >		m_sourceData;					// Source artist data
	MipArray						m_mips;							// Mipmaps
	GpuDataBuffer					m_cookedData;					// Mipmaps cooked (this is only raw data without header, header can be deduced from above)
	Int32							m_pcDownscaleBias;				// Bias used when downscaling a texture
	Int32							m_xboneDownscaleBias;			// Bias used when downscaling a texture
	Int32							m_ps4DownscaleBias;				// Bias used when downscaling a texture
	GpuApi::eTextureFormat			m_platformSpecificCompression;	// Platform-specific compression type

	Uint32							m_textureCacheKey;				// Key for lookup in texture cache

protected:
	IRenderResource*				m_renderResource;				// Rendering resource
	IBitmapTextureStreamingSource*	m_streamingSource;				// Streaming source
	Uint8							m_residentMipIndex;				// Index of first resident mipmap

#ifndef NO_DEBUG_PAGES
	friend class CDebugPageLoadedResources;
public:
	Bool							m_resourceLoadError;			// Error happened when loading resource
#endif

public:
	//! Get the width of the bitmap
	RED_FORCE_INLINE Uint32 GetWidth() const { return m_width; }
	//! Get the height of the bitmap
	RED_FORCE_INLINE Uint32 GetHeight() const { return m_height; }

	//! Setters, needed only for resizing
	RED_FORCE_INLINE void SetWidth(Uint32 width) { m_width = width; }
	RED_FORCE_INLINE void SetHeight(Uint32 height) { m_height = height; }


	//! Get the raw ( source ) format of the texture data
	RED_FORCE_INLINE ETextureRawFormat GetFormat() const { return m_format; }
	//! Get the compression used to generate texture display data
	RED_FORCE_INLINE ETextureCompression GetCompression() const { return m_compression; }
	//! Get the texture's GpuApi texture format, which is decided based on the raw format and compression.
	RED_FORCE_INLINE GpuApi::eTextureFormat GetPlatformSpecificCompression() const { return m_platformSpecificCompression; }

	//! Get source data
	RED_FORCE_INLINE CSourceTexture* GetSourceData() const { return m_sourceData.Get(); }

	//! Get texture display data
	RED_FORCE_INLINE const MipArray& GetMips() const { return m_mips; }
	//! Get the number of mips of display data in this texture
	RED_FORCE_INLINE Uint32 GetMipCount() const { return m_mips.Size(); }

	//! Get the resident mip index (this mip and smaller should always be loaded).
	RED_FORCE_INLINE Uint8 GetResidentMipIndex() const { return m_residentMipIndex; }
	//! Set the resident mip index.
	RED_FORCE_INLINE void SetResidentMipIndex( Uint8 residentMipIndex ) { m_residentMipIndex = residentMipIndex; }

	//! Get downscale factor for PC
	RED_FORCE_INLINE Uint32 GetPCDownscaleBias() const { return m_pcDownscaleBias; }
	//! Get downscale factor for XBone
	RED_FORCE_INLINE Uint32 GetXBoneDownscaleBias() const { return m_xboneDownscaleBias; }
	//! Get downscale factor for PS4
	RED_FORCE_INLINE Uint32 GetPS4DownscaleBias() const { return m_ps4DownscaleBias; }

	Uint32 GetDownscaleBiasForCurrentPlatform() const;

	//! Change downscale factor for PC
	RED_FORCE_INLINE void SetPCDownscaleBias( Uint32 value ) { m_pcDownscaleBias = value; }
	//! Change downscale factor for XBone
	RED_FORCE_INLINE void SetXBoneDownscaleBias( Uint32 value ) { m_xboneDownscaleBias = value; }
	//! Change downscale factor for PS4
	RED_FORCE_INLINE void SetPS4DownscaleBias( Uint32 value ) { m_ps4DownscaleBias = value; }

	//! Get texture streaming source
	RED_FORCE_INLINE IBitmapTextureStreamingSource* GetStreamingSource() const { return m_streamingSource; }

	//! Get cooked data for read only
	RED_FORCE_INLINE const GpuDataBuffer& GetCookedData() const { return m_cookedData; }
	void SetCookedData( const GpuDataBuffer& data );

	//! Get a value to find this texture in the texture cache
	RED_FORCE_INLINE Uint32 CalcTextureCacheKey() const { return GetHash( GetDepotPath() ); }


#ifndef NO_DEBUG_PAGES
	RED_FORCE_INLINE Bool GetResourceLoadError() const { return m_resourceLoadError; }
#endif

public:
	CBitmapTexture();
	virtual ~CBitmapTexture();

	//! Serialize bitmap
	virtual void OnSerialize( IFile& file ) override;

#ifndef NO_RESOURCE_COOKING
	Uint32 GetHighestMipForCookingPlatform( ECookingPlatform platform ) const;
	// Get the mip level that should remain resident, relative to the highest cooked mip.
	Uint32 GetResidentMipForCookingPlatform( ECookingPlatform platform ) const;

	virtual void OnCook( ICookerFramework& cooker ) override;
	virtual Bool UncookData( const TextureCacheEntry& textureEntry, BufferHandle textureData );
	RED_FORCE_INLINE MipArray& GetMips( ) { return m_mips; }
#endif

protected:
	//! Serialize texture mip maps
	void SerializeMipMaps( IFile& file );

public:
	//! Bitmap was loaded from file
	virtual void OnPostLoad() override;
	
#ifndef NO_DATA_VALIDATION
	//! Validate this resource using the data error reporter
	virtual void OnCheckDataErrors() const override;
#endif

	//! Get additional resource info, displayed in editor
	virtual void GetAdditionalInfo( TDynArray< String >& info ) const override;

	//! Get texture group name
	RED_FORCE_INLINE const CName& GetTextureGroupName() const { return m_textureGroup; }
	//! Get texture group settings used by texture
	const TextureGroup& GetTextureGroup() const;
	//! Change texture group
	void SetTextureGroup( CName textureGroup );

	//! Get the related rendering resource
	virtual IRenderResource* GetRenderResource() const override;
	//! Create rendering resource for this bitmap texture
	void CreateRenderResource();
	//! Release rendering resource
	void ReleaseRenderResource();

	//! Generate mipmap chain for texture
	void GenerateMipmaps();
	//! Remove mipmap chain from texture
	void RemoveMipmaps();
	//! Remove ( drop ) given first mipmaps from texture
	void DropMipmaps( Int32 numMipmapsToDrop );

	//! Fill from source data
	Bool InitFromSourceData( CSourceTexture* sourceTexture, CName textureGroup, Bool cancelTextureStreaming = true );
	//! Fill from mip - method to provide compatibility for terrain or fonts
	Bool InitFromMip( const MipMap& mip, CName textureGroupName, ETextureRawFormat textureFormat );
	Bool InitFromCompressedMip( const MipMap& mip, CName textureGroupName, ETextureRawFormat textureFormat );

	//! Fill with fill byte
	Bool InitEmpty( Uint32 width, Uint32 height, ETextureRawFormat format, CName textureGroup, Uint8 fillByte=0 );

public:
	// Get pixel size in bits for given raw format of texture, is the same on every platform
	static Uint32 GetPixelSize( ETextureRawFormat rawFormat );

	// Get pixel size in bits for given raw format of texture, is the same on every platform
	static Uint32 GetNumChannels( ETextureRawFormat rawFormat );

	// Create empty mip
	static Bool CreateMip( MipMap& destData, Uint32 width, Uint32 height, ETextureRawFormat destFormat, ETextureCompression destCompreesion );

	// Compress texture
	static Bool CompressMipChain( const MipMap& srcData, ETextureRawFormat srcFormat, MipArray& destMips, ETextureCompression destCompression, Bool generateAllMips );

	// Copy part of texture with the same format
	static Bool CopyRect( const MipMap& srcData, const Rect* srcRect, MipMap& destData, const Rect* destRect, ETextureRawFormat format, ETextureCompression compreesion );

	// Copy part of texture with the same format
	static Bool CopyRects( const MipMap& srcData, const TDynArray< Rect >& srcRects, MipMap& destData, const TDynArray< Rect >& destRects, ETextureRawFormat format, ETextureCompression compreesion );

	// Copy texture with decompression, faster
	static Bool CopyRect( const MipMap& srcData, ETextureRawFormat format, ETextureCompression srcCompreesion, MipMap& destData, ETextureCompression destCompreesion );

	// Copy texture with decompression, faster
	static Bool CopyBitmap( CBitmapTexture* destBitmap, const Rect* destRect, const CBitmapTexture* srcBitmap, const Rect* srcRect );

	// Get GpuApi compressed format for format & compression
	static Bool GetCompressedFormat( ETextureRawFormat format, ETextureCompression compression, GpuApi::eTextureFormat &outFormat, Bool oldVersion = true );

	// Compress raw data buffer
	// alphaThreshold is for compressing to BC1, any alpha values below it will become transparent.
	static Bool ConvertBuffer( Uint32 width, Uint32 height, Uint32 sourcePitch, GpuApi::eTextureFormat sourceFormat, const void* sourceData, size_t sourceDataSize, GpuApi::eTextureFormat targetFormat, void *outTargetData, GpuApi::EImageCompressionHint compressionHint, Float alphaThreshold = 0.5f );

	// Swizzling step
	static Bool SwizzlingStep( Uint8 * buf, Uint32 width, Uint32 height, ETextureRawFormat srcFormat, ETextureCompression srcCompression, ETextureRawFormat dstFormat, ETextureCompression dstCompression );

	SStats GetStats() const;
#ifndef NO_EDITOR
	Uint32 CalcTextureDataSize( );
#endif
};

BEGIN_CLASS_RTTI( CBitmapTexture );
	PARENT_CLASS( ITexture );
	PROPERTY_RO( m_width, TXT("Width of the texture") );
	PROPERTY_RO( m_height, TXT("Height of the texture") );
	PROPERTY_RO( m_format, TXT("Source texture format type") );
	PROPERTY_RO( m_compression, TXT("Compression method to use") );
	PROPERTY_NOT_COOKED( m_sourceData );
	PROPERTY_CUSTOM_EDIT( m_textureGroup, TXT("Texture group"), TXT("TextureGroupList") );
	PROPERTY_SETTER( m_textureGroup, SetTextureGroup );
	PROPERTY_RO_NOT_COOKED( m_pcDownscaleBias, TXT("Bias used when downscaling texture for PC") );
	PROPERTY_RO_NOT_COOKED( m_xboneDownscaleBias, TXT("Bias used when downscaling texture for XB One") );
	PROPERTY_RO_NOT_COOKED( m_ps4DownscaleBias, TXT("Bias used when downscaling texture for PS 4") );
	PROPERTY( m_residentMipIndex );
	PROPERTY( m_textureCacheKey );
END_CLASS_RTTI();


class CSourceTexture : public CResource
{
	friend class CBitmapTexture;

	DECLARE_ENGINE_RESOURCE_CLASS( CSourceTexture, CResource, "xbd", "2D Texture" );

private:
	BitmapMipLatentDataBuffer	m_dataBuffer;
	Uint32						m_width;
	Uint32						m_height;
	Uint32						m_pitch;
	ETextureRawFormat			m_format;

public:
	CSourceTexture();
	~CSourceTexture();

public:

	Uint32 GetWidth() const { return m_width; }
	Uint32 GetHeight() const { return m_height; }
	ETextureRawFormat GetFormat() const { return m_format; }

	// Initialize empty
	void Init( Uint32 width, Uint32 height, ETextureRawFormat format );

	// Get direct pointer to buffer
	void* GetBufferAccessPointer();
	Uint32 GetBufferSize() const { return m_dataBuffer.GetSize(); }

	// Create from mip with same format, size and no compression
	Bool CreateFromMip( const CBitmapTexture::MipMap& mip );

	// Create from raw data with same format, size and no compression
	Bool CreateFromRawData( void* buf, Uint32 origWidth, Uint32 origHeight, Uint32 origPitch );

	// Create from mip with same format, size, but with compression - use only in fallback cases :( 
	Bool CreateFromDataCompressed( const void* data, size_t dataSize, ETextureCompression textureCompression );

	// Create from mip with same format, size, but with compression - use only in fallback cases :( 
	Bool CreateFromMipCompressed( const CBitmapTexture::MipMap& mip, ETextureCompression textureCompression );

	// Create mip but use bicubic resampling - so it can be used as mip 0 for texture on other platform eg xbox
	void CreateFromSourceBicubicResample( const CSourceTexture* other );

	// Create mip map chain with normal box resampling
	void CreateMips( CBitmapTexture::MipArray& outMips, Uint32 width, Uint32 height, ETextureCompression textureCompression, Bool allMips, Bool flipX = false, Bool flipY = false, Bool rotate = false, Bool silent = false );

	// Fill uncompressed mip with data
	void FillMip( CBitmapTexture::MipMap& mip );

	// Fill raw, truecolor noncompressed buffer
	void FillBufferTrueColor( Uint8* buf, Uint32 width, Uint32 height );
	
	// Fill raw, hdr noncompressed buffer
	void FillBufferHDR( Float* buf, Uint32 width, Uint32 height );

	// On serialize
	virtual void OnSerialize( IFile& file ) override;

public:
	// Helper function to copy raw buffer assuming that it has same compression and format
	static void CopyBufferPitched( void* dest, Uint32 destPitch, const void* src, Uint32 srcPitch, Uint32 rowDataSize, Uint32 numRows );

};

BEGIN_CLASS_RTTI( CSourceTexture );
PARENT_CLASS( CResource );
PROPERTY_RO( m_width, TXT("Width of the texture") );
PROPERTY_RO( m_height, TXT("Height of the texture") );
PROPERTY_RO( m_pitch, TXT("Pitch of the texture") );
PROPERTY_RO( m_format, TXT("Source texture format type") );
END_CLASS_RTTI();



#ifndef NO_TEXTURECACHE_COOKER

class CTextureBakerSourceBitmapTexture : public ITextureBakerSource, public Red::System::NonCopyable
{
private:
	CBitmapTexture*	m_texture;
	Uint16			m_startMip;

public:
	CTextureBakerSourceBitmapTexture( CBitmapTexture* texture, Uint16 startMip = 0 );

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

	static const LatentDataBuffer* GetBitmapMipDataBuffer( CBitmapTexture* texture, Uint32 mip );
};

#endif // !NO_TEXTURECACHE_COOKER
