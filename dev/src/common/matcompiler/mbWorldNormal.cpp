/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Vertex world space normal vector
class CMaterialBlockWorldNormal : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockWorldNormal, CMaterialBlock, "Deprecated", "World normal" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialOutputSocketSpawnInfo(CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk pos = PS_DATA( "WorldNormal" );
		return PS_VAR_FLOAT4( Float4( Normalize( pos.xyz() ), 0.0f ) );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockWorldNormal , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockWorldNormal );

#endif