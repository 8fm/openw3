/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialSampler.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that combines normals with detail normals
class CMaterialBlockSamplerDetail : public CMaterialBlockSampler
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockSamplerDetail, CMaterialBlockSampler, "Deprecated", "Detailed texture sampler" );

protected:
	Vector	m_scale;
	Float	m_BlendStartDistance;
	Float	m_BlendEndDistance;

public:
	CMaterialBlockSamplerDetail();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
};

BEGIN_CLASS_RTTI( CMaterialBlockSamplerDetail )
PARENT_CLASS(CMaterialBlockSampler)
PROPERTY_EDIT( m_scale, TXT("Interal texture scale") );
PROPERTY_EDIT( m_BlendStartDistance, TXT("Blend start distance") );
PROPERTY_EDIT( m_BlendEndDistance, TXT("Blend end distance") );
END_CLASS_RTTI()

#endif
