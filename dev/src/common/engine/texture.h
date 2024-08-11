/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderObject.h"
#include "../core/resource.h"
#include "textureCache.h"

class IRenderResource;


/// Base texture interface
class ITexture : public CResource
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ITexture, CResource );

public:
	ITexture();

	// get render resource to be implemented in subclasses
	virtual IRenderResource* GetRenderResource() const = 0;

	// Get priority
	virtual ResourceReloadPriority GetReloadPriority() { return 0; }

	// Get default resource used when resource of given type has not been found
	virtual CGatheredResource * GetDefaultResource();


	// Encode a GpuApi texture format into an integer value. This ensures that the format will have the same encoded value even
	// if the enumeration changes. Returns 0 for unsupported format.
	static Uint16 EncodeTextureFormat( GpuApi::eTextureFormat format );
	// Decode a previously encoded format to the appropriate GpuApi enumeration value. Returns TEXFMT_Max for invalid encoded value.
	static GpuApi::eTextureFormat DecodeTextureFormat( Uint16 encoded );
};

BEGIN_ABSTRACT_CLASS_RTTI( ITexture );
	PARENT_CLASS( CResource );
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////


/// Bitmap texture streaming mip info
struct SBitmapTextureStreamingMipInfo
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_RenderData );

	//////////////////////////////////////////////////////////////////////////
	// The following may be invalid for cooked data, and should only be used when m_isCooked is false.
	//
	// TODO : Since everything here is only used for uncooked, we could just have the streaming source provide
	// mip info for uncooked mips... if it's null, then it's cooked...
	Uint32		m_pitch;			//!< Pitch of the mip map data.
	Uint32		m_offset;			//!< Offset ( in file ) to the mip map data
	Uint32		m_size;				//!< Size of the mip map data buffer
	//////////////////////////////////////////////////////////////////////////

	Bool		m_isCooked;			//!< Is this cooked data?
};

/// Texture streaming interface
/// After initialization, only the renderer should use these.
class IBitmapTextureStreamingSource : public IRenderObject
{
protected:
	Uint16		m_baseWidth;		//!< Width of mip 0 of the texture
	Uint16		m_baseHeight;		//!< Height of mip 0 of the texture
	Uint16		m_encodedFormat;	//!< Encoded texture format
	Uint8		m_numMipmaps;		//!< Number of mipmaps in the streaming sources
	Uint8		m_numTextures;		//!< Number of textures in the streaming source

	Uint8		m_smallestLoadable;	//!< Smallest mip that can be loaded. The mips in range [m_smallestLoadable,m_numMips-1] can be loaded
									//!< together, but it might not be possible to load a subrange of that.

#ifndef RED_FINAL_BUILD
	String		m_debugName;
#endif

public:
	//! Get width of the texture
	RED_INLINE Uint16 GetBaseWidth() const { return m_baseWidth; }

	//! Get height of the texture
	RED_INLINE Uint16 GetBaseHeight() const { return m_baseHeight; }

	//! Get number of mipmaps in the streaming source
	RED_INLINE Uint8 GetNumMipmaps() const { return m_numMipmaps; }

	//! Get number of textures in the streaming source
	RED_INLINE Uint8 GetNumTextures() const { return m_numTextures; }

	//! Encoded format of the texture, used for non-resource textures
	RED_INLINE Uint16 GetEncodedFormat() const { return m_encodedFormat; }

	//! Get the smallest mip that can be loaded with this streaming source.
	RED_INLINE Int8 GetSmallestLoadableMip() const { return m_smallestLoadable; }

#ifndef RED_FINAL_BUILD
	RED_INLINE const String& GetDebugName() const { return m_debugName; }
	RED_INLINE void SetDebugName( const String& name ) { m_debugName = name; }
#endif

public:
	IBitmapTextureStreamingSource()
		: m_baseWidth( 0 )
		, m_baseHeight( 0 )
		, m_encodedFormat( 0 )
		, m_numMipmaps( 0 )
		, m_numTextures( 0 )
		, m_smallestLoadable( -1 )
	{}

	virtual ~IBitmapTextureStreamingSource() {}

	//! Create loading token to stream mipmaps
	virtual IFileLatentLoadingToken* CreateLoadingToken( Uint32 textureIndex = 0 ) const=0;

	//! Position on disk
	virtual Uint32 GetDiskPosition() const { return 0; }

	//! Request mipmap loading info
	// TODO : The stuff in here is really only needed for uncooked data. So, we could really just return null here for cooked mips.
	virtual const SBitmapTextureStreamingMipInfo* GetMipLoadingInfo( Int32 mipIndex, Uint32 textureIndex = 0 ) const=0;


	virtual Bool IsCooked() const = 0;

	//! Is it possible to stream from this source?
	virtual Bool IsReady() const { return true; }

	//! If !IsReady, see if the source can be made ready.
	virtual void TryToMakeReady() { return; }
};


//////////////////////////////////////////////////////////////////////////


/// Streaming source for PC
class CBitmapTextureStreamingSourcePC : public IBitmapTextureStreamingSource
{
public:
	IFileLatentLoadingToken*		m_mipLoader;		//!< Latent loading token to open first mipmap
	SBitmapTextureStreamingMipInfo*	m_mipInfo;			//!< Info about each mipmap

public:
	CBitmapTextureStreamingSourcePC( Uint16 baseWidth, Uint16 baseHeight, Uint8 numMipmaps, IFileLatentLoadingToken* loadingToken );
	~CBitmapTextureStreamingSourcePC();
	
	//! Create loading token to stream mipmaps
	virtual IFileLatentLoadingToken* CreateLoadingToken( Uint32 textureIndex = 0 ) const override;
	
	//! Request mipmap loading info
	virtual const SBitmapTextureStreamingMipInfo* GetMipLoadingInfo( Int32 mipIndex, Uint32 textureIndex = 0 ) const override;
	
	virtual Bool IsCooked() const override { return false; }
};


//////////////////////////////////////////////////////////////////////////


/// Streaming source for texture array PC
class CTextureArrayStreamingSourcePC : public IBitmapTextureStreamingSource
{
public:
	TDynArray< IFileLatentLoadingToken* >	m_mipLoaders;		//!< Latent loading token to open first mipmap
	SBitmapTextureStreamingMipInfo**		m_mipInfo;			//!< Info about each mipmap

public:
	CTextureArrayStreamingSourcePC( Uint16 baseWidth, Uint16 baseHeight, Uint8 numMipmaps, const TDynArray< IFileLatentLoadingToken* >& loadingTokens );
	~CTextureArrayStreamingSourcePC();

	//! Create loading token to stream mipmaps
	virtual IFileLatentLoadingToken* CreateLoadingToken( Uint32 textureIndex = 0 ) const override;

	//! Request mipmap loading info
	virtual const SBitmapTextureStreamingMipInfo* GetMipLoadingInfo( Int32 mipIndex, Uint32 textureIndex = 0 ) const override;

	virtual Bool IsCooked() const override { return false; }
};


//////////////////////////////////////////////////////////////////////////


/// Streaming source for texture cube PC
class CCubeTextureStreamingSourcePC : public IBitmapTextureStreamingSource
{
public:
	IFileLatentLoadingToken*		m_mipLoader;		//!< Latent loading token to open first mipmap
	SBitmapTextureStreamingMipInfo*	m_mipInfo[6];		//!< Info about each mipmap

public:
	CCubeTextureStreamingSourcePC( Uint16 edgeSize, Uint8 numMipmaps, IFileLatentLoadingToken* loadingToken );
	~CCubeTextureStreamingSourcePC();

	//! Create loading token to stream mipmaps
	virtual IFileLatentLoadingToken* CreateLoadingToken( Uint32 faceIndex = 0 ) const override;

	//! Request mipmap loading info
	virtual const SBitmapTextureStreamingMipInfo* GetMipLoadingInfo( Int32 mipIndex, Uint32 faceIndex ) const override;

	virtual Bool IsCooked() const override { return false; }
};


//////////////////////////////////////////////////////////////////////////


/// For streaming from the texture cache
class CTextureCacheStreamingSourcePC : public IBitmapTextureStreamingSource
{
public:
	Uint32				m_entryHash;
	CTextureCacheQuery	m_cacheQuery;

public:
	CTextureCacheStreamingSourcePC( Uint32 entryHash );
	~CTextureCacheStreamingSourcePC();

	//! Create loading token to stream mipmaps
	virtual IFileLatentLoadingToken* CreateLoadingToken( Uint32 textureIndex = 0 ) const override;

	//! Request mipmap loading info
	virtual const SBitmapTextureStreamingMipInfo* GetMipLoadingInfo( Int32 mipIndex, Uint32 textureIndex = 0 ) const override;

	virtual Bool IsCooked() const override { return true; }

	Uint32 GetTextureCacheHash() const { return m_entryHash; }

	const CTextureCacheQuery& GetTextureCacheQuery() const { return m_cacheQuery; }

	virtual Bool IsReady() const override;
	virtual void TryToMakeReady() override;

private:
	void MakeQuery();
};
