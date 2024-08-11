/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Frac two values
class CMaterialBlockMathFrac : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathFrac, CMaterialBlock, "Math", "Frac" );

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
		CodeChunk x = CompileInput( compiler, CNAME( X ), shaderTarget, 0.0f );		
		return CodeChunkOperators::Frac( x );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathFrac , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathFrac );

#endif