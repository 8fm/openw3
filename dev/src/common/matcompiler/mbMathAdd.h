/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Add two values
class CMaterialBlockMathAdd : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathAdd, CMaterialBlock, "Math", "Add" );

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
		return a + b;
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathAdd , CMaterialBlock);
#endif