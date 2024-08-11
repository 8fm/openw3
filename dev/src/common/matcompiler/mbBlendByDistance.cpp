/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Blend two values by distance
class CMaterialBlockBlendByDistance : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockBlendByDistance, CMaterialBlock, "Deprecated", "Blend by Distnace" );

protected:
	Float		m_BlendStartDistance;
	Float		m_BlendEndDistance;

public:
	CMaterialBlockBlendByDistance()
		: m_BlendStartDistance( 10.0f )
		, m_BlendEndDistance( 30.0f )
	{};
	

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Near ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Far ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( StartDist ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( EndDist ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk startDist;
		if ( HasInput(CNAME( StartDist ) ) )
		{
			startDist = CompileInput( compiler, CNAME( StartDist ), shaderTarget );
		}
		else
		{
			startDist = m_BlendStartDistance;
		}

		CodeChunk endDist;
		if ( HasInput(CNAME( EndDist ) ) )
		{
			endDist = CompileInput( compiler, CNAME( EndDist ), shaderTarget );
		}
		else
		{
			endDist = m_BlendEndDistance;
		}

		if ( startDist != endDist )
		{
			CodeChunk a = CompileInput( compiler, CNAME( Near ), shaderTarget, 0.0f );
			CodeChunk b = CompileInput( compiler, CNAME( Far ), shaderTarget, 0.0f );
			CodeChunk distance = PS_DATA( "CameraDistance" );
			CodeChunk frac = PS_VAR_FLOAT( Saturate( ( distance - startDist )  / ( endDist - startDist ) ) );
			return Lerp( a, b, frac );
		}
		else
		{
			return CompileInput( compiler, CNAME( Near ), shaderTarget, 0.0f );
		}
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockBlendByDistance)
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_BlendStartDistance, TXT("Blend start distance") );
	PROPERTY_EDIT( m_BlendEndDistance, TXT("Blend end distance") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockBlendByDistance );

#endif