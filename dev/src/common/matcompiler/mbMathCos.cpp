/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Add two values
class CMaterialBlockMathCos : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathCos, CMaterialBlock, "Math", "Cos" );

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
		return CodeChunkOperators::Cos( a );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathCos , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathCos );

#endif