/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialSampler.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that samples and decompressed a normals
class CMaterialBlockSamplerNormal : public CMaterialBlockSampler
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockSamplerNormal, CMaterialBlockSampler, "Deprecated", "Normalmap sampler" );

protected:
	Vector	m_scale;
	Bool	m_sampleTangentSpace;

public:
	CMaterialBlockSamplerNormal();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
};

BEGIN_CLASS_RTTI( CMaterialBlockSamplerNormal )
	PARENT_CLASS(CMaterialBlockSampler)
	PROPERTY_EDIT( m_scale, TXT("Interal texture scale") );
	PROPERTY_EDIT( m_sampleTangentSpace, TXT("Sample normal in tangent space") );
END_CLASS_RTTI()

#endif