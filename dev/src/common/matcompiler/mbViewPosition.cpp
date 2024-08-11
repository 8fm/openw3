/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Vertex world space position 
class CMaterialBlockViewPosition : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockViewPosition, CMaterialBlock, "Input", "View Position" );

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
		CodeChunk vertexPos = PS_DATA( "ViewPosition" );
		return PS_VAR_FLOAT4( Float4( vertexPos.xyz(), 1.0f ) );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockViewPosition , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockViewPosition );

#endif