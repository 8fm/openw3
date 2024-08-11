/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

RED_DEFINE_STATIC_NAME( MaxDepth );
RED_DEFINE_STATIC_NAME( WorldPosition );
RED_DEFINE_STATIC_NAME( Blur );

/// Distance from the world position to the first light blocker calculated along global light direction - used for faking translucency
class CMaterialBlockShadowSurfaceDepth : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockShadowSurfaceDepth, CMaterialBlock, "System Samplers", "Shadow Surface Depth" );

public:
	CMaterialBlockShadowSurfaceDepth ()
	{}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Blur ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( MaxDepth ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( WorldPosition ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		using namespace CodeChunkOperators;

		CodeChunk result = Float4 ( 1.f, 1.f, 1.f, 1.f );

		CodeChunk blur = Float4 ( 1.f, 1.f, 1.f, 1.f );

		if ( HasInput( CNAME( Blur ) ) )
		{
			blur = CompileInput( compiler, CNAME( Blur ), shaderTarget );	
		}

		CodeChunk maxDepth = Float4 ( 15.f, 15.f, 15.f, 15.f );


		if ( HasInput( CNAME( MaxDepth ) ) )
		{
			maxDepth = CompileInput( compiler, CNAME( MaxDepth ), shaderTarget );	
		}

		CodeChunk worldPos = SHADER_VAR_FLOAT3( SHADER_DATA( "WorldPosition", shaderTarget ).xyz(), shaderTarget );

		if ( HasInput( CNAME( WorldPosition ) ) )
		{
			worldPos = CompileInput( compiler, CNAME( WorldPosition ), shaderTarget );	
		}


		compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );

		CodeChunk param = compiler.GetPS().AutomaticName();
		CodeChunk call = CodeChunk::Printf( false, "float %s = CalculateShadowSurfaceDepth( %s, %s.x, %s.x );", param.AsChar(), worldPos.AsChar(), maxDepth.AsChar(), blur.AsChar() );
		compiler.GetPS().Statement( call );

		result = PS_VAR_FLOAT4( param.xxxx() );
		return result;
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockShadowSurfaceDepth )
	PARENT_CLASS(CMaterialBlock)	
	END_CLASS_RTTI()

	IMPLEMENT_ENGINE_CLASS( CMaterialBlockShadowSurfaceDepth );

#endif