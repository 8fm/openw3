/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that emits texture coordinates
class CMaterialBlockVertexLight: public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockVertexLight, CMaterialBlock, "System Samplers", "Vertex Light" );
public:
	CMaterialBlockVertexLight();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	virtual Int32 GetShaderTarget() const
	{
		return MSH_VertexShader;
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockVertexLight )
	PARENT_CLASS(CMaterialBlock)
END_CLASS_RTTI()

#endif