/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Vertex view z position
class CMaterialBlockViewZ : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockViewZ, CMaterialBlock, "System Samplers", "View Depth" );

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
		CodeChunk refDepth   = PS_DATA( "ViewZ" );
		return PS_VAR_FLOAT4( Float4( refDepth.xxx(), 1.0f ) );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockViewZ , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockViewZ );

#endif