/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbSamplerCube.h"
#include "../engine/materialInputCubeSocket.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSamplerCube );

CMaterialBlockSamplerCube::CMaterialBlockSamplerCube()
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockSamplerCube::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputCubeSocketSpawnInfo( CNAME( Cube ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( LodBias ), false ) ); 
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UVW ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Color ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( Red ), Color::RED ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("yyyy"), CNAME( Green ), Color::GREEN ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("zzzz"), CNAME( Blue ), Color::BLUE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( Alpha ), Color( 127, 127, 127 ) ) );
}

#endif

CodeChunk CMaterialBlockSamplerCube::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Get texture socket
	CMaterialInputCubeSocket* cubeSocket = Cast< CMaterialInputCubeSocket >( FindSocket( CNAME( Cube ) ) );

	// Get sampler data
	if ( cubeSocket && cubeSocket->HasConnections() && HasInput( CNAME( UVW ) ) )
	{
		// Sample UV
		CodeChunk uvw = CompileInput( compiler, CNAME( UVW ), shaderTarget );

		// Get texture name
		CodeChunk textureName = cubeSocket->Compile( compiler, shaderTarget );

		CodeChunk samplerName = BindSamplerState( compiler, shaderTarget );

		if ( HasInput( CNAME( LodBias ) ) )
		{
			CodeChunk lodBias = PS_VAR_FLOAT( CompileInput( compiler, CNAME( LodBias ), shaderTarget, 0.0f ).x() );
			return PS_VAR_FLOAT4( TexCUBEbias( textureName, samplerName, uvw, lodBias ) );
		}
		else
		{
			return PS_VAR_FLOAT4( TexCUBE( textureName, samplerName, uvw ) );
		}

	}
	else
	{
		// No texture
		return Vector( 0.5f, 0.5f, 0.5f, 1.0f );
	}
}

#endif