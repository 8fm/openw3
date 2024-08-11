/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "swfTexture.h"

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CSwfTexture );

//////////////////////////////////////////////////////////////////////////
// CSwfTexture
//////////////////////////////////////////////////////////////////////////
#ifndef NO_RESOURCE_IMPORT
CSwfTexture* CSwfTexture::Create( const FactoryInfo& data )
{
	CSwfTexture* swfTexture = data.m_reuse;
	if ( ! swfTexture )
	{
		swfTexture = ::CreateObject< CSwfTexture >( data.m_parent );
	}

	swfTexture->m_linkageName = data.m_linkageName;
//	swfTexture->SetResidentMipIndex( 0 );
	
	return swfTexture;
}

Bool CSwfTexture::UncookData()
{
	m_cookedData.Load();
	void* dataPtr = m_cookedData.GetDataHandle().GetRawPtr();
	if ( !dataPtr || m_mips.Empty() )
	{
		return false;
	}

	CBitmapTexture::MipMap& mip0 = m_mips[0];
	// Let the helper determine data sizes for us.
	if( CreateMip( mip0, mip0.m_width, mip0.m_height, m_format, GetCompression( ) ) )
	{
		// Store the mip data compressed in current platform's format.
		Uint32 srcBytes = mip0.m_data.GetSize( );
		Red::MemoryCopy( mip0.m_data.GetData( ), dataPtr, srcBytes );
	}
	else
	{
		// No data (zero width or zero height) or invalid texture format.
		RED_LOG_WARNING( CBitmapTexture, TXT( "Invalid or missing data for mip level zero!" ));
	}

	m_residentMipIndex = 0;
	m_textureCacheKey = 0;

	ClearFlag( OF_WasCooked );

	return true;
}

#endif // #ifndef NO_RESOURCE_IMPORT