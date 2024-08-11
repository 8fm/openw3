/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Add two values
class CMaterialBlockMathDerivativeToNormal : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathDerivativeToNormal, CMaterialBlock, "Deprecated", "DerivativeToNormal" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DerivativeVector ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk a = CompileInput( compiler, CNAME( DerivativeVector ), shaderTarget, CodeChunk( Vector( 0.0f, 0.0f, 1.0f, 0.0f ) ) );
		CodeChunk invZSquare = SHADER_VAR_FLOAT( (a.x() * a.x()) + (a.y() * a.y()) + CodeChunk( 1.0f ), shaderTarget );
		CodeChunk z = SHADER_VAR_FLOAT( CodeChunk( 1.0f ) / CodeChunkOperators::Sqrt( invZSquare ), shaderTarget );
		CodeChunk x = a.x() * z;
		CodeChunk y = a.y() * z;
		return SHADER_VAR_FLOAT4( CodeChunkOperators::Float4( x, y, z, CodeChunk(0.0f) ), shaderTarget );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathDerivativeToNormal , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathDerivativeToNormal );

#endif