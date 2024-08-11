/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Clamp value to range
class CMaterialBlockMathMapValue : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathMapValue, CMaterialBlock, "Math", "Map Value" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Min ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Max ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk a = CompileInput( compiler, CNAME( In ), shaderTarget, 1.0f );
		CodeChunk min = CompileInput( compiler, CNAME( Min ), shaderTarget, 0.0f );
		CodeChunk max = CompileInput( compiler, CNAME( Max ), shaderTarget, 1.0f );
		return SHADER_VAR_FLOAT4( Saturate( (a - min) / ( max  - min) ), shaderTarget );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathMapValue , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathMapValue );

#endif