/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

template< class T >
class TextureMipLockerConst
{
public:
	const T*		m_data;
	Uint32			m_pitch;
	Uint32			m_width;
	Uint32			m_height;

public:
	TextureMipLockerConst( const CBitmapTexture* bitmap, Uint32 mipIndex=0 )
		: m_pitch( 0 )
		, m_width( 0 )
		, m_height( 0 )
	{
		ASSERT( bitmap->GetMips().Size() > 0 );
		const CBitmapTexture::MipMap& mip = bitmap->GetMips()[ mipIndex ];

		// Load the mip
		if ( const_cast< CBitmapTexture::MipMap& >( mip ).m_data.Load() )
		{
			// Get the data
			m_data = (const T*)mip.m_data.GetData();
			if ( m_data )
			{
				m_width = mip.m_width;
				m_height = mip.m_height;
				m_pitch = mip.m_pitch;
			}
		}
	}

	~TextureMipLockerConst()
	{
	}

	const T* GetRow( Uint32 index )
	{
		ASSERT( m_data );
		ASSERT( index < m_height );
		return OffsetPtr( m_data, m_pitch * index );
	}

	const T* GetPixel( Uint32 x, Uint32 y )
	{
		return GetRow( y ) + x;
	}
};

template< class T >
class TextureMipLockerModify
{
public:
	CBitmapTexture*		m_bitmap;
	T*					m_data;
	Uint32				m_pitch;
	Uint32				m_width;
	Uint32				m_height;
	Bool				m_generateMips;

public:
	TextureMipLockerModify( CBitmapTexture* bitmap, Uint32 mipIndex=0, Bool updateMipChain=true )
		: m_bitmap( bitmap )
		, m_pitch( 0 )
		, m_width( 0 )
		, m_height( 0 )
		, m_generateMips( updateMipChain )
	{
		ASSERT( bitmap->GetMips().Size() > 0 );
		CBitmapTexture::MipMap& mip = const_cast< CBitmapTexture::MipMap& >( bitmap->GetMips()[ mipIndex ] );

		if ( mip.m_data.Load() )
		{
			// Unlink data
			mip.m_data.Unlink();

			// Get the data
			m_data = (T*)mip.m_data.GetData();
			if ( m_data )
			{
				m_width = mip.m_width;
				m_height = mip.m_height;
				m_pitch = mip.m_pitch;
			}
		}
	}

	~TextureMipLockerModify()
	{
		if ( m_generateMips && m_bitmap->GetMipCount() > 1 )
		{
			// Generate full mipmap chain
			m_bitmap->GenerateMipmaps();
		}
		else
		{
			// Just reload texture
			m_bitmap->CreateRenderResource();
			m_bitmap->MarkModified();
		}
	}

	T* GetRow( Uint32 index )
	{
		ASSERT( index < m_height );
		return OffsetPtr( m_data, m_pitch * index );
	}

	T* GetPixel( Uint32 x, Uint32 y )
	{
		return GetRow( y ) + x;
	}
};