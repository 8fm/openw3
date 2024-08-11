/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "thumbnail.h"

IMPLEMENT_RTTI_ENUM( EThumbnailFlags );
IMPLEMENT_ENGINE_CLASS( CThumbnail );
IMPLEMENT_ENGINE_CLASS( IThumbnailImageLoader );
IMPLEMENT_ENGINE_CLASS( IThumbnailGenerator );

CThumbnail::CThumbnail()
    : m_imageHandle( NULL )
	, m_imageData( TDataBufferAllocator< MC_Thumbnail >::GetInstance() )
	, m_cameraFov( 70.0 )
{
}

CThumbnail::CThumbnail( const void* data, const Uint32 size )
	: m_imageData( TDataBufferAllocator< MC_Thumbnail >::GetInstance(), size, data )
    , m_imageHandle( NULL )
	, m_cameraFov( 70.0 )
{
	RecreateImage();
	AddToRootSet();
}

CThumbnail::~CThumbnail()
{
	// Delete image
	if ( m_imageHandle )
	{
		delete m_imageHandle;
		m_imageHandle = NULL;
	}
}

void CThumbnail::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	RecreateImage();
}

void CThumbnail::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );
	m_imageData.Serialize( file );
}

void CThumbnail::DiscardImage()
{
	// Delete current image
	if ( m_imageHandle )
	{
		delete m_imageHandle;
		m_imageHandle = NULL;
	}
}

void CThumbnail::RecreateImage()
{
	// Delete current image
	if ( m_imageHandle )
	{
		delete m_imageHandle;
		m_imageHandle = NULL;
	}

	// Find thumbnail image loader
	// TODO: find another way...
	static THandle< IThumbnailImageLoader > loader = nullptr;
	if ( !loader.IsValid() )
	{
		TDynArray< CClass* > compressors;
		SRTTI::GetInstance().EnumClasses( ClassID< IThumbnailImageLoader >(), compressors );

		// No texture compressors found
		if ( compressors.Size() )
		{
			loader = compressors[0]->GetDefaultObject< IThumbnailImageLoader >();
		}
	}

	// Load thumbnail data
	if ( loader.IsValid() )
	{
		m_imageHandle = loader->Load( m_imageData );
	}
}

void CThumbnail::DiscardThumbnailImages()
{
	// Get all thumbnails
	for ( ObjectIterator<CThumbnail> it; it; ++it )
	{
		(*it)->DiscardImage();
	}
}
