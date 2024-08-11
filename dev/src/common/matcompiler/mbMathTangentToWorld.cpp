/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Blend two values by distance
class CMaterialBlockTangentToWorld : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockTangentToWorld, CMaterialBlock, "Deprecated", "Tangent to world" );

public:
	CMaterialBlockTangentToWorld()
	{};

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );		
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk tangentNormal = CompileInput( compiler, CNAME( Normal ), shaderTarget, 0.0f );

		// Get tangent space basis in world space
		CodeChunk basisX = SHADER_DATA( "WorldTangent", shaderTarget );
		CodeChunk basisY = SHADER_DATA( "WorldBinormal", shaderTarget );
		CodeChunk basisZ = SHADER_DATA( "WorldNormal", shaderTarget );

		// Convert to world space
		CodeChunk worldNormalX = SHADER_VAR_FLOAT3( basisX * tangentNormal.x(), shaderTarget );
		CodeChunk worldNormalY = SHADER_VAR_FLOAT3( basisY * tangentNormal.y(), shaderTarget );
		CodeChunk worldNormalZ = SHADER_VAR_FLOAT3( basisZ * tangentNormal.z(), shaderTarget );
		CodeChunk worldNormal = worldNormalX + worldNormalY + worldNormalZ;
		return SHADER_VAR_FLOAT4( Float4( worldNormal, 1.0f ), shaderTarget );
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockTangentToWorld)
	PARENT_CLASS(CMaterialBlock)
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockTangentToWorld );

#endif