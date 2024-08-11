/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

#include "../matcompiler//materialShaderConstants.h"
#include "../engine/graphConnectionRebuilder.h"

using namespace CodeChunkOperators;

/// Simple Fresnel effect
class CMaterialBlockMathFresnel : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathFresnel, CMaterialBlock, "Math", "Fresnel" );

public:
	Float		m_power;		// Fresnel power
	Bool		m_twoSided;		// Two sided

public:
	CMaterialBlockMathFresnel()
		: m_power( 2.0f )
		, m_twoSided( false )
	{
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		ASSERT(shaderTarget==MSH_PixelShader, TXT("No camera position in vertex shaders so no fresnel in vertex shaders, if you need this desperately than ask nicely."));

		// Calculate direction to camera
		CodeChunk vertexPos = SHADER_DATA( "WorldPosition", shaderTarget );
		
		CodeChunk cameraPos;
		if (shaderTarget == MSH_PixelShader)
		{
			cameraPos = SHADER_CONST_FLOAT3( PSC_CameraPosition, shaderTarget );
		}
		CodeChunk dirToCamera = Normalize( cameraPos.xyz() - vertexPos.xyz() );

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

		// Calculate Fresnel term
		if ( m_twoSided )
		{
			CodeChunk fresnelTerm = Pow( Saturate( 1.0f - Abs( Dot3( normal, dirToCamera ) ) ), m_power );
			return Float4( fresnelTerm.xxxx() );
		}
		else
		{
			CodeChunk fresnelTerm = Pow( Saturate( 1.0f - Dot3( normal, dirToCamera ) ), m_power );
			return Float4( fresnelTerm.xxxx() );
		}
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockMathFresnel)
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_power, TXT("Fresnel exponent") );
	PROPERTY_EDIT( m_twoSided, TXT("Is two sided (ignore negative vector)") );
END_CLASS_RTTI()


IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathFresnel );

#endif