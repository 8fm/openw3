/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbTexCoords.h"
#include "../engine/graphBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockTexCoords );

CMaterialBlockTexCoords::CMaterialBlockTexCoords()
	: m_coordinatesIndex( 0 )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockTexCoords::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	// Outputs
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( UV ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( X ), Color( 31, 31, 31 ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("yyyy"), CNAME( Y ), Color( 63, 63, 63 ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("zzzz"), CNAME( Z ), Color( 127, 127, 127 ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( W ), Color( 255, 255, 255 ) ) );
}

#endif

CodeChunk CMaterialBlockTexCoords::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	if ( m_coordinatesIndex == 0 )
	{
		CodeChunk uv = SHADER_DATA( "TexCoords0", shaderTarget );
		return Float4( uv.xy(), 0.0f, 0.0f );
	}
	else if ( m_coordinatesIndex == 1 )
	{
		CodeChunk uv = SHADER_DATA( "TexCoords1", shaderTarget );
		return Float4( uv.xy(), 0.0f, 0.0f );
	}
	else if ( m_coordinatesIndex == 2 )
	{
		CodeChunk uv = SHADER_DATA( "TexCoords2", shaderTarget );
		return uv;
	}
	else
	{
		return Float4( 0.0f, 0.0f, 0.0f, 0.0f );
	}
}

#endif