/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/core/thumbnail.h"

/// Drawable thumbnail image
class CWXThumbnailImage : public IThumbnailImage
{
protected:
	Gdiplus::Bitmap*	m_bitmap;

public:
	CWXThumbnailImage( Gdiplus::Bitmap* bitmap );
	virtual ~CWXThumbnailImage();
	virtual Uint32 GetWidth() const;
	virtual Uint32 GetHeight() const;
	Gdiplus::Bitmap* GetBitmap() const;
};

/// PNG thumbnail image loader
class CWXThumbnailImageLoader : public IThumbnailImageLoader
{
	DECLARE_ENGINE_CLASS( CWXThumbnailImageLoader, IThumbnailImageLoader, 0 );

public:
	// Load thumbnail image
	virtual IThumbnailImage* Load( const DataBuffer& data );
};

DEFINE_SIMPLE_RTTI_CLASS( CWXThumbnailImageLoader, IThumbnailImageLoader );