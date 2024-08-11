/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Blend two values by distance
class CMaterialBlockNormalmapBlend : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockNormalmapBlend, CMaterialBlock, "Image Processing", "Blend Normals" );

protected:
	Float		m_firstMapWeight;
	Float		m_secondMapWeight;

public:
	CMaterialBlockNormalmapBlend()
		: m_firstMapWeight( 1.0f )
		, m_secondMapWeight( 1.0f )
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
		CodeChunk first = CompileInput( compiler, CNAME( First ), shaderTarget, 0.0f );
		CodeChunk second = CompileInput( compiler, CNAME( Second ), shaderTarget, 0.0f );

		CodeChunk firstWeight = CompileInput( compiler, CNAME( FirstWeight ), shaderTarget, m_firstMapWeight );
		CodeChunk secondWeight = CompileInput( compiler, CNAME( SecondWeight ), shaderTarget, m_secondMapWeight );

		CodeChunk retVal = Float3( firstWeight.x() * first.x() * second.z() + secondWeight.x() * second.x() * first.z(),
								   firstWeight.x() * first.y() * second.z() + secondWeight.x() * second.y() * first.z(),
								   first.z() * second.z() );

		return Normalize( retVal );
	}
};

BEGIN_CLASS_RTTI(CMaterialBlockNormalmapBlend)
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_firstMapWeight, TXT("Weight of first normalmap") );
	PROPERTY_EDIT( m_secondMapWeight, TXT("Weight of second normalmap") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockNormalmapBlend );

#endif