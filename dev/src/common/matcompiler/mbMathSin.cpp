/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Add two values
class CMaterialBlockMathSin : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathSin, CMaterialBlock, "Math", "Sin" );

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
		return CodeChunkOperators::Sin( a );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathSin , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathSin );

#endif