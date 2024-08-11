/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Add two values
class CMaterialBlockMathNormalToDerivative : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathNormalToDerivative, CMaterialBlock, "Deprecated", "NormalToDerivative" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( NormalVector ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk a = CompileInput( compiler, CNAME( NormalVector ), shaderTarget, CodeChunk( Vector(0.0f, 0.0f, 1.0f, 0.0f) ) );
		return SHADER_VAR_FLOAT4( CodeChunkOperators::Float4( a.x() / a.z(), a.y() / a.z(), CodeChunk(0.0f), CodeChunk(1.0f) ), shaderTarget );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathNormalToDerivative , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathNormalToDerivative );

#endif