/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Vertex animation frame output stream
class CMaterialBlockVertexMotionBlend : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockVertexMotionBlend, CMaterialBlock, "Input", "Motion Blend" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk pos = PS_DATA( "MotionBlend" );
		return PS_VAR_FLOAT( pos );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockVertexMotionBlend , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockVertexMotionBlend );

#endif