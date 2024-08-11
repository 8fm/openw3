/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Split vector into components
class CMaterialBlockMathAppendVector : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathAppendVector, CMaterialBlock, "Deprecated", "AppendVector" );

public:


#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );    
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( X ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( Y ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( Z ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( W ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk x = CompileInput( compiler, CNAME( X ), shaderTarget, 0.0f );
		CodeChunk y = CompileInput( compiler, CNAME( Y ), shaderTarget, 0.0f );
		CodeChunk z = CompileInput( compiler, CNAME( Z ), shaderTarget, 0.0f );
		CodeChunk w = CompileInput( compiler, CNAME( W ), shaderTarget, 0.0f );
		return Float4( x.x(),y.x(),z.x(),w.x() );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathAppendVector, CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathAppendVector );

#endif