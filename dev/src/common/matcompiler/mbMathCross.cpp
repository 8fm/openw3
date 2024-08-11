/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Add two values
class CMaterialBlockMathCross : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathCross, CMaterialBlock, "Math", "Cross" );

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
		CodeChunk a = CompileInput( compiler, CNAME( A ), shaderTarget, Vector( 1.0f, 1.0f, 1.0f, 1.0f ) );
		CodeChunk b = CompileInput( compiler, CNAME( B ), shaderTarget, Vector( 1.0f, 1.0f, 1.0f, 1.0f ) );
		return CodeChunkOperators::Float4( CodeChunkOperators::Cross( a, b ).xyz(), 0.0f );
	}
};


BEGIN_CLASS_RTTI(CMaterialBlockMathCross)
	PARENT_CLASS(CMaterialBlock);
END_CLASS_RTTI();
IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathCross );

#endif