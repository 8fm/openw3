/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbDerivativeNormal.h"
#include "../engine/graphBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockDerivativeNormal );

CMaterialBlockDerivativeNormal::CMaterialBlockDerivativeNormal()
	: m_derivativeToNormal( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockDerivativeNormal::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
}

#endif

String CMaterialBlockDerivativeNormal::GetCaption() const
{
	if ( m_derivativeToNormal == true )
	{
		
		return TXT("Derivative to Normal");
	}

	else
	{
		return TXT("Normal to Derivative");
	}
}

CodeChunk CMaterialBlockDerivativeNormal::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	CodeChunk a = CompileInput( compiler, CNAME( In ), shaderTarget, CodeChunk( Vector( 0.0f, 0.0f, 1.0f, 0.0f ) ) );

	if ( m_derivativeToNormal == true )
	{
		CodeChunk invZSquare = SHADER_VAR_FLOAT( (a.x() * a.x()) + (a.y() * a.y()) + CodeChunk( 1.0f ), shaderTarget );
		CodeChunk z = SHADER_VAR_FLOAT( CodeChunk( 1.0f ) / CodeChunkOperators::Sqrt( invZSquare ), shaderTarget );
		CodeChunk x = a.x() * z;
		CodeChunk y = a.y() * z;
		return SHADER_VAR_FLOAT4( CodeChunkOperators::Float4( x, y, z, CodeChunk(0.0f) ), shaderTarget );
	}

	else
	{
		return SHADER_VAR_FLOAT4( CodeChunkOperators::Float4( a.x() / a.z(), a.y() / a.z(), CodeChunk(0.0f), CodeChunk(1.0f) ), shaderTarget );
	}
}

#endif