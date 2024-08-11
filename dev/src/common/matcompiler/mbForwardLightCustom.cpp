/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbForwardLightCustom.h"
#include "../engine/graphBlock.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

RED_DEFINE_STATIC_NAME( WorldPosition );
RED_DEFINE_STATIC_NAME( Albedo );

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockForwardLightCustom );

CMaterialBlockForwardLightCustom::CMaterialBlockForwardLightCustom()
	:m_globalLightDiffuse(true)
	,m_globalLightSpecular(true)
	,m_deferredDiffuse(true)
	,m_deferredSpecular(true)
	,m_excludeFlags(true)
	,m_envProbes(true)
	,m_ambientOcclusion(true)
	,m_fog( true )
	,m_lightUsageMask(0) // All flags on
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockForwardLightCustom::OnPropertyPostChange( IProperty *property )
{
	// Pass to base class
	TBaseClass::OnPropertyPostChange( property );

	// Update layout
	OnRebuildSockets();
}
void CMaterialBlockForwardLightCustom::OnRebuildSockets()
{

	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( WorldPosition ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( WorldNormal ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Albedo ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Specularity ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Roughness ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Translucency ) ) );

	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
}

#endif


CodeChunk CMaterialBlockForwardLightCustom::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	using namespace CodeChunkOperators;

	compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );

	CodeChunk result = Float4 ( 1.f, 1.f, 1.f, 1.f );

	Bool hasSpecular = false;

	CodeChunk vertexPos;
	if ( HasInput( CNAME( WorldPosition ) ) )
	{
		vertexPos = CompileInput( compiler, CNAME( WorldPosition ), shaderTarget );	
	}
	else
	{
		vertexPos = PS_DATA( "WorldPosition" );
	}

	CodeChunk vertexNormal;
	if ( HasInput( CNAME( WorldNormal ) ) )
	{
		vertexNormal = CompileInput( compiler, CNAME( WorldNormal ), shaderTarget );	
	}
	else
	{
		vertexNormal = PS_DATA( "WorldNormal" );
	}

	CodeChunk albedo;
	if ( HasInput( CNAME( Albedo ) ) )
	{
		albedo = PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Albedo ), shaderTarget ) ).xyz();
	}
	else
	{
		albedo = Float3( 1.0f, 1.0f, 1.0f );
	}

	CodeChunk specularity;
	if ( HasInput( CNAME( Specularity ) ) )
	{
		hasSpecular = true;
		specularity = PS_VAR_FLOAT( CompileInput( compiler, CNAME( Specularity ), shaderTarget ).x() );			
	}
	else
	{
		specularity = 0.0f;
	}

	CodeChunk translucency;
	if ( HasInput( CNAME( Translucency ) ) )
	{
		translucency = PS_VAR_FLOAT( CompileInput( compiler, CNAME( Translucency ), shaderTarget ).x() );
	}
	else
	{
		translucency = 0.0f;
	}

	CodeChunk roughness;
	if ( HasInput( CNAME( Roughness ) ) )
	{
		roughness = PS_VAR_FLOAT( CompileInput( compiler, CNAME( Roughness ), shaderTarget ).x() );
	}
	else
	{
		roughness = 1.0f;
	}

	//dex++: vertex normal
	CodeChunk faceNormal = PS_DATA( "WorldNormal" );
	//dex--

	CodeChunk viewPos = PS_VAR_FLOAT3( PS_DATA( "ViewPosition" ).xyz() );
	CodeChunk screenPos = PS_VAR_FLOAT2( PS_DATA( "ScreenVPOS" ).xy() );

	CodeChunk viewDepth = PS_VAR_FLOAT( PS_DATA( "ViewZ" ) );

	CodeChunk param = compiler.GetPS().AutomaticName();

	// build extra lighting params
	CodeChunk extraLightingParamsName = compiler.GetPS().AutomaticName();
	compiler.GetPS().Statement( CodeChunk( "#ifdef __PSSL__" ) );
	
	compiler.GetPS().Statement( CodeChunk::Printf( false, "SCompileInLightingParams %s;", extraLightingParamsName.AsChar() ) );
	compiler.GetPS().Statement( CodeChunk( "#else" ) );
	compiler.GetPS().Statement( CodeChunk::Printf( false, "SCompileInLightingParams %s = (SCompileInLightingParams)0;", extraLightingParamsName.AsChar() ) );
	compiler.GetPS().Statement( CodeChunk( "#endif" ) );

	// forward lighting calculation			
	CodeChunk call = CodeChunk::Printf( false, "float3 %s = CalculateLightingComponents( %s, %s, %s, %s, %s, %s, %s, %s, %s, %i, %s, %s, %s, %s, %s, %s, %s, %s, false, false );", 
		param.AsChar(), 
		vertexPos.AsChar(),
		albedo.AsChar(),
		vertexNormal.AsChar(),
		faceNormal.AsChar(),
		specularity.AsChar(),
		roughness.AsChar(),
		translucency.AsChar(),
		screenPos.AsChar(),
		m_excludeFlags ? CodeChunk("true").AsChar() : CodeChunk("false").AsChar(),
		m_lightUsageMask,
		m_globalLightDiffuse ? CodeChunk("true").AsChar() : CodeChunk("false").AsChar(),
		m_globalLightSpecular ? CodeChunk("true").AsChar() : CodeChunk("false").AsChar(),
		m_deferredDiffuse ? CodeChunk("true").AsChar() : CodeChunk("false").AsChar(),
		m_deferredSpecular ? CodeChunk("true").AsChar() : CodeChunk("false").AsChar(),
		m_envProbes ? CodeChunk("true").AsChar() : CodeChunk("false").AsChar(),
		m_ambientOcclusion ? CodeChunk("true").AsChar() : CodeChunk("false").AsChar(),
		m_fog ? CodeChunk("true").AsChar() : CodeChunk("false").AsChar(),
		extraLightingParamsName.AsChar()
		)
		;			

	compiler.GetPS().Statement( call );

	result = PS_VAR_FLOAT4( Float4(param.xyz(),1.0f) );

	return result;
}

#endif