/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Append one normal to another
class CMaterialBlockCombineNormals : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockCombineNormals, CMaterialBlock, "Deprecated", "Combine Normals" );

protected:
	Float		m_firstWeight;
	Float		m_secondWeight;
	Bool		m_tangentToWorld;

public:
	CMaterialBlockCombineNormals()
		: m_firstWeight( 1.0f )
		, m_secondWeight( 1.0f )
		, m_tangentToWorld( true )
	{};

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( First ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Second ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( FirstWeight ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( SecondWeight ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk first = PS_VAR_FLOAT3( CompileInput( compiler, CNAME( First ), shaderTarget, Float4( 0.0f, 0.0f, 1.0f, 0.0f ) ) );
		CodeChunk second = PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Second ), shaderTarget, Float4( 0.0f, 0.0f, 1.0f, 0.0f ) ) );

		CodeChunk firstWeight = CompileInput( compiler, CNAME( FirstWeight ), shaderTarget, m_firstWeight );
		CodeChunk secondWeight = CompileInput( compiler, CNAME( SecondWeight ), shaderTarget, m_secondWeight );

		// Compose normal through partial derivatives addition
		// http://diaryofagraphicsprogrammer.blogspot.com/2009/01/partial-derivative-normal-maps.html
		CodeChunk dxy = PS_VAR_FLOAT2( -first.xy()/first.zz() );
		CodeChunk dxy2 = PS_VAR_FLOAT2( -second.xy()/second.zz() );
		
		CodeChunk finalNormal = PS_VAR_FLOAT3( Float3( -firstWeight * dxy + -secondWeight * dxy2, 1.0f ) );
		finalNormal = Normalize( finalNormal );

		if ( m_tangentToWorld )
		{
			// Get tangent space basis in world space
			CodeChunk basisX = PS_DATA( "WorldTangent" );
			CodeChunk basisY = PS_DATA( "WorldBinormal" );
			CodeChunk basisZ = PS_DATA( "WorldNormal" );

			// Convert to world space
			CodeChunk worldNormalX = PS_VAR_FLOAT3( basisX * finalNormal.x() );
			CodeChunk worldNormalY = PS_VAR_FLOAT3( basisY * finalNormal.y() );
			CodeChunk worldNormalZ = PS_VAR_FLOAT3( basisZ * finalNormal.z() );
			CodeChunk worldNormal = worldNormalX + worldNormalY + worldNormalZ;
			return PS_VAR_FLOAT4( Float4( worldNormal, 0.0f ) );
		}
		else
		{
			return PS_VAR_FLOAT4( Float4( finalNormal, 0.0f ) );
		}
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockCombineNormals)
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_firstWeight, TXT("Weight of first normalmap") );
	PROPERTY_EDIT( m_secondWeight, TXT("Weight of second normalmap") );
	PROPERTY_EDIT( m_tangentToWorld, TXT("Transform output normal to world space") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockCombineNormals );

#endif