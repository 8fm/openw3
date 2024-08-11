/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "renderTextureBase.h"

class CCubeTexture;

/// Cube map interface
class CRenderCubeTexture : public CRenderTextureBase
{
protected:
	String		m_displayableName;

protected:
	CRenderCubeTexture( const GpuApi::TextureRef &texture, const String& displayableName );

public:
	virtual ~CRenderCubeTexture();

	// Describe resource
	virtual CName GetCategory() const;

	// Get displayable name
	virtual String GetDisplayableName() const { return m_displayableName; }

public:
	//! Create vertex type declaration
	static CRenderCubeTexture* Create( const CCubeTexture* texture, Uint64 partialRegistrationHash );
};
