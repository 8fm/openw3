/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Blend two values by distance
class CMaterialBlockTexCoordMad : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockTexCoordMad, CMaterialBlock, "Deprecated", "Tex coord mad" );

public:
	CMaterialBlockTexCoordMad()
	{};

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( TexCoord ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( ShiftScale ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk a = CompileInput( compiler, CNAME( TexCoord ), shaderTarget, Vector( 0.5f, 0.5f, 0.0f, 0.0f ) );
		CodeChunk b = CompileInput( compiler, CNAME( ShiftScale ), shaderTarget, Vector( 0.0f, 0.0f, 1.0f, 1.0f ) );						
		return a.xy() * b.zw() + b.xy();
	}

};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockTexCoordMad , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockTexCoordMad );

#endif