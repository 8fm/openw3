/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbPackNormal.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockPackNormal );

CMaterialBlockPackNormal::CMaterialBlockPackNormal()
	: m_unpack( true )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockPackNormal::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );

}

#endif

String CMaterialBlockPackNormal::GetCaption() const
{
	if ( m_unpack == true )
	{
		return TXT("Unpack Normal");
	}

	else
	{
		return TXT("Pack Normal");
	}
}

CodeChunk CMaterialBlockPackNormal::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	CodeChunk in = CompileInput( compiler, CNAME( In ), shaderTarget, 0.0f );

	if ( m_unpack == true )
	{
		
		return SHADER_VAR_FLOAT4(  2.0f * ( in - 0.5f ) , shaderTarget );
	}

	else
	{
		return SHADER_VAR_FLOAT4(  in * 0.5f + 0.5f , shaderTarget );
	}
}

#endif