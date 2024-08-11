/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Vertex world space position 
class CMaterialBlockViewVector : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockViewVector, CMaterialBlock, "Input", "View Vector" );

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
		CodeChunk vertexPos = PS_DATA( "WorldPosition" );
		CodeChunk cameraPos = PS_CONST_FLOAT3( PSC_CameraPosition );
		return PS_VAR_FLOAT4( Float4( Normalize( cameraPos.xyz() - vertexPos.xyz() ), 0.0f ) );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockViewVector , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockViewVector );

#endif