/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Negate value
class CMaterialBlockMathNegate : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathNegate, CMaterialBlock, "Math", "Negate (-x)" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( X ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk a = CompileInput( compiler, CNAME( X ), shaderTarget, 0.0f );
		CodeChunk b = SHADER_VAR_FLOAT4(0.0f, shaderTarget);
		return b - a;
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathNegate , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathNegate );

#endif