/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Split vector into components
class CMaterialBlockMathSplitVector : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathSplitVector, CMaterialBlock, "Deprecated", "SplitVector" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );    
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( In )) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( X ), Color::RED ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("yyyy"), CNAME( Y ), Color::GREEN ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("zzzz"), CNAME( Z ), Color::BLUE ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( W ), Color( 127, 127, 127 ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk a = CompileInput( compiler, CNAME( In ), shaderTarget, 0.0f );
		return a;
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathSplitVector , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathSplitVector );

#endif