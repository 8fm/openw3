/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Rise something to a power
class CMaterialBlockMathPower : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathPower, CMaterialBlock, "Math", "Power" );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Exponent ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk a = CompileInput( compiler, CNAME( In ), shaderTarget, 1.0f );
		CodeChunk b = CompileInput( compiler, CNAME( Exponent ), shaderTarget, 1.0f );
		return SHADER_VAR_FLOAT4( Pow( a, b.x() ), shaderTarget );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathPower , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathPower );

#endif