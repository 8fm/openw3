/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

class CMaterialBlockWorldToTangent : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockWorldToTangent, CMaterialBlock, "Deprecated", "World to tangent" );

public:
	CMaterialBlockWorldToTangent()
	{};

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Vector ) ) );		
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk worldNormal = CompileInput( compiler, CNAME( Vector ), shaderTarget, 1.0f );

		CodeChunk TBN = PS_DATA( "TBNMatrix" );

		CodeChunk tangentNormal = Mul( TBN, worldNormal.xyz() );
		return PS_VAR_FLOAT4( Float4( Normalize( tangentNormal.xyz() ), 0.0f ) );
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockWorldToTangent)
PARENT_CLASS(CMaterialBlock)
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockWorldToTangent );

#endif