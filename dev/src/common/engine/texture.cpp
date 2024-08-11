/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/gatheredResource.h"
#include "texture.h"
#include "textureCache.h"
#include "../core/fileLatentLoadingToken.h"

CGatheredResource resDefaultTexture( TXT("engine\\textures\\editor\\debug.xbm"), RGF_NotCooked );

IMPLEMENT_ENGINE_CLASS( ITexture );

ITexture::ITexture()
{
}

CGatheredResource * ITexture::GetDefaultResource()
{
	return &resDefaultTexture;
}




struct TexEncodePair
{
	GpuApi::eTextureFormat	format;
	Uint16					code;
};

static const TexEncodePair TEX_ENCODE_PAIRS[] = 
{
	// Codes here can be pretty much arbitrary, but must not exceed 0x7fff
	// It is important, though, that codes do not change relative to the TEXFMT_ value. These come from CCubeTexture, so any existing
	// cube textures depend on these current values.
	{ GpuApi::TEXFMT_R8G8B8A8,				1021 },
	{ GpuApi::TEXFMT_BC1,					1031 },
	{ GpuApi::TEXFMT_BC3,					1032 },
	{ GpuApi::TEXFMT_BC6H,					1033 },
	{ GpuApi::TEXFMT_BC7,					1034 },
	{ GpuApi::TEXFMT_Float_R16G16B16A16,	1035 },
	{ GpuApi::TEXFMT_Float_R32G32B32A32,	1036 },
	{ GpuApi::TEXFMT_BC2,					1037 },
	{ GpuApi::TEXFMT_BC4,					1038 },
	{ GpuApi::TEXFMT_BC5,					1039 },
};

Uint16 ITexture::EncodeTextureFormat( GpuApi::eTextureFormat format )
{
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32( TEX_ENCODE_PAIRS ); ++i )
	{
		if ( format == TEX_ENCODE_PAIRS[i].format )
		{
			return TEX_ENCODE_PAIRS[i].code;
		}
	}
	RED_HALT( "No encoding for texture format: %" RED_PRIWas, GpuApi::GetTextureFormatName( format ) );
	return 0;
}

GpuApi::eTextureFormat ITexture::DecodeTextureFormat( Uint16 encoded )
{
	for ( Uint32 i = 0; i < ARRAY_COUNT_U32( TEX_ENCODE_PAIRS ); ++i )
	{
		if ( encoded == TEX_ENCODE_PAIRS[i].code )
		{
			return TEX_ENCODE_PAIRS[i].format;
		}
	}
	RED_HALT( "Invalid encoded texture format: %u", encoded );
	return GpuApi::TEXFMT_Max;
}


//////////////////////////////////////////////////////////////////////////
// Texture streaming

CBitmapTextureStreamingSourcePC::CBitmapTextureStreamingSourcePC( Uint16 baseWidth, Uint16 baseHeight, Uint8 numMipmaps, IFileLatentLoadingToken* loadingToken )
	: m_mipLoader( loadingToken )
{
	m_baseWidth			= baseWidth;
	m_baseHeight		= baseHeight;
	m_numMipmaps		= numMipmaps;
	m_numTextures		= 1;
	m_smallestLoadable	= numMipmaps - 1;

	// Initialize array
	m_mipInfo = new SBitmapTextureStreamingMipInfo [ numMipmaps ];
	Red::System::MemoryZero( m_mipInfo, sizeof( SBitmapTextureStreamingMipInfo ) * numMipmaps );
}

CBitmapTextureStreamingSourcePC::~CBitmapTextureStreamingSourcePC()
{
	// Close loader
	if ( m_mipLoader )
	{
		delete m_mipLoader;
		m_mipLoader = nullptr;
	}

	// Cleanup array
	delete [] m_mipInfo;
}

//! Create loading token to stream mipmaps
IFileLatentLoadingToken* CBitmapTextureStreamingSourcePC::CreateLoadingToken( Uint32 /*textureIndex*/ /*=0*/ ) const
{
	RED_WARNING( m_mipLoader != nullptr, "Bitmap streaming source has null loading token" );
	if ( m_mipLoader == nullptr )
	{
		return nullptr;
	}
	return m_mipLoader->Clone();
}

//! Request mipmap loading info
const SBitmapTextureStreamingMipInfo* CBitmapTextureStreamingSourcePC::GetMipLoadingInfo( Int32 mipIndex, Uint32 /*textureIndex*/ /*=0*/ ) const
{
	RED_WARNING( m_mipInfo != nullptr, "Null mip info??" );
	RED_WARNING( mipIndex >= 0 && mipIndex < (Int32)m_numMipmaps, "mip index out of range: %i", mipIndex );
	if ( m_mipInfo == nullptr || mipIndex < 0 || mipIndex >= (Int32)m_numMipmaps )
	{
		return nullptr;
	}

	return &m_mipInfo[ mipIndex ];
}


//////////////////////////////////////////////////////////////////////////


CTextureArrayStreamingSourcePC::CTextureArrayStreamingSourcePC( Uint16 baseWidth, Uint16 baseHeight, Uint8 numMipmaps, const TDynArray< IFileLatentLoadingToken* >& loadingTokens )
	: m_mipLoaders( loadingTokens )
{
	m_baseWidth			= baseWidth;
	m_baseHeight		= baseHeight;
	m_numMipmaps		= numMipmaps;
	m_numTextures		= (Uint8)loadingTokens.Size();
	m_smallestLoadable	= numMipmaps - 1;

	// Initialize array
	m_mipInfo = new SBitmapTextureStreamingMipInfo*[ numMipmaps ];
	
	for ( Uint32 i = 0; i < numMipmaps; ++i )
	{
		m_mipInfo[ i ] = new SBitmapTextureStreamingMipInfo[ m_numTextures ];
		Red::System::MemoryZero( m_mipInfo[ i ], sizeof( SBitmapTextureStreamingMipInfo ) * m_numTextures );
	}
}

CTextureArrayStreamingSourcePC::~CTextureArrayStreamingSourcePC()
{
	// Close loader
	for ( Uint32 i = 0; i < m_mipLoaders.Size(); ++i )
	{
		if ( m_mipLoaders[ i ] )
		{
			delete m_mipLoaders[ i ];
			m_mipLoaders[ i ] = nullptr;
		}
	}
	m_mipLoaders.ClearFast();

	// Cleanup array
	for ( Uint32 i = 0; i < m_numMipmaps; ++i )
	{
		delete [] m_mipInfo[ i ];
	}
	delete[] m_mipInfo;
}

//! Create loading token to stream mipmaps
IFileLatentLoadingToken* CTextureArrayStreamingSourcePC::CreateLoadingToken( Uint32 textureIndex /*=0*/ ) const
{
	RED_WARNING( textureIndex < (Uint32)m_numTextures, "texture index out of range: %i", textureIndex );
	if ( textureIndex >= (Uint32)m_numTextures )
	{
		return nullptr;
	}

	IFileLatentLoadingToken* token = m_mipLoaders[ textureIndex ];
	RED_WARNING( token != nullptr, "Texture array streaming source has null mip loader. Texture %u missing?", textureIndex );
	if ( token == nullptr )
	{
		return nullptr;
	}

	return token->Clone();
}

//! Request mipmap loading info
const SBitmapTextureStreamingMipInfo* CTextureArrayStreamingSourcePC::GetMipLoadingInfo( Int32 mipIndex, Uint32 textureIndex /*=0*/ ) const
{
	RED_WARNING( m_mipInfo != nullptr, "Null mip info??" );
	RED_WARNING( mipIndex >= 0 && mipIndex < (Int32)m_numMipmaps, "mip index out of range: %i", mipIndex );
	if ( m_mipInfo == nullptr || mipIndex < 0 || mipIndex >= (Int32)m_numMipmaps )
	{
		return nullptr;
	}

	RED_WARNING( m_mipInfo[ mipIndex ] != nullptr, "Null mip info for mip %i??", mipIndex );
	RED_WARNING( textureIndex < (Uint32)m_numTextures, "texture index out of range: %i", textureIndex );
	if ( m_mipInfo[ mipIndex ] == nullptr || textureIndex >= (Uint32)m_numTextures )
	{
		return nullptr;
	}

	return &m_mipInfo[ mipIndex ][ textureIndex ];
}


//////////////////////////////////////////////////////////////////////////


CCubeTextureStreamingSourcePC::CCubeTextureStreamingSourcePC( Uint16 edgeSize, Uint8 numMipmaps, IFileLatentLoadingToken* loadingToken )
	: m_mipLoader( loadingToken )
{
	m_baseWidth			= edgeSize;
	m_baseHeight		= edgeSize;
	m_numMipmaps		= numMipmaps;
	m_numTextures		= 6;
	m_smallestLoadable	= numMipmaps - 1;

	// Initialize array
	for ( Uint32 i = 0; i < 6; ++i )
	{
		m_mipInfo[ i ] = new SBitmapTextureStreamingMipInfo[ numMipmaps ];
		Red::System::MemorySet( m_mipInfo[ i ], 0, sizeof( SBitmapTextureStreamingMipInfo ) * numMipmaps );
	}
}

CCubeTextureStreamingSourcePC::~CCubeTextureStreamingSourcePC()
{
	// Close loader
	if ( m_mipLoader )
	{
		delete m_mipLoader;
		m_mipLoader = nullptr;
	}

	// Cleanup array
	for ( Uint32 i = 0; i < 6; ++i )
	{
		delete [] m_mipInfo[ i ];
	}
}

//! Create loading token to stream mipmaps
IFileLatentLoadingToken* CCubeTextureStreamingSourcePC::CreateLoadingToken( Uint32 /*textureIndex*/ /*=0*/ ) const
{
	// Faces are stored together, so we can use the same loading token for all.
	RED_WARNING( m_mipLoader != nullptr, "Texture cube streaming source has null mip loader." );
	if ( m_mipLoader == nullptr )
	{
		return nullptr;
	}

	return m_mipLoader->Clone();
}

//! Request mipmap loading info
const SBitmapTextureStreamingMipInfo* CCubeTextureStreamingSourcePC::GetMipLoadingInfo( Int32 mipIndex, Uint32 faceIndex ) const
{
	RED_FATAL_ASSERT( m_mipInfo[ faceIndex ] != nullptr, "Null mip info??" );

	RED_WARNING( faceIndex < 6, "Face index out of range: %u", faceIndex );
	if ( faceIndex >= 6 ) { return nullptr; }

	RED_WARNING( mipIndex >= 0 && mipIndex < (Int32)m_numMipmaps, "Mip index out of range: %u", mipIndex );
	if ( mipIndex < 0 || mipIndex >= (Int32)m_numMipmaps ) { return nullptr; }

	return &m_mipInfo[ faceIndex ][ mipIndex ];
}


//////////////////////////////////////////////////////////////////////////


CTextureCacheStreamingSourcePC::CTextureCacheStreamingSourcePC( Uint32 entryHash )
	: m_entryHash( entryHash )
{
	MakeQuery();
}

CTextureCacheStreamingSourcePC::~CTextureCacheStreamingSourcePC()
{
}

IFileLatentLoadingToken* CTextureCacheStreamingSourcePC::CreateLoadingToken( Uint32 textureIndex /*= 0*/ ) const
{
	// We can't really make a straightforward loading token here... texcache _could_ have some implementation that handles the
	// details of decompression, but this would be painfully inefficient loading mip-by-mip, as the streaming does.
	return nullptr;
}

const SBitmapTextureStreamingMipInfo* CTextureCacheStreamingSourcePC::GetMipLoadingInfo( Int32 mipIndex, Uint32 textureIndex /*= 0*/ ) const
{
	static SBitmapTextureStreamingMipInfo mipInfo;
	mipInfo.m_isCooked = true;
	return &mipInfo;
}

Bool CTextureCacheStreamingSourcePC::IsReady() const
{
	return m_cacheQuery;
}

void CTextureCacheStreamingSourcePC::TryToMakeReady()
{
	if ( !m_cacheQuery )
	{
		MakeQuery();
	}
}

void CTextureCacheStreamingSourcePC::MakeQuery()
{
	m_cacheQuery = GTextureCache->FindEntry( m_entryHash );

	if ( m_cacheQuery )
	{
		m_baseWidth			= m_cacheQuery.GetEntry().m_info.m_baseWidth;
		m_baseHeight		= m_cacheQuery.GetEntry().m_info.m_baseHeight;
		m_numMipmaps		= (Uint8)m_cacheQuery.GetEntry().m_info.m_mipCount;
		m_numTextures		= (Uint8)m_cacheQuery.GetEntry().m_info.m_sliceCount;
		m_encodedFormat		= m_cacheQuery.GetEntry().m_encodedFormat;
		m_smallestLoadable	= m_cacheQuery.GetLowestLoadableMip();
	}
}
