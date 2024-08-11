/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

#include "../matcompiler//materialShaderConstants.h"
#include "../engine/materialBlock.h"
using namespace CodeChunkOperators;

/// Multiply two values
class CMaterialBlockMathReflection : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathReflection, CMaterialBlock, "Math", "Reflection" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		ASSERT(shaderTarget==MSH_PixelShader, TXT("No camera position in vertex shaders so no reflection in vertex shaders, if you need this desperately than ask nicely."));

		// Get input vector
		CodeChunk vec;
		if ( HasInput( CNAME( In ) ) )
		{
			// Use given vector
			vec = CompileInput( compiler, CNAME( In ), shaderTarget, 0.0f );
		}
		else
		{
			// Calculate direction to camera
			CodeChunk vertexPos = SHADER_DATA( "WorldPosition", shaderTarget );
			CodeChunk cameraPos;
			if (shaderTarget == MSH_PixelShader)
			{
				 cameraPos = SHADER_CONST_FLOAT3( PSC_CameraPosition, shaderTarget );
			}
			vec = SHADER_VAR_FLOAT3( ( cameraPos.xyz() - vertexPos.xyz() ).Normalize(), shaderTarget );
		}

		// Get normal
		CodeChunk normal;
		if ( HasInput( CNAME( Normal ) ) )
		{
			normal = CompileInput( compiler, CNAME( Normal ), shaderTarget );
		}
		else
		{
			normal = SHADER_DATA( "WorldNormal", shaderTarget );
		}

		// Reflect
		return SHADER_VAR_FLOAT4( Float4( -Reflect( vec, normal ).xyz(), 0.0f ), shaderTarget );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathReflection , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathReflection );

#endif