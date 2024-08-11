/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderTextureBase.h"


class CTextureCacheQuery;


/// Texture
class CRenderTexture : public CRenderTextureBase
{
protected:
	Float								m_width;
	Float								m_height;

protected:
	CRenderTexture( const GpuApi::TextureRef &texture );

public:
	virtual ~CRenderTexture();

	// Setup from texture reference
	void Setup( const GpuApi::TextureRef &texture );

	// Get size vector
	void GetSizeVector( Vector& sizeVector ) const;

	// Get 1/size vector
	void GetInvSizeVector( Vector& invSizeVector ) const;

	// Get size
	void GetSize( Float& width, Float& height ) const;

	// Describe resource
	virtual CName GetCategory() const;

	// render texture
	virtual Bool IsRenderTexture() const override { return true; }

	// Recalculate max streaming mip
	virtual void RecalculateMipStreaming();

protected:
	virtual void OnStreamingChanged();

public:
	static CRenderTexture* Create( const GpuApi::TextureRef &texture, Uint64 partialRegistrationHash );

	//! Create texture surface from engine bitmap
	static CRenderTexture* Create( const CBitmapTexture* texture, Uint64 partialRegistrationHash );

	static CRenderTexture* Create( const CTextureCacheQuery& cacheQuery, Uint64 partialRegistrationHash );
	
	static CRenderTexture* Create( IBitmapTextureStreamingSource* streamingSource, const CName& groupName, Uint64 partialRegistrationHash, GpuApi::eInternalTexture fallback = GpuApi::INTERTEX_Blank2D );
};
