/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Vertex normal output stream
class CMaterialBlockVertexNormal : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockVertexNormal, CMaterialBlock, "Input", "Vertex Normal" );

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
		CodeChunk pos = SHADER_DATA( "VertexNormal", shaderTarget );
		return SHADER_VAR_FLOAT4( Float4( pos.xyz(), 0.0f ), shaderTarget );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockVertexNormal , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockVertexNormal );

#endif