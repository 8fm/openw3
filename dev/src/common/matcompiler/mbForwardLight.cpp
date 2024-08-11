/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialCompiler.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

RED_DEFINE_STATIC_NAME( WorldPosition );
RED_DEFINE_STATIC_NAME( Albedo );

/// Render feedback data fetch
class CMaterialBlockForwardLight : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockForwardLight, CMaterialBlock, "System Samplers", "Forward Light" );

public:
	CMaterialBlockForwardLight ()
	{}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
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

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
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
			albedo = Float3(1.0f,0.0f,0.0f);
		}

		CodeChunk specularity;
		if ( HasInput( CNAME( Specularity ) ) )
		{
			hasSpecular = true;
			specularity = PS_VAR_FLOAT( CompileInput( compiler, CNAME( Specularity ), shaderTarget ).x() );			
		}
		else
		{
			specularity = Float2(0.0f,0.0f).x();
		}

		CodeChunk translucency;
		if ( HasInput( CNAME( Translucency ) ) )
		{
			translucency = PS_VAR_FLOAT( CompileInput( compiler, CNAME( Translucency ), shaderTarget ).x() );
		}
		else
		{
			translucency = Float2(0.0f,0.0f).x();
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
			compiler.GetPS().Statement( CodeChunk( "#ifdef __PSSL__" ) );
			compiler.GetPS().Statement( CodeChunk( "SCompileInLightingParams extraLightingParams;" ) );
			compiler.GetPS().Statement( CodeChunk( "#else" ) );
			compiler.GetPS().Statement( CodeChunk( "SCompileInLightingParams extraLightingParams = (SCompileInLightingParams)0;" ) );
			compiler.GetPS().Statement( CodeChunk( "#endif" ) );
			
			// forward lighting calculation			
			CodeChunk call = CodeChunk::Printf( false, "float3 %s = CalculateLightingPBRPipeline( %s, %s, %s, %s, %s, %s, %s, %s, extraLightingParams, false, false );", 
				param.AsChar(), 
				vertexPos.AsChar(),
				albedo.AsChar(),
				vertexNormal.AsChar(),
				faceNormal.AsChar(),
				specularity.AsChar(),
				roughness.AsChar(),
				translucency.AsChar(),
				screenPos.AsChar() );			

		compiler.GetPS().Statement( call );

		result = PS_VAR_FLOAT4( Float4(param.xyz(),1.0f) );

		return result;
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockForwardLight )
	PARENT_CLASS(CMaterialBlock)	
	END_CLASS_RTTI()
	IMPLEMENT_ENGINE_CLASS( CMaterialBlockForwardLight );

#endif
