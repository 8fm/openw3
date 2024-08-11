/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbSamplerDetail.h"
#include "../engine/graphBlock.h"
#include "../engine/materialInputTextureSocket.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSamplerDetail );

CMaterialBlockSamplerDetail::CMaterialBlockSamplerDetail()
	: m_scale( 1,1,1,1 )
	, m_BlendStartDistance( 0.0f )
	, m_BlendEndDistance( 3.6f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockSamplerDetail::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( DetailTexture ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailUV ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailUVRotation ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailPower ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailDensity ) ) );

	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( EndDist ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( StartDist ) ) );

	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Output ) ) );
}

#endif

CodeChunk CMaterialBlockSamplerDetail::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Get texture sockets
	CMaterialInputTextureSocket* dnmSocket = Cast< CMaterialInputTextureSocket >( FindSocket( CNAME( DetailTexture ) ) );

	CodeChunk detPow, detDens;

	CodeChunk detailSample;

	CodeChunk output;

	// if detail map is connected - combine it with the first one
	if ( dnmSocket->HasConnections() )
	{
		// Get socket data
		if ( HasInput( CNAME( DetailPower ) ) )
		{
			detPow = CompileInput( compiler, CNAME( DetailPower ), shaderTarget );	
		}
		else
		{
			detPow = SHADER_VAR_FLOAT( 1.0f, shaderTarget );
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
			detDens = CompileInput( compiler, CNAME( DetailDensity ), shaderTarget );
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

		CodeChunk samplerName = BindSamplerState( compiler, shaderTarget );

		detailSample = SHADER_VAR_FLOAT4( Tex2D( detailTextureName, samplerName, detailUV ), shaderTarget );

		CodeChunk clampedDetailSample = SHADER_VAR_FLOAT( Clamp( detailSample.z(), 1.0f - detPow, 1.0f ), shaderTarget );
		CodeChunk multiplyColor = SHADER_VAR_FLOAT( Lerp( clampedDetailSample, 1.0f, distanceBlend ), shaderTarget );

		output = SHADER_VAR_FLOAT4( multiplyColor, shaderTarget );
	}
	else output = SHADER_VAR_FLOAT( 1.0f, shaderTarget );

	return output;
}

#endif