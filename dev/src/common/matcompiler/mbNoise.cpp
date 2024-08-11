/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbNoise.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockNoise );
IMPLEMENT_RTTI_ENUM(ENoiseTypes);

/// Split vector into components
CMaterialBlockNoise::CMaterialBlockNoise()
	:m_noiseType( Noise2D ) 
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockNoise::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );    
	CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( In )) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( Out ), Color::WHITE) );
}

String CMaterialBlockNoise::GetCaption() const
{
	switch (m_noiseType)
	{
		case Noise1D: return TXT("Noise 1D");
		case Noise2D: return TXT("Noise 2D");
		default: return TXT("Noise");
	}
}

#endif

CodeChunk CMaterialBlockNoise::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	CodeChunk input = CompileInput( compiler, CNAME( In ), shaderTarget, Vector( 1.0f, 1.0f, 1.0f, 1.0f ) );
	
	/*
	ja bym sie zastanowil jak to zebrac do jednej kupy zmiennych
	plus opisac co robi kazda zmienna dla noise 1d i dla noise 2d
	*/

	CodeChunk paramA;
	CodeChunk paramB;
	CodeChunk paramC;
	CodeChunk paramD;
	CodeChunk paramF;
	CodeChunk paramG;
	CodeChunk paramXF;
	CodeChunk paramXG;
	CodeChunk paramYF;
	CodeChunk paramYG;
	CodeChunk paramAB;
	CodeChunk paramCD;
	CodeChunk paramResult;

	CodeChunk randomA;
	CodeChunk randomB;
	CodeChunk randomC;
	CodeChunk randomD;
	CodeChunk f;
	CodeChunk g;
	CodeChunk xf;
	CodeChunk xg;
	CodeChunk yf;
	CodeChunk yg;
	CodeChunk result;
	CodeChunk easeAB;
	CodeChunk easeCD;

	if( shaderTarget == MSH_PixelShader )
	{
		compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );

		paramA = compiler.GetPS().AutomaticName();
		paramB = compiler.GetPS().AutomaticName();
		paramC = compiler.GetPS().AutomaticName();
		paramD = compiler.GetPS().AutomaticName();

		paramF = compiler.GetPS().AutomaticName();
		paramG = compiler.GetPS().AutomaticName();
		paramXF = compiler.GetPS().AutomaticName();
		paramXG = compiler.GetPS().AutomaticName();
		paramYF = compiler.GetPS().AutomaticName();
		paramYG = compiler.GetPS().AutomaticName();

		paramAB = compiler.GetPS().AutomaticName();
		paramCD = compiler.GetPS().AutomaticName();

		paramResult = compiler.GetPS().AutomaticName();
	}

	if( shaderTarget == MSH_VertexShader )
	{
		compiler.GetMaterialCompiler()->GetVertexShaderCompiler()->Include( "include_constants.fx" );

		paramA = compiler.GetVS().AutomaticName();
		paramB = compiler.GetVS().AutomaticName();
		paramC = compiler.GetVS().AutomaticName();
		paramD = compiler.GetVS().AutomaticName();

		paramF = compiler.GetVS().AutomaticName();
		paramG = compiler.GetVS().AutomaticName();
		paramXF = compiler.GetVS().AutomaticName();
		paramXG = compiler.GetVS().AutomaticName();
		paramYF = compiler.GetVS().AutomaticName();
		paramYG = compiler.GetVS().AutomaticName();

		paramAB = compiler.GetVS().AutomaticName();
		paramCD = compiler.GetVS().AutomaticName();

		paramResult = compiler.GetVS().AutomaticName();
	}


	switch (m_noiseType)
	{
	case Noise1D:
		randomA = CodeChunk::Printf( false, "float %s = CalculateNoise1D( asuint( floor( ( %s ).x ) ) );", paramA.AsChar(), input.AsChar());
		randomB = CodeChunk::Printf( false, "float %s = CalculateNoise1D( asuint( floor( ( %s ).x - 1.0f ) ) );", paramB.AsChar(), input.AsChar());
		f = CodeChunk::Printf( false, "float %s = 1.0f - frac(%s).x;", paramF.AsChar(), input.AsChar());
		g = CodeChunk::Printf( false, "float %s = -2.0f*%s*%s*%s + 3.0f*%s*%s;", paramG.AsChar(), paramF.AsChar(), paramF.AsChar(), paramF.AsChar(), paramF.AsChar(), paramF.AsChar());
		result = CodeChunk::Printf( false, "float %s = lerp( %s, %s, %s);", paramResult.AsChar(), paramA.AsChar(), paramB.AsChar(), paramG.AsChar() );
		break;

	case Noise2D:

		randomA = CodeChunk::Printf( false, "float %s = CalculateNoise2D(  asuint( floor( ( %s ).x ) ), asuint( floor( ( %s ).y  ) ) );", paramA.AsChar(), input.AsChar(), input.AsChar());
		randomB = CodeChunk::Printf( false, "float %s = CalculateNoise2D(  asuint( floor( ( %s ).x - 1.0f ) ), asuint( floor( ( %s ).y ) ) );", paramB.AsChar(), input.AsChar(), input.AsChar());
		randomC = CodeChunk::Printf( false, "float %s = CalculateNoise2D(  asuint( floor( ( %s ).x ) ), asuint( floor( ( %s ).y - 1.0f ) ) );", paramC.AsChar(), input.AsChar(), input.AsChar());
		randomD = CodeChunk::Printf( false, "float %s = CalculateNoise2D(  asuint( floor( ( %s ).x - 1.0f ) ), asuint( floor( ( %s ).y - 1.0f ) ) );", paramD.AsChar(), input.AsChar(), input.AsChar());

		xf = CodeChunk::Printf( false, "float %s = 1.0f - frac(%s).x;", paramXF.AsChar(), input.AsChar());
		xg = CodeChunk::Printf( false, "float %s = -2.0f*%s*%s*%s + 3.0f*%s*%s;", paramXG.AsChar(), paramXF.AsChar(), paramXF.AsChar(), paramXF.AsChar(), paramXF.AsChar(), paramXF.AsChar());
		yf = CodeChunk::Printf( false, "float %s = 1.0f - frac(%s).y;", paramYF.AsChar(), input.AsChar());
		yg = CodeChunk::Printf( false, "float %s = -2.0f*%s*%s*%s + 3.0f*%s*%s;", paramYG.AsChar(), paramYF.AsChar(), paramYF.AsChar(), paramYF.AsChar(), paramYF.AsChar(), paramYF.AsChar());
		easeAB = CodeChunk::Printf( false, "float %s = lerp( %s, %s, %s );", paramAB.AsChar(), paramA.AsChar(), paramB.AsChar(), paramXG.AsChar());
		easeCD = CodeChunk::Printf( false, "float %s = lerp( %s, %s, %s );", paramCD.AsChar(), paramC.AsChar(), paramD.AsChar(), paramXG.AsChar());
		result = CodeChunk::Printf( false, "float %s = lerp( %s, %s, %s);", paramResult.AsChar(), paramAB.AsChar(), paramCD.AsChar(), paramYG.AsChar() );
		break;

	default: 
		result = CodeChunk::Printf( false, "float %s = float4(0.0f, 0.0f, 0.0f, 1.0f);", paramResult.AsChar());
		break;
	}

	if( shaderTarget == MSH_PixelShader )
	{
		compiler.GetPS().Statement( randomA );
		compiler.GetPS().Statement( randomB );
		compiler.GetPS().Statement( randomC );
		compiler.GetPS().Statement( randomD );
		compiler.GetPS().Statement( f );
		compiler.GetPS().Statement( g );
		compiler.GetPS().Statement( xf );
		compiler.GetPS().Statement( xg );
		compiler.GetPS().Statement( yf );
		compiler.GetPS().Statement( yg );
		compiler.GetPS().Statement( easeAB);
		compiler.GetPS().Statement( easeCD );
		compiler.GetPS().Statement( result );
	}

	if( shaderTarget == MSH_VertexShader )
	{
		compiler.GetVS().Statement( randomA );
		compiler.GetVS().Statement( randomB );
		compiler.GetVS().Statement( randomC );
		compiler.GetVS().Statement( randomD );
		compiler.GetVS().Statement( f );
		compiler.GetVS().Statement( g );
		compiler.GetVS().Statement( xf );
		compiler.GetVS().Statement( xg );
		compiler.GetVS().Statement( yf );
		compiler.GetVS().Statement( yg );
		compiler.GetVS().Statement( easeAB);
		compiler.GetVS().Statement( easeCD );
		compiler.GetVS().Statement( result );
	}

	return SHADER_VAR_FLOAT4( paramResult, shaderTarget );
}



#endif