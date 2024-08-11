/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Split vector into components
class CMaterialBlockNoise2D: public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockNoise2D, CMaterialBlock, "Deprecated", "Noise2D" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );    
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( In )) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( Out ), Color::WHITE) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk input = CompileInput( compiler, CNAME( In ), shaderTarget, Vector( 1.0f, 1.0f, 1.0f, 1.0f ) );
		CodeChunk paramA;
		CodeChunk paramB;
		CodeChunk paramC;
		CodeChunk paramD;
		CodeChunk paramXF;
		CodeChunk paramXG;
		CodeChunk paramYF;
		CodeChunk paramYG;
		CodeChunk paramAB;
		CodeChunk paramCD;
		CodeChunk paramResult;

		if( shaderTarget == MSH_PixelShader )
		{
			compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );

			paramA = compiler.GetPS().AutomaticName();
			paramB = compiler.GetPS().AutomaticName();
			paramC = compiler.GetPS().AutomaticName();
			paramD = compiler.GetPS().AutomaticName();


			paramXF = compiler.GetPS().AutomaticName();
			paramXG = compiler.GetPS().AutomaticName();
			paramYF = compiler.GetPS().AutomaticName();
			paramYG = compiler.GetPS().AutomaticName();

			paramAB = compiler.GetPS().AutomaticName();
			paramCD = compiler.GetPS().AutomaticName();

			paramResult = compiler.GetPS().AutomaticName();
		}

		else
		{
			compiler.GetMaterialCompiler()->GetVertexShaderCompiler()->Include( "include_constants.fx" );

			paramA = compiler.GetVS().AutomaticName();
			paramB = compiler.GetVS().AutomaticName();
			paramC = compiler.GetVS().AutomaticName();
			paramD = compiler.GetVS().AutomaticName();


			paramXF = compiler.GetVS().AutomaticName();
			paramXG = compiler.GetVS().AutomaticName();
			paramYF = compiler.GetVS().AutomaticName();
			paramYG = compiler.GetVS().AutomaticName();

			paramAB = compiler.GetVS().AutomaticName();
			paramCD = compiler.GetVS().AutomaticName();

			paramResult = compiler.GetVS().AutomaticName();
		}

		CodeChunk randomA = CodeChunk::Printf( false, "float %s = CalculateNoise2D(  asuint( floor( ( %s ).x ) ), asuint( floor( ( %s ).y  ) ) );", paramA.AsChar(), input.AsChar(), input.AsChar());
		CodeChunk randomB = CodeChunk::Printf( false, "float %s = CalculateNoise2D(  asuint( floor( ( %s ).x - 1.0f ) ), asuint( floor( ( %s ).y ) ) );", paramB.AsChar(), input.AsChar(), input.AsChar());
		CodeChunk randomC = CodeChunk::Printf( false, "float %s = CalculateNoise2D(  asuint( floor( ( %s ).x ) ), asuint( floor( ( %s ).y - 1.0f ) ) );", paramC.AsChar(), input.AsChar(), input.AsChar());
		CodeChunk randomD = CodeChunk::Printf( false, "float %s = CalculateNoise2D(  asuint( floor( ( %s ).x - 1.0f ) ), asuint( floor( ( %s ).y - 1.0f ) ) );", paramD.AsChar(), input.AsChar(), input.AsChar());

		CodeChunk xf = CodeChunk::Printf( false, "float %s = 1.0f - frac(%s).x;", paramXF.AsChar(), input.AsChar());
		CodeChunk xg = CodeChunk::Printf( false, "float %s = -2.0f*%s*%s*%s + 3.0f*%s*%s;", paramXG.AsChar(), paramXF.AsChar(), paramXF.AsChar(), paramXF.AsChar(), paramXF.AsChar(), paramXF.AsChar());
		CodeChunk yf = CodeChunk::Printf( false, "float %s = 1.0f - frac(%s).y;", paramYF.AsChar(), input.AsChar());
		CodeChunk yg = CodeChunk::Printf( false, "float %s = -2.0f*%s*%s*%s + 3.0f*%s*%s;", paramYG.AsChar(), paramYF.AsChar(), paramYF.AsChar(), paramYF.AsChar(), paramYF.AsChar(), paramYF.AsChar());
		CodeChunk easeAB = CodeChunk::Printf( false, "float %s = lerp( %s, %s, %s );", paramAB.AsChar(), paramA.AsChar(), paramB.AsChar(), paramXG.AsChar());
		CodeChunk easeCD = CodeChunk::Printf( false, "float %s = lerp( %s, %s, %s );", paramCD.AsChar(), paramC.AsChar(), paramD.AsChar(), paramXG.AsChar());
		CodeChunk result = CodeChunk::Printf( false, "float %s = lerp( %s, %s, %s);", paramResult.AsChar(), paramAB.AsChar(), paramCD.AsChar(), paramYG.AsChar() );

		if( shaderTarget == MSH_PixelShader )
		{
			compiler.GetPS().Statement( randomA );
			compiler.GetPS().Statement( randomB );
			compiler.GetPS().Statement( randomC );
			compiler.GetPS().Statement( randomD );
			compiler.GetPS().Statement( xf );
			compiler.GetPS().Statement( xg );
			compiler.GetPS().Statement( yf );
			compiler.GetPS().Statement( yg );
			compiler.GetPS().Statement( easeAB);
			compiler.GetPS().Statement( easeCD );
			compiler.GetPS().Statement( result );
		}
		else
		{
			compiler.GetVS().Statement( randomA );
			compiler.GetVS().Statement( randomB );
			compiler.GetVS().Statement( randomC );
			compiler.GetVS().Statement( randomD );
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
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockNoise2D , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockNoise2D );

#endif