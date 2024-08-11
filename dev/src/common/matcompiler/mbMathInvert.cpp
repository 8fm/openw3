/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// One minus
class CMaterialBlockMathInvert : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathInvert, CMaterialBlock, "Math", "Invert (1-x)" );

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
		CodeChunk b = SHADER_VAR_FLOAT4(1.0f, shaderTarget);
		return b - a;
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathInvert , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathInvert );

#endif