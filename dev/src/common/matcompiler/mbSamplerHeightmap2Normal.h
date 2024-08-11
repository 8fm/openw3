/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialSampler.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that samples heightmap and control map textures and combines output normal
class CMaterialBlockSamplerHeightmap2Normal : public CMaterialBlockSampler
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockSamplerHeightmap2Normal, CMaterialBlockSampler, "Deprecated", "Multichannel heightmap to normal sampler" );

protected:
	Vector	m_scale;

public:
	CMaterialBlockSamplerHeightmap2Normal();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
};

BEGIN_CLASS_RTTI( CMaterialBlockSamplerHeightmap2Normal )
	PARENT_CLASS( CMaterialBlockSampler )
	PROPERTY_EDIT( m_scale, TXT("Interal texture scale") );
END_CLASS_RTTI()

#endif