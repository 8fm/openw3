/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Render feedback data fetch
class CMaterialBlockShadowSample : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockShadowSample, CMaterialBlock, "System Samplers", "Shadow Sample" );
	
public:
	CMaterialBlockShadowSample ()
	{}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		using namespace CodeChunkOperators;

		CodeChunk result = Float4 ( 1.f, 1.f, 1.f, 1.f );
		
		compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );

		CodeChunk worldPos = SHADER_VAR_FLOAT3( SHADER_DATA( "WorldPosition", shaderTarget ).xyz(), shaderTarget );
		CodeChunk screenPos = SHADER_VAR_FLOAT2( SHADER_DATA( "TexCoords0", shaderTarget ).xy(), shaderTarget );

		CodeChunk param = compiler.GetPS().AutomaticName();
		CodeChunk call = CodeChunk::Printf( false, "float %s = CalcFullShadowFactor( %s, %s );", param.AsChar(), worldPos.AsChar(), screenPos.AsChar() );
		compiler.GetPS().Statement( call );

		result = PS_VAR_FLOAT4( param.xxxx() );
		return result;
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockShadowSample )
	PARENT_CLASS(CMaterialBlock)	
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockShadowSample );

#endif