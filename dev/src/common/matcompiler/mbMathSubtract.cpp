/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Subtract two values
class CMaterialBlockMathSubtract : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathSubtract, CMaterialBlock, "Math", "Subtract" );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( A ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( B ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk a = CompileInput( compiler, CNAME( A ), shaderTarget, 0.0f );
		CodeChunk b = CompileInput( compiler, CNAME( B ), shaderTarget, 0.0f );
		return SHADER_VAR_FLOAT4( a - b, shaderTarget );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathSubtract , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathSubtract );

#endif