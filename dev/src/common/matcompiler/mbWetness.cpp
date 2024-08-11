/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbWetness.h"
#include "../engine/graphBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
RED_DEFINE_STATIC_NAME( SkeletalExtraData );
RED_DEFINE_STATIC_NAME( SpecularColor );

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockWetness );
IMPLEMENT_RTTI_ENUM(EWetnessOverrideType);

/// Split vector into components
CMaterialBlockWetness::CMaterialBlockWetness()
	: m_overrideType( EM_Diffuse )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockWetness::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );    

	CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( SkeletalExtraData )) ); 
	CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( Roughness )) );
	
	switch ( m_overrideType )
	{
	case EM_Diffuse:				
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( DiffuseColor )) );
		break;	

	case EM_Specular:				
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( DiffuseColor )) ); 
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( SpecularColor )) ); 
		break;
	}

	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Out ), Color::WHITE) );
}

String CMaterialBlockWetness::GetCaption() const
{
	switch ( m_overrideType )
	{
		case EM_Diffuse:
			return TXT("Wetness Diffuse");
			break;

		case EM_Roughness:
			return TXT("Wetness Roughness");
			break;

		case EM_Specular:
			return TXT("Wetness Specular");	
			break;
	}
	
	return TXT("Wetness - TYPE ERROR ");
}

#endif

CodeChunk CMaterialBlockWetness::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	RED_ASSERT( shaderTarget == MSH_PixelShader );

	CodeChunk param;

	if( shaderTarget == MSH_PixelShader )
	{
		compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );
		param = compiler.GetPS().AutomaticName();
	}

	CodeChunk statement;
	CodeChunk wet = CompileInput( compiler, CNAME( SkeletalExtraData ), shaderTarget, Float3( 0.0f, 0.0f, 0.0f ) );
	CodeChunk rou = CompileInput( compiler, CNAME( Roughness ), shaderTarget, Vector( 1.0f, 1.0f, 1.0f, 1.0f ) );
	CodeChunk dif;

	switch( m_overrideType )
	{
	case EM_Diffuse:
		dif = CompileInput( compiler, CNAME( DiffuseColor ), shaderTarget, Vector( 1.0f, 1.0f, 1.0f, 1.0f ) );
		statement = CodeChunk::Printf( false, "float4 %s = CalculateWetnessDiffuse( (%s).xyz, (%s).xyzw, (%s).x );", param.AsChar(), wet.AsChar(), dif.AsChar(), rou.AsChar() );
		break;

	case EM_Roughness:	
		statement = CodeChunk::Printf( false, "float4 %s = CalculateWetnessRoughness( (%s).xyz, (%s).x );", param.AsChar(), wet.AsChar(), rou.AsChar() );
		break;

	case EM_Specular:
		CodeChunk spe = CompileInput( compiler, CNAME( SpecularColor ), shaderTarget, Vector( 1.0f, 1.0f, 1.0f, 1.0f ) );
		statement = CodeChunk::Printf( false, "float4 %s = CalculateWetnessSpecular( (%s).xyz, (%s).x, (%s).xyz );", param.AsChar(), wet.AsChar(), rou.AsChar(), spe.AsChar());
		break;
	}	
	
	compiler.GetPS().Statement( statement );	
	return SHADER_VAR_FLOAT4( param.AsChar(), shaderTarget );
}



#endif