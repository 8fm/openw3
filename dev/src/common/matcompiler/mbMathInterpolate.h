/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Interpolate vlaue
class CMaterialBlockMathInterpolate : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathInterpolate, CMaterialBlock, "Math", "Interpolate" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( A ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( B ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Fraction ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk a = CompileInput( compiler, CNAME( A ), shaderTarget, 0.0f );
		CodeChunk b = CompileInput( compiler, CNAME( B ), shaderTarget, 1.0f );
		CodeChunk f = CompileInput( compiler, CNAME( Fraction ), shaderTarget, 0.0f );
		return Lerp( a, b, f.xxxx() );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathInterpolate , CMaterialBlock);
#endif