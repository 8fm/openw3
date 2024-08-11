/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialSampler.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that combines normals with detail normals
class CMaterialBlockSamplerNormalDetail : public CMaterialBlockSampler
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockSamplerNormalDetail, CMaterialBlockSampler, "Deprecated", "Detailed Normal Map Sampler" );

protected:
	Vector	m_scale;
	Bool	m_sampleTangentSpace;
	Float	m_BlendStartDistance;
	Float	m_BlendEndDistance;

public:
	CMaterialBlockSamplerNormalDetail();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
};

BEGIN_CLASS_RTTI( CMaterialBlockSamplerNormalDetail )
	PARENT_CLASS(CMaterialBlockSampler)
	PROPERTY_EDIT( m_scale, TXT("Interal texture scale") );
	PROPERTY_EDIT( m_sampleTangentSpace, TXT("Sample normal in tangent space") );
	PROPERTY_EDIT( m_BlendStartDistance, TXT("Blend start distance") );
	PROPERTY_EDIT( m_BlendEndDistance, TXT("Blend end distance") );
END_CLASS_RTTI()

#endif