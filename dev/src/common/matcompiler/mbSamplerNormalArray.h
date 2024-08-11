/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that samples normalmap
class CMaterialBlockSamplerNormalArray : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockSamplerNormalArray, CMaterialBlock, "Deprecated", "Normalmap array sampler" );

private:
	CodeChunk SampleTexture( CMaterialBlockCompiler& compiler, const CodeChunk &sampler, const CodeChunk &uv, const CodeChunk& index, bool allowBias ) const;

public:
	Uint32		m_subUVWidth;			//!< SUB UV images in the X direction
	Uint32		m_subUVHeight;			//!< SUB UV images in the Y direction
	Bool		m_subUVInterpolate;		//!< Use frame interpolation

public:
	CMaterialBlockSamplerNormalArray();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
};

BEGIN_CLASS_RTTI( CMaterialBlockSamplerNormalArray )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_subUVWidth, TXT("SUB UV images in the X direction") );
PROPERTY_EDIT( m_subUVHeight, TXT("SUB UV images in the Y direction") );
PROPERTY_EDIT( m_subUVInterpolate, TXT("Use frame interpolation") );
END_CLASS_RTTI()

#endif