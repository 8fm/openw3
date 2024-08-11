/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialSampler.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that samples texture
class CMaterialBlockSamplerTexture : public CMaterialBlockSampler
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockSamplerTexture, CMaterialBlockSampler, "Samplers", "Texture Sampler" );

private:
	CodeChunk SampleTexture( CMaterialBlockCompiler& compiler, const CodeChunk &texture, const CodeChunk &sampler, const CodeChunk &uv, bool allowBias, EMaterialShaderTarget shaderTarget ) const;

public:
	Uint32							m_subUVWidth;			//!< SUB UV images in the X direction
	Uint32							m_subUVHeight;			//!< SUB UV images in the Y direction
	Bool							m_subUVInterpolate;		//!< Use frame interpolation
	Bool							m_projected;			//!< Sample in projected space
	
public:
	CMaterialBlockSamplerTexture();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
};

BEGIN_CLASS_RTTI( CMaterialBlockSamplerTexture )
	PARENT_CLASS(CMaterialBlockSampler)
	PROPERTY_EDIT( m_subUVWidth, TXT("SUB UV images in the X direction") );
	PROPERTY_EDIT( m_subUVHeight, TXT("SUB UV images in the Y direction") );
	PROPERTY_EDIT( m_subUVInterpolate, TXT("Use frame interpolation") );
	PROPERTY_EDIT( m_projected, TXT("Sample tex coords in projected texture space") );
END_CLASS_RTTI()

#endif