/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbMathWorldTangentRecalculation.h"
#include "../engine/graphBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockWorldTangentRecalculation );

CMaterialBlockWorldTangentRecalculation::CMaterialBlockWorldTangentRecalculation()
	: m_worldToTangent( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockWorldTangentRecalculation::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );

}

#endif

String CMaterialBlockWorldTangentRecalculation::GetCaption() const
{
	if ( m_worldToTangent == true )
	{
		return TXT("World to Tangent");
	}

	else
	{
		return TXT("Tangent to World");
	}
}

CodeChunk CMaterialBlockWorldTangentRecalculation::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	CodeChunk in = CompileInput( compiler, CNAME( In ), shaderTarget, 0.0f );

	if ( m_worldToTangent == true )
	{
		CodeChunk worldNormal = CompileInput( compiler, CNAME( In ), shaderTarget, 1.0f );

		CodeChunk TBN = PS_DATA( "TBNMatrix" );

		CodeChunk tangentNormal = Mul( TBN, worldNormal.xyz() );
		return PS_VAR_FLOAT4( Float4( Normalize( tangentNormal.xyz() ), 0.0f ));
	}

	else
	{
		// Get tangent space basis in world space
		CodeChunk basisX = SHADER_DATA( "WorldTangent", shaderTarget );
		CodeChunk basisY = SHADER_DATA( "WorldBinormal", shaderTarget );
		CodeChunk basisZ = SHADER_DATA( "WorldNormal", shaderTarget );

		// Convert to world space
		CodeChunk worldNormalX = SHADER_VAR_FLOAT3( basisX * in.x(), shaderTarget );
		CodeChunk worldNormalY = SHADER_VAR_FLOAT3( basisY * in.y(), shaderTarget );
		CodeChunk worldNormalZ = SHADER_VAR_FLOAT3( basisZ * in.z(), shaderTarget );
		CodeChunk worldNormal = worldNormalX + worldNormalY + worldNormalZ;
		return SHADER_VAR_FLOAT4( Float4( worldNormal, 1.0f ), shaderTarget );
	}
}

#endif