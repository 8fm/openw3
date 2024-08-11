/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Compile global fog data - VS or PS based.
CodeChunk CompileGlobalFogData( CMaterialBlockCompiler& compiler, Bool isSky, Bool isClouds, Bool vertexBased, Bool calcFullData )
{
	if ( vertexBased )
	{
		compiler.GetMaterialCompiler()->GetVertexShaderCompiler()->Include( "include_constants.fx" );
	}

	CodeChunk fogData;
	if ( calcFullData )
	{
		if ( vertexBased )
		{
			CodeChunk fogDataVS = compiler.GetVS().AutomaticName();
			CodeChunk worldSpacePosition = compiler.GetVS().Data( "WorldPosition" );
			compiler.GetVS().Statement( CodeChunk::Printf( false, "SFogData %s = CalculateFogFull( %s, %s, %s );", fogDataVS.AsChar(), (isSky ? "true" : "false"), (isClouds ? "true" : "false"), worldSpacePosition.xyz().AsChar() ) );

			CodeChunk vsResult0 = compiler.GetVS().AutomaticName();
			CodeChunk vsResult1 = compiler.GetVS().AutomaticName();
			compiler.GetVS().Statement( CodeChunk::Printf( false, "float4 %s = %s.paramsFog;", vsResult0.AsChar(), fogDataVS.AsChar() ) );
			compiler.GetVS().Statement( CodeChunk::Printf( false, "float4 %s = %s.paramsAerial;", vsResult1.AsChar(), fogDataVS.AsChar() ) );

			CodeChunk psResult0 = compiler.GetPS().Var( MDT_Float4, 0.0f );
			CodeChunk psResult1 = compiler.GetPS().Var( MDT_Float4, 0.0f );
			compiler.GetMaterialCompiler()->Connect( MDT_Float4, vsResult0, psResult0, CodeChunk::Printf(true, "Fog" ) );
			compiler.GetMaterialCompiler()->Connect( MDT_Float4, vsResult1, psResult1, CodeChunk::Printf(true, "Aerial" ) );

			fogData = compiler.GetPS().AutomaticName();
			compiler.GetPS().Statement( CodeChunk::Printf( false, "SFogData %s;", fogData.AsChar() ) );
			compiler.GetPS().Statement( CodeChunk::Printf( false, "%s.paramsFog = %s;", fogData.AsChar(), psResult0.AsChar() ) );
			compiler.GetPS().Statement( CodeChunk::Printf( false, "%s.paramsAerial = %s;", fogData.AsChar(), psResult1.AsChar() ) );
		}
		else
		{		
			fogData = compiler.GetPS().AutomaticName();
			CodeChunk worldSpacePosition = PS_VAR_FLOAT3( PS_DATA( "WorldPosition" ).xyz() );
			compiler.GetPS().Statement( CodeChunk::Printf( false, "SFogData %s = CalculateFogFull( %s, %s, %s );", fogData.AsChar(), (isSky ? "true" : "false"), (isClouds ? "true" : "false"), worldSpacePosition.xyz().AsChar() ) );
		}
	}
	else
	{
		if ( vertexBased )
		{
			CodeChunk fogDataVS = compiler.GetVS().AutomaticName();
			CodeChunk worldSpacePosition = compiler.GetVS().Data( "WorldPosition" );
			compiler.GetVS().Statement( CodeChunk::Printf( false, "float4 %s = CalculateFog( %s, %s, %s );", fogDataVS.AsChar(), (isSky ? "true" : "false"), (isClouds ? "true" : "false"), worldSpacePosition.xyz().AsChar() ) );

			fogData = compiler.GetPS().Var( MDT_Float4, 0.0f );
			compiler.GetMaterialCompiler()->Connect( MDT_Float4, fogDataVS, fogData, CodeChunk::Printf(true, "Fog" ) );
		}
		else
		{		
			fogData = compiler.GetPS().AutomaticName();
			CodeChunk worldSpacePosition = PS_VAR_FLOAT3( PS_DATA( "WorldPosition" ).xyz() );
			compiler.GetPS().Statement( CodeChunk::Printf( false, "float4 %s = CalculateFog( %s, %s, %s );", fogData.AsChar(), (isSky ? "true" : "false"), (isClouds ? "true" : "false"), worldSpacePosition.xyz().AsChar() ) );
		}
	}

	return fogData;
}

/// Calculate GlobalFog
class CMaterialBlockGlobalFog : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockGlobalFog, CMaterialBlock, "System Samplers", "Fog Global" );

public:	
	Bool					m_isSky;
	Bool					m_isClouds;
	Bool					m_isVertexBased;

public:
	CMaterialBlockGlobalFog()
		: m_isSky ( false )
		, m_isClouds ( false )
		, m_isVertexBased ( false )
	{};

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Color ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Data ),   Color::WHITE ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( Alpha ),  Color::WHITE ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk fog_data		= CompileGlobalFogData( compiler, m_isSky, m_isClouds, m_isVertexBased, false );
		CodeChunk result		= fog_data;

		if ( HasInput( CNAME( Color ) ) )
		{
			CodeChunk inputColor = CompileInput( compiler, CNAME( Color ), shaderTarget, Vector( 1.0f, 1.0f, 1.0f, 1.0f ) );
			CodeChunk resultColor = CodeChunk::Printf( false, "ApplyFogData( %s, %s )", inputColor.AsChar(), fog_data.AsChar() );
			result = Float4( resultColor.xyz(), result.w() );
		}

		return result;
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockGlobalFog )
	PARENT_CLASS( CMaterialBlock )	
	PROPERTY_EDIT( m_isSky, TXT("Is sky") );	
	PROPERTY_EDIT( m_isClouds, TXT("Use clouds custom fog parameters") );
	PROPERTY_EDIT( m_isVertexBased, TXT("Vertex based fog calculation") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockGlobalFog );

#endif
