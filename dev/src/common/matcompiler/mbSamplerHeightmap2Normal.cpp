/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbSamplerHeightmap2Normal.h"
#include "../engine/materialInputTextureSocket.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSamplerHeightmap2Normal );

CMaterialBlockSamplerHeightmap2Normal::CMaterialBlockSamplerHeightmap2Normal()
	: m_scale( 1,1,1,1 )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockSamplerHeightmap2Normal::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( Texture ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( ControlVector ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UV ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UVMultiplier ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Texel_size ) ) );
	//CreateSocket( MaterialInputSocketSpawnInfo( CNAME( LodBias ) ) ); 
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Normal ) ) );
}

#endif

CodeChunk CMaterialBlockSamplerHeightmap2Normal::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Get texture socket
	CMaterialInputTextureSocket* hmSocket = Cast< CMaterialInputTextureSocket >( FindSocket( CNAME( Texture ) ) );

	// Get sampler data
	if ( hmSocket && hmSocket->HasConnections() ) 
	{
		// Sample UV
		CodeChunk uv;
		if ( HasInput( CNAME( UV ) ) )
		{
			// Use given coordinates
			uv = CompileInput( compiler, CNAME( UV ), shaderTarget );
		}
		else
		{
			// Get uv set 0 coordinates
			uv = PS_DATA( "TexCoords0" );
		}

		// multiply uv coordinates (mostly for detail maps)
		if ( HasInput( CNAME( UVMultiplier ) ) )
		{
			CodeChunk uvMult = CompileInput( compiler, CNAME( UVMultiplier ), shaderTarget );
			uv = PS_VAR_FLOAT2( uv * uvMult );  
		}
		
		// Sample normal in tangent space		
		CodeChunk hmTextureName = hmSocket->Compile( compiler, shaderTarget );
		CodeChunk unbiased;

		CodeChunk samplerName = BindSamplerState( compiler, shaderTarget );

		CodeChunk texelSize = CompileInput( compiler, CNAME( Texel_size ), shaderTarget, 1.0f/256.0f ).xy();
		
		// compute normal using texel neighbourhood
		CodeChunk leftHeights = PS_VAR_FLOAT3( Tex2D( hmTextureName, samplerName, Float2( uv.x() - texelSize.x(), uv.y() ) ) );
		CodeChunk rightHeights = PS_VAR_FLOAT3( Tex2D( hmTextureName, samplerName, Float2( uv.x() + texelSize.x(), uv.y() ) ) );
		CodeChunk upHeights = PS_VAR_FLOAT3( Tex2D( hmTextureName, samplerName, Float2( uv.x(), uv.y() + texelSize.y() ) ) );
		CodeChunk downHeights = PS_VAR_FLOAT3( Tex2D( hmTextureName, samplerName, Float2( uv.x(), uv.y() - texelSize.y() ) ) );

		CodeChunk scaledLeft;
		CodeChunk scaledRight;
		CodeChunk scaledUp;
		CodeChunk scaledDown;

		CodeChunk controlVector = CompileInput( compiler, CNAME( ControlVector ), shaderTarget, Vector( 1.0f, 0.0f, 0.0f, 0.0f ) ).xyz();

		scaledLeft = leftHeights * controlVector;
		scaledRight = rightHeights * controlVector;
		scaledUp = upHeights * controlVector;
		scaledDown = downHeights * controlVector;

		CodeChunk finalLeft = PS_VAR_FLOAT( scaledLeft.x() + scaledLeft.y() + scaledLeft.z() );
		CodeChunk finalRight = PS_VAR_FLOAT( scaledRight.x() + scaledRight.y() + scaledRight.z() );
		CodeChunk finalUp = PS_VAR_FLOAT( scaledUp.x() + scaledUp.y() + scaledUp.z() );
		CodeChunk finalDown = PS_VAR_FLOAT( scaledDown.x() + scaledDown.y() + scaledDown.z() );

		CodeChunk X = PS_VAR_FLOAT( finalLeft - finalRight );
		CodeChunk Y = PS_VAR_FLOAT( finalDown - finalUp );
		CodeChunk Z = PS_VAR_FLOAT( 0.5f );
		CodeChunk normal = PS_VAR_FLOAT3( Float3( X, Y, Z ) );

		Vector scaleInvG(m_scale.X, -m_scale.Y, m_scale.Z); // invert green channel
		normal = PS_VAR_FLOAT3( normal.xyz() * scaleInvG );
		normal = PS_VAR_FLOAT3( Normalize( normal ) );

		return PS_VAR_FLOAT4( Float4( normal, 0.0f ) );
	}
	else
	{
		return PS_VAR_FLOAT4( Float4( 0.0f, 0.0f, 1.0f, 0.0f ) );
	}
}

#endif