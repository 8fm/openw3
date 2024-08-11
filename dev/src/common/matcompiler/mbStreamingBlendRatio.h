/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Vertex animation frame output stream
class CMaterialBlockStreamingBlendRatio : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockStreamingBlendRatio, CMaterialBlock, "Input", "Streaming Blend Ratio" );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
};

BEGIN_CLASS_RTTI( CMaterialBlockStreamingBlendRatio )
	PARENT_CLASS( CMaterialBlock )
END_CLASS_RTTI()

#endif

