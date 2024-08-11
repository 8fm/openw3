/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderTexture.h"

/// Texture array
class CRenderTextureArray : public CRenderTexture
{
protected:
	CRenderTextureArray( const GpuApi::TextureRef &texture );

public:
	virtual ~CRenderTextureArray();

	//! Get resource category
	virtual CName GetCategory() const { return CNAME( RenderTextureArray ); }

	// render atlas texture
	virtual Bool IsRenderTexture() const override { return true; }

	//! Create vertex type declaration
	static CRenderTextureArray* Create( const CTextureArray* textureArray, Uint64 partialRegistrationHash );
};
