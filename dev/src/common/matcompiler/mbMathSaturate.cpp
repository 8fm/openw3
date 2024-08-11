/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Saturate value to 0-1 range
class CMaterialBlockMathSaturate : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathSaturate, CMaterialBlock, "Math", "Saturate" );

public:
	Float	m_min;
	Float	m_max;

public:
	CMaterialBlockMathSaturate()
	{
		m_min = 0.0f;
		m_max = 1.0f;
	}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk a = CompileInput( compiler, CNAME( In ), shaderTarget, 1.0f );
		if ( m_min == 0.0f && m_max == 1.0f )
		{
			return SHADER_VAR_FLOAT4( a.Saturate(), shaderTarget );
		}
		else
		{
			return SHADER_VAR_FLOAT4( Clamp( a, m_min, m_max ), shaderTarget );
		}
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockMathSaturate)
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_min, TXT("Minimum value") );
	PROPERTY_EDIT( m_max, TXT("Maximum value") );
END_CLASS_RTTI()


IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathSaturate );

#endif