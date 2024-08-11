/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

#include "../matcompiler//materialShaderConstants.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"
using namespace CodeChunkOperators;

RED_DEFINE_STATIC_NAME( InR1 );
RED_DEFINE_STATIC_NAME( InR2 );

RED_DEFINE_STATIC_NAME( InG1 );
RED_DEFINE_STATIC_NAME( InG2 );

RED_DEFINE_STATIC_NAME( InB1 );
RED_DEFINE_STATIC_NAME( InB2 );

RED_DEFINE_STATIC_NAME( InRGB );


/// Multiply two values
class CMaterialBlockMathSH : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathSH, CMaterialBlock, "Deprecated", "SH" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( InR1 ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( InR2 ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( InG1 ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( InG2 ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( InB1 ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( InB2 ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( InRGB ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		// Get normal
		CodeChunk normal;
		if ( HasInput( CNAME( Normal ) ) )
		{
			normal = SHADER_VAR_FLOAT4( Float4( CompileInput( compiler, CNAME( Normal ), shaderTarget ).xyz(), 1.0f ), shaderTarget );
		}
		else
		{
			normal = SHADER_VAR_FLOAT4( Float4( SHADER_DATA( "WorldNormal", shaderTarget ).xyz(), 1.0f ), shaderTarget );
		}

		CodeChunk r1 = CompileInput( compiler, CNAME( InR1 ), shaderTarget, Float4( 0.0f,0.0f,0.0f,0.0f ) );
		CodeChunk r2 = CompileInput( compiler, CNAME( InR2 ), shaderTarget, Float4( 0.0f,0.0f,0.0f,0.0f ) );

		CodeChunk g1 = CompileInput( compiler, CNAME( InG1 ), shaderTarget, Float4( 0.0f,0.0f,0.0f,0.0f ) );
		CodeChunk g2 = CompileInput( compiler, CNAME( InG2 ), shaderTarget, Float4( 0.0f,0.0f,0.0f,0.0f ) );

		CodeChunk b1 = CompileInput( compiler, CNAME( InB1 ), shaderTarget, Float4( 0.0f,0.0f,0.0f,0.0f ) );
		CodeChunk b2 = CompileInput( compiler, CNAME( InB2 ), shaderTarget, Float4( 0.0f,0.0f,0.0f,0.0f ) );

		CodeChunk rgb = CompileInput( compiler, CNAME( InRGB ), shaderTarget, Float4( 0.0f,0.0f,0.0f,0.0f ) );

		CodeChunk x1 = SHADER_VAR_FLOAT3( Float3( Dot4(r1,normal).x(), Dot4(g1,normal).x(), Dot4(b1,normal).x() ), shaderTarget );

		CodeChunk vB = SHADER_VAR_FLOAT4( Float4( normal.x(), normal.y(), normal.z(), normal.z() ) * Float4( normal.y(), normal.z(), normal.z(), normal.x() ), shaderTarget );

		CodeChunk x2 = SHADER_VAR_FLOAT3( Float3( Dot4(r2,vB).x(), Dot4(g2,vB).x(), Dot4(b2,vB).x() ), shaderTarget );

		CodeChunk vC = SHADER_VAR_FLOAT( normal.x() * normal.x() - normal.y() * normal.y(), shaderTarget );

		CodeChunk x3 = SHADER_VAR_FLOAT3( vC.xxx() * rgb.xyz(), shaderTarget );

		// Reflect
		return SHADER_VAR_FLOAT4( Float4( x1+x2+x3, 0.0f ), shaderTarget );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathSH , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathSH );

#endif