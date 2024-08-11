/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "textureCache.h"

#ifdef USE_SCALEFORM


namespace GpuApi
{
	enum eTextureFormat;
}


// A Scaleform Image that can be loaded from the texture cache. 
class CScaleformTextureCacheImage : public SF::Render::ImageBase
{
private:
	CTextureCacheQuery m_cacheQuery;

public:
	CScaleformTextureCacheImage( const CTextureCacheQuery& cacheQuery );
	virtual ~CScaleformTextureCacheImage();

	virtual SF::Render::ImageBase::ImageType GetImageType() const  { return Type_Other; }

	virtual SF::Render::ImageFormat GetFormat() const;
	virtual SF::Render::ImageSize GetSize() const;
	virtual unsigned GetMipmapCount() const;

	virtual SF::Render::Image* GetAsImage() { return nullptr; }

	GpuApi::eTextureFormat GetGpuTextureFormat() const;

	RED_INLINE const CTextureCacheQuery& GetCacheQuery() const { return m_cacheQuery; }

};

#endif
