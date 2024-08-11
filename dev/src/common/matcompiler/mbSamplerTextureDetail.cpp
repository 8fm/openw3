/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbSamplerTextureDetail.h"
#include "../engine/graphSocket.h"
#include "../engine/materialInputTextureSocket.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSamplerTextureDetail );

CMaterialBlockSamplerTextureDetail::CMaterialBlockSamplerTextureDetail()
: m_scale( 1,1,1,1 )
, m_BlendStartDistance( 0.0f )
, m_BlendEndDistance( 3.6f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockSamplerTextureDetail::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( Texture ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UV ) ) );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( DetailTexture ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailUV ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailUVRotation ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailPower ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailAlphaPower ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailDensity ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DiffuseColor ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DiffuseBrightness ) ) );

	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( EndDist ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( StartDist ) ) );

	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Output ),	Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( Red ),		Color::RED ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("yyyy"), CNAME( Green ),	Color::GREEN ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("zzzz"), CNAME( Blue ),	Color::BLUE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( Alpha ),	Color( 127, 127, 127 ) ) );
}

#endif

CodeChunk CMaterialBlockSamplerTextureDetail::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Get texture sockets
	CMaterialInputTextureSocket* nmSocket = Cast< CMaterialInputTextureSocket >( FindSocket( CNAME( Texture ) ) );
	CMaterialInputTextureSocket* dnmSocket = Cast< CMaterialInputTextureSocket >( FindSocket( CNAME( DetailTexture ) ) );

	if ( nmSocket && nmSocket->HasConnections() ) 
	{
		CodeChunk uv;
		// Sample UV
		if ( HasInput( CNAME( UV ) ) )
		{
			// Use given coordinates
			uv = CompileInput( compiler, CNAME( UV ), shaderTarget );
		}
		else
		{
			// Get uv set 0 coordinates
			uv = SHADER_DATA( "TexCoords0", shaderTarget );
		}

		CodeChunk diffuseBrightness;
		if ( HasInput(CNAME( DiffuseBrightness ) ) )
		{
			diffuseBrightness = CompileInput( compiler, CNAME( DiffuseBrightness ), shaderTarget );
		}
		else
		{
			diffuseBrightness = Float( 1.0f );
		}

		CodeChunk diffuseColor;
		if ( HasInput(CNAME( DiffuseColor ) ) )
		{
			diffuseColor = CompileInput( compiler, CNAME( DiffuseColor ), shaderTarget );
		}
		else
		{
			diffuseColor = Float4( 1.0f, 1.0f, 1.0f, 1.0f );
		}

		CodeChunk diffuseFinalBrightness = Float4( diffuseColor * diffuseBrightness );

		// Sample
		CodeChunk textureName = nmSocket->Compile( compiler, shaderTarget );

		CodeChunk samplerName = BindSamplerState( compiler, shaderTarget );

		CodeChunk sample = SHADER_VAR_FLOAT4( Tex2D( textureName, samplerName, uv ) * diffuseFinalBrightness, shaderTarget );

		// if detail map is connected - combine it with the first one
		if ( dnmSocket->HasConnections() )
		{
			CodeChunk detPow;
			// Get socket data
			if ( HasInput( CNAME( DetailPower ) ) )
			{
				detPow = CompileInput( compiler, CNAME( DetailPower ), shaderTarget );	
			}
			else
			{
				detPow = SHADER_VAR_FLOAT( 1.0f, shaderTarget );
			}

			// if DetailAlphaPower is attached, use it for controlling the alpha value, otherwise just use the DetailPower input
			CodeChunk detAlphaPow;
			if ( HasInput( CNAME( DetailAlphaPower ) ) )
			{
				detAlphaPow = CompileInput( compiler, CNAME( DetailAlphaPower ), shaderTarget );	
			}
			else
			{
				detAlphaPow = detPow;
			}

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

			CodeChunk distanceBlend;
			if ( startDist != endDist )
			{
				CodeChunk distance = SHADER_DATA( "CameraDistance", shaderTarget );
				CodeChunk frac = SHADER_VAR_FLOAT( Saturate( ( distance - startDist )  / ( endDist - startDist ) ), shaderTarget );
				distanceBlend = frac;
			}
			CodeChunk detailUV;
			if ( HasInput( CNAME( DetailUV ) ) )
			{
				// Use given coordinates
				detailUV = CompileInput( compiler, CNAME( DetailUV ), shaderTarget );
			}
			else
			{
				// Get uv set 0 coordinates
				detailUV = SHADER_DATA( "TexCoords0", shaderTarget );
			}

			if ( HasInput( CNAME( DetailDensity ) ) )
			{
				CodeChunk detDens = CompileInput( compiler, CNAME( DetailDensity ), shaderTarget );
				detailUV = SHADER_VAR_FLOAT2( detailUV * detDens, shaderTarget );
			}
			// fake detail map rotation
			if ( HasInput( CNAME( DetailUVRotation ) ) )
			{
				// in the close distance from 0, we can approximate sin(x) function with x
				CodeChunk Ux = CompileInput( compiler, CNAME( DetailUVRotation ), shaderTarget ).x();
				CodeChunk Uy = SHADER_VAR_FLOAT( 1.0f - CodeChunkOperators::Abs(Ux), shaderTarget );

				// apply simple rotation matrix with two dot operations
				// [  cos(x) sin(x) ] = [  Uy  Ux ]
				// [ -sin(x) cos(x) ]   [ -Ux  Uy ]
				CodeChunk Uvec = SHADER_VAR_FLOAT2( Float2( Uy, Ux ), shaderTarget );
				CodeChunk Vvec = SHADER_VAR_FLOAT2( Float2( -Ux, Uy ), shaderTarget );
				detailUV = SHADER_VAR_FLOAT2( Float2( Dot2(Uvec, detailUV.xy()), Dot2(Vvec, detailUV.xy() ) ), shaderTarget );
			}

			// sample detail
			CodeChunk detailTextureName = dnmSocket->Compile( compiler, shaderTarget );

			CodeChunk detailSample = SHADER_VAR_FLOAT4( Tex2D( detailTextureName, samplerName, detailUV ), shaderTarget );
			
			CodeChunk clampedDetailSample = SHADER_VAR_FLOAT( Clamp( detailSample.z(), 1.0f - detPow, 1.0f ), shaderTarget );
			CodeChunk clampedDetailSampleAlpha = SHADER_VAR_FLOAT( Clamp( detailSample.z(), 1.0f - detAlphaPow, 1.0f ), shaderTarget );

			// control RGB and Alpha separately
			CodeChunk multiplyColor = SHADER_VAR_FLOAT( Lerp( clampedDetailSample, 1.0f, distanceBlend ), shaderTarget );
			CodeChunk multiplyAlpha = SHADER_VAR_FLOAT( Lerp( clampedDetailSampleAlpha, 1.0f, distanceBlend ), shaderTarget );
			
			return SHADER_VAR_FLOAT4( Float4( sample.xyz() * multiplyColor, sample.w() * multiplyAlpha ), shaderTarget );
		}
		
		return sample;
	}
	else
	{
		return SHADER_VAR_FLOAT4( Float4( 1.0f, 0.0f, 0.0f, 0.0f ), shaderTarget );
	}
}

#endif