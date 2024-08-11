/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "wxThumbnailImageLoader.h"

#include <ole2.h>

IMPLEMENT_ENGINE_CLASS( CWXThumbnailImageLoader );

CWXThumbnailImage::CWXThumbnailImage( Gdiplus::Bitmap* bitmap )
	: m_bitmap( bitmap )
{
}

CWXThumbnailImage::~CWXThumbnailImage()
{
	if ( m_bitmap )
	{
		delete m_bitmap;
		m_bitmap = NULL;
	}
}

Uint32 CWXThumbnailImage::GetWidth() const
{
	return m_bitmap->GetWidth();
}

Uint32 CWXThumbnailImage::GetHeight() const
{
	return m_bitmap->GetHeight();
}

Gdiplus::Bitmap* CWXThumbnailImage::GetBitmap() const
{
	return m_bitmap;
}

IThumbnailImage* CWXThumbnailImageLoader::Load( const DataBuffer& data )
{
	Gdiplus::Bitmap* bitmap = NULL;

	// Allocate global memory buffer
	HGLOBAL hBuffer = ::GlobalAlloc( GMEM_MOVEABLE, data.GetSize() );
	if ( hBuffer )
	{
		void* pBuffer = ::GlobalLock( hBuffer );
		if( pBuffer )
		{
			CopyMemory( pBuffer, data.GetData(), data.GetSize() );
			IStream* pStream = NULL;

			if ( CreateStreamOnHGlobal( hBuffer, FALSE, &pStream ) == S_OK )
			{
				bitmap = Gdiplus::Bitmap::FromStream( pStream, false );
				
				pStream->Release();
			}
			::GlobalUnlock(hBuffer);
		}
		::GlobalFree(hBuffer);          
	}

	// Use it
	return bitmap ? new CWXThumbnailImage( bitmap ) : NULL;
}
