/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbMathWorldViewRecalculation.h"
#include "../engine/graphBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockWorldViewRecalculation );

CMaterialBlockWorldViewRecalculation::CMaterialBlockWorldViewRecalculation()
	: m_worldToView( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockWorldViewRecalculation::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
}

#endif

String CMaterialBlockWorldViewRecalculation::GetCaption() const
{
	if ( m_worldToView == true )
	{
		return TXT("World to View");
	}

	else
	{
		return TXT("View to World");
	}
}

CodeChunk CMaterialBlockWorldViewRecalculation::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	if ( shaderTarget == MSH_PixelShader )
	{
		compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_sharedConsts.fx" );
	}
	if ( shaderTarget == MSH_VertexShader )
	{
		compiler.GetMaterialCompiler()->GetVertexShaderCompiler()->Include( "include_sharedConsts.fx" );
	}
	
	CodeChunk inputVector = CompileInput( compiler, CNAME( In ), shaderTarget, 0.0f );
	CodeChunk matrix = compiler.GetShader(shaderTarget).AutomaticName();
	CodeChunk matrixDefinition;

	if ( m_worldToView == true )
	{
		matrixDefinition = CodeChunk::Printf( false, "float3x3 %s = worldToView;", matrix.AsChar() );
	}
	else
	{
		matrixDefinition = CodeChunk::Printf( false, "float3x3 %s = viewToWorld;", matrix.AsChar() );
	}

	compiler.GetShader(shaderTarget).Statement( matrixDefinition );
	CodeChunk outputVector = Mul( matrix, inputVector.xyzw() );
	return PS_VAR_FLOAT4( Float4(( outputVector.xyz() ), 0.0f ));
}

#endif