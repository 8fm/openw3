/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/materialCompiler.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Coard reflection
class CMaterialBlockCoarseReflection : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockCoarseReflection, CMaterialBlock, "System Samplers", "Coarse Reflection" );

protected:
	Bool	m_ForceGlobalReflection;

public:
	CMaterialBlockCoarseReflection()
		: m_ForceGlobalReflection( false )
	{};

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Roughness ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
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

		// Get view vector
		CodeChunk viewDir;
		{
			CodeChunk vertexPos = SHADER_DATA( "WorldPosition", shaderTarget );
			CodeChunk cameraPos;
			if (shaderTarget == MSH_PixelShader)
			{
				cameraPos = SHADER_CONST_FLOAT3( PSC_CameraPosition, shaderTarget );
			}
			viewDir = SHADER_VAR_FLOAT3( ( cameraPos.xyz() - vertexPos.xyz() ).Normalize(), shaderTarget );
		}

		// Get some simple params
		CodeChunk vertexPos = PS_VAR_FLOAT3( PS_DATA( "WorldPosition" ).xyz() );
		CodeChunk roughness	= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Roughness ), shaderTarget,   MATERIAL_DEFAULT_ROUGHNESS  ).x() );
		CodeChunk screenPos = PS_VAR_FLOAT2( PS_DATA( "ScreenVPOS" ).xy() );

		// Calc ambient
		CodeChunk reflectionResult = compiler.GetPS().AutomaticName();
		CodeChunk call = CodeChunk::Printf( false, "float3 %s = CalcEnvProbeReflection_NormalBasedMipLod( %s, %s, %s, %s, %s, %s );", reflectionResult.AsChar(), vertexPos.xyz().AsChar(), viewDir.xyz().AsChar(), normal.xyz().AsChar(), roughness.AsChar(), screenPos.AsChar(), (m_ForceGlobalReflection ? "false" : "true") );
		compiler.GetPS().Statement( call );

		//
		return Float4 ( reflectionResult, 1.f );
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockCoarseReflection)
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_ForceGlobalReflection, TXT("Force global reflection") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockCoarseReflection );

#endif