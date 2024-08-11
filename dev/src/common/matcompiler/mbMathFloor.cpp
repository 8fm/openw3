/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Floor two values
class CMaterialBlockMathFloor : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathFloor, CMaterialBlock, "Math", "Floor" );

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
		return CodeChunkOperators::Floor( x );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathFloor , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathFloor );

#endif