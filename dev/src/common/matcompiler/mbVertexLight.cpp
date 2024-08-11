/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbVertexLight.h"
#include "../engine/graphBlock.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

RED_DEFINE_STATIC_NAME( Albedo );
RED_DEFINE_STATIC_NAME( ShadowSamples );
RED_DEFINE_STATIC_NAME( SamplingRadius );

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockVertexLight );

CMaterialBlockVertexLight::CMaterialBlockVertexLight()
{
}
#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockVertexLight::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( ShadowSamples ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( SamplingRadius ) ) );

}

#endif

CodeChunk CMaterialBlockVertexLight::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{

	compiler.GetMaterialCompiler()->GetVertexShaderCompiler()->Include( "include_constants.fx" );

	CodeChunk vsResult = Float4 ( 1.0f, 1.0f, 1.0f, 1.0f );
	CodeChunk psResult = compiler.GetPS().Var( MDT_Float4, 0.0f );

	CodeChunk vertexPos = compiler.GetVS().Data( "WorldPosition" );

	CodeChunk shadowSamples;
	if ( HasInput( CNAME( ShadowSamples ) ) )
	{
		shadowSamples = compiler.GetVS().Var(MDT_Float3, CompileInput( compiler, CNAME( ShadowSamples ), MSH_VertexShader )).xxx();
	}
	else
	{
		shadowSamples = 1.0f;
	}

	CodeChunk samplingRadius;
	if ( HasInput( CNAME( SamplingRadius ) ) )
	{
		samplingRadius = compiler.GetVS().Var(MDT_Float3, CompileInput( compiler, CNAME( SamplingRadius ), MSH_VertexShader )).xxx();
	}
	else
	{
		samplingRadius = 1.0f;
	}

	CodeChunk screenPos = compiler.GetVS().Data( "ScreenPosition" ).xy();

	CodeChunk param = compiler.GetVS().AutomaticName();

	// build extra lighting params
	compiler.GetVS().Statement( CodeChunk::Printf( false, "SCompileInLightingParams extraLightingParams = (SCompileInLightingParams)0;" ) );

	// forward lighting calculation			
	CodeChunk call = CodeChunk::Printf( false, "float3 %s = CalculateVertexLitParticlePipeline( %s, %s, %s, %s, extraLightingParams);", 
		param.AsChar(), 
		vertexPos.AsChar(),
		screenPos.AsChar(),
		shadowSamples.AsChar(),
		samplingRadius.AsChar());

	compiler.GetVS().Statement( call );

	vsResult = compiler.GetVS().Var( MDT_Float4, Float4(param.xyz(),1.0f) );

	compiler.GetMaterialCompiler()->Connect(MDT_Float4, vsResult, psResult, CodeChunk::Printf(true, "TexCoords0" ) );

	return psResult;
}
#endif
