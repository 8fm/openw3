/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "gpuDataBuffer.h"
#include "../core/resource.h"
#include "bitmapTexture.h"
#include "textureCache.h"

class CBitmapTexture;

// Face of cubemap
class CubeFace
{
	DECLARE_RTTI_STRUCT( CubeFace );

public:
	THandle< CBitmapTexture >	m_texture;		//!< Source texture
	THandle< CSourceTexture >	m_sourceTexture;//!< Source texture
	Bool						m_rotate;		//!< Face rotation
	Bool						m_flipX;		//!< Flip in X direction
	Bool						m_flipY;		//!< Flip in Y direction

public:
	// Constructor
	RED_INLINE CubeFace()
		: m_texture( nullptr )
		, m_sourceTexture( nullptr )
		, m_rotate( false )
		, m_flipX( false )
		, m_flipY( false )
	{}


	Uint32 GetWidth() const;
	Uint32 GetHeight() const;
	Bool GetSize( Uint32& width, Uint32& height ) const;
};

BEGIN_CLASS_RTTI( CubeFace );
	PROPERTY_EDIT_NAME( m_texture, TXT("m_texture"), TXT("Source texture") );
	PROPERTY_RO( m_sourceTexture, TXT("Source texture") );
	PROPERTY_EDIT_NAME( m_rotate, TXT("m_rotate"), TXT("Rotate texture") );
	PROPERTY_EDIT_NAME( m_flipX, TXT("m_flipX"), TXT("Flip in X direction") );
	PROPERTY_EDIT_NAME( m_flipY, TXT("m_flipY"), TXT("Flip in Y direction") );
END_CLASS_RTTI();


enum ECubeGenerationStrategy
{
	ECGS_SharpAll,
	ECGS_BlurMips,
	ECGS_Preblur,
	ECGS_PreblurStrong,
	ECGS_PreblurDiffuse,
};

BEGIN_ENUM_RTTI( ECubeGenerationStrategy );
	ENUM_OPTION( ECGS_SharpAll );
	ENUM_OPTION( ECGS_BlurMips );
	ENUM_OPTION( ECGS_Preblur );
	ENUM_OPTION( ECGS_PreblurStrong );
	ENUM_OPTION( ECGS_PreblurDiffuse );
END_ENUM_RTTI();

/// Bitmap based texture
class CCubeTexture : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CCubeTexture, CResource, "w2cube", "Cubemap" );

public:
	//! Resource factory data
	class FactoryInfo : public CResource::FactoryInfo< CCubeTexture >
	{
	public:
		ETextureRawFormat		m_format;			//!< Source data format ( should match data stored in mipmaps )
		ETextureCompression		m_compression;		//!< Target compression

	public:
		RED_INLINE FactoryInfo()
			: m_format( TRF_TrueColor )
			, m_compression( TCM_DXTNoAlpha )
		{}
	};



	struct CookedDataHeader
	{
		Uint32		m_textureCacheKey;			//!< Key for lookup in texture cache.
		Uint16		m_residentMip;				//!< Highest mip which is resident. Anything higher must come from texture cache.
		Uint16		m_encodedFormat;			//!< Encoded format/cooked state. Don't access directly, use the GetTextureFormat/IsCooked.
		Uint16		m_edgeSize;					//!< Length of the cube sides at mip 0.
		Uint16		m_mipCount;					//!< Number of mip levels.

		CookedDataHeader();

		//! Get the actual GpuApi format for this texture.
		GpuApi::eTextureFormat GetTextureFormat() const;

		//! Whether this is actually cooked data, or just cached data to avoid regenerating all the time.
		Bool IsCooked() const;

#ifndef NO_CUBEMAP_GENERATION
		Bool SetTextureFormat( GpuApi::eTextureFormat format );
		void SetIsCooked( Bool cooked );
#endif
	};

	struct CookedData
	{
		CookedDataHeader	m_header;				//!< Header, containing information about the cooked data
		GpuDataBuffer		m_deviceData;			//!< The actual texture data. Will either be cooked and ready to drop into a render resource
													//!< (or used directly for in-place textures); or cached, with each face/mip in sequence.

		CookedData()
			: m_deviceData( GpuApi::INPLACE_Texture )
		{
		}

		//! Check whether the cooked data is valid. Simple check, just make sure we have device data.
		RED_INLINE Bool IsValid() const
		{
			return m_deviceData.GetSize() > 0;
		}
	};


protected:

#ifndef NO_CUBEMAP_GENERATION
	CubeFace						m_left;							//!< Left face of cube 
	CubeFace						m_right;						//!< Right face of cube
	CubeFace						m_front;						//!< Front face of cube
	CubeFace						m_back;							//!< Back face of cube
	CubeFace						m_top;							//!< Top face of cube
	CubeFace						m_bottom;						//!< Bottom face of cube
	ETextureCompression				m_compression;					//!< Target compression
	Uint32							m_targetFaceSize;				//!< 0 = take from faces
	ECubeGenerationStrategy			m_strategy;
#endif 

	IRenderResource*				m_renderResource;				//!< Rendering resource
	CookedData						m_cookedData;					//!< Cooked resource data. This is always valid, but may contain uncooked, cached data instead.
	GpuApi::eTextureFormat			m_platformSpecificCompression;

	IBitmapTextureStreamingSource*	m_streamingSource;				//!< Streaming source

public:
#ifndef NO_CUBEMAP_GENERATION

	//! Get the number of mip levels in this cube.
	RED_INLINE Uint32 GetMipCount() const { return m_cookedData.m_header.m_mipCount; }

	RED_INLINE const CubeFace& GetFace( Uint32 index ) const
	{
		switch ( index )
		{
		case 0: return m_left;
		case 1: return m_right;
		case 2: return m_front;
		case 3: return m_back;
		case 4: return m_top;
		case 5: return m_bottom;
		}

		// In the case of a bad index, just return the front face so we at least have something.
		RED_HALT( "Invalid face index: %u", index );
		return m_front;
	}
	
	RED_INLINE GpuApi::eTextureFormat GetPlatformSpecificCompression() const { return m_platformSpecificCompression; }

#endif

	RED_INLINE Uint32 CalcTextureCacheKey() const { return GetHash( GetDepotPath() ); }

	//! Get the associated rendering resource
	IRenderResource* GetRenderResource() const;

	//! Get cooked data
	RED_INLINE const CookedData& GetCookedData() const { return m_cookedData; }

public:
	CCubeTexture();
	virtual ~CCubeTexture();

	//! Property was changed
	virtual void OnPropertyPostChange( IProperty* property ) override;

	//! Serialize data
	virtual void OnSerialize( IFile& file ) override;

#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( ICookerFramework& cooker ) override;
	// Check if we should recreate the cube faces for the cook. If we need to downsample for a particular platform, this may give better
	// results, but there may be cases where recreating is impossible (e.g. if we don't have the original source bitmaps).
	Bool ShouldRecreateForCook() const;
	// Get the number of expected mips for the given platform.
	Uint32 GetMipCountForCookingPlatform( ECookingPlatform platform ) const;
	// Get the resident mip index for the given platform. This is relative to the highest cooked mip.
	Uint32 GetResidentMipForCookingPlatform( ECookingPlatform platform ) const;
#endif

	virtual void OnSave() override;

	//! Get streaming source. Will be null if streaming is not available for the cube (it's fully resident)
	RED_INLINE IBitmapTextureStreamingSource* GetStreamingSource() const { return m_streamingSource; }

	//! Upload cubemap to the renderer
	void CreateRenderResource();

	//! Release render resource
	void ReleaseRenderResource();

#ifndef NO_CUBEMAP_GENERATION
	//! Generate cached data for this texture, containing full mip chain.
	void GenerateCachedData( CCubeTexture::CookedData &outData );
#endif

private:
	//! Determine what texture format should be used based on compression settings and cube faces.
	Bool DeterminePlatformSpecificCompression( GpuApi::eTextureFormat& format );

	//! Create streaming source for this texture, loading from the texture cache.
	void CreateStreamingSource();

	//! Convert old cached data, where everything was packed into a single buffer, into the new separate header/data. If the provided
	//! data is not valid, this will re-create the cached data with GenerateCachedData().
	void UpgradeOldCachedData( const DataBuffer& deviceData, const DataBuffer& systemData );
};

BEGIN_CLASS_RTTI( CCubeTexture );
	PARENT_CLASS( CResource );
#ifndef NO_EDITOR
	PROPERTY_EDIT_NOT_COOKED( m_targetFaceSize, TXT("Target size for cube face") );
	PROPERTY_EDIT_NOT_COOKED( m_strategy, TXT("Cube generation strategy") );
	PROPERTY_EDIT_NOT_COOKED( m_compression, TXT("Compression method to use") );
	PROPERTY_EDIT_NOT_COOKED( m_front, TXT("Front face of cube") );
	PROPERTY_EDIT_NOT_COOKED( m_back, TXT("Back face of cube") );
	PROPERTY_EDIT_NOT_COOKED( m_top, TXT("Top face of cube") );
	PROPERTY_EDIT_NOT_COOKED( m_bottom, TXT("Down face of cube") );
	PROPERTY_EDIT_NOT_COOKED( m_left, TXT("Left face of cube ") );
	PROPERTY_EDIT_NOT_COOKED( m_right, TXT("Right face of cube") );
#endif 
END_CLASS_RTTI();



#ifndef NO_TEXTURECACHE_COOKER
class CTextureBakerSourceCubeTexture : public ITextureBakerSource, public Red::System::NonCopyable
{
private:
	const CCubeTexture::CookedData&	m_cookedData;
	GpuApi::eTextureFormat			m_format;
	Uint16							m_baseMip;

	Uint32							m_offsetToBaseMip;

public:
	CTextureBakerSourceCubeTexture( const CCubeTexture::CookedData& cookedData, Uint16 baseMip );

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

private:
	Uint16 GetMipEdgeLength( Uint16 mip ) const;
};
#endif // !NO_TEXTURECACHE_COOKER
