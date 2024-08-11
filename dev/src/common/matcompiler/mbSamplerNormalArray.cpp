/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbSamplerNormalArray.h"
#include "../engine/graphBlock.h"
#include "../engine/materialBlock.h"
#include "../engine/materialInputTextureSocket.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

CodeChunk CompileNormalmapSample( CMaterialBlockCompiler& compiler, const CodeChunk &normalmapSample, const Vector &scale, bool outputInTangentSpace );
CodeChunk CompileNormalmapSampleLerp( CMaterialBlockCompiler& compiler, const CodeChunk &normalmapSample0, const CodeChunk &normalmapSample1, const CodeChunk &lerpFactor, const Vector &scale, bool outputInTangentSpace );

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSamplerNormalArray );

CMaterialBlockSamplerNormalArray::CMaterialBlockSamplerNormalArray()
	: m_subUVWidth ( 0 )
	, m_subUVHeight ( 0 )
	, m_subUVInterpolate ( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockSamplerNormalArray::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( Texture ) ) );

	// Inputs
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UV ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( LodBias ), false ) ); 
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( ArrayIndex ) ) );

	// Keyframed texture
	if ( m_subUVWidth >= 1 && m_subUVHeight >= 1 && ( m_subUVHeight * m_subUVWidth > 1 ) )
	{
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Frame ), true ) ); 
	}

	// Outputs
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Color ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( Red ), Color::RED ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("yyyy"), CNAME( Green ), Color::GREEN ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("zzzz"), CNAME( Blue ), Color::BLUE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( Alpha ), Color( 127, 127, 127 ) ) );
}

#endif

CodeChunk CMaterialBlockSamplerNormalArray::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	CodeChunk arrayIndex = CompileInput( compiler, CNAME( ArrayIndex ), shaderTarget, 0.0f );

	// Get texture socket
	CMaterialInputTextureSocket* textureSocket = Cast< CMaterialInputTextureSocket >( FindSocket( CNAME( Texture ) ) );

	// Get sampler data
	if ( textureSocket && textureSocket->HasConnections() ) 
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
			uv = PS_DATA( "TexCoords0" ); //float2
		}

		// Get sample name
		CodeChunk samplerName = textureSocket->Compile( compiler, shaderTarget );

		// Sub UV ?
		const Bool isSubUV = (m_subUVWidth >= 1 && m_subUVHeight >= 1 && ( m_subUVHeight * m_subUVWidth > 1 ));
		if ( isSubUV )
		{
			// Get the base frame
			CodeChunk frame = PS_VAR_FLOAT( CompileInput( compiler, CNAME( Frame ), shaderTarget, 0.0f ).x() );

			// Calculate the UV
			CodeChunk call1 = CodeChunk::Printf( false, "CalcSubUV( (%s), (%s), %1.0f, %1.0f)", uv.xy().AsChar(), frame.AsChar(), (Float)m_subUVWidth, (Float)m_subUVHeight );
			CodeChunk frame1UV = PS_VAR_FLOAT2( call1 );

			// Sample first texture
			CodeChunk tex1 = PS_VAR_FLOAT4( SampleTexture( compiler, samplerName, frame1UV, arrayIndex, true ) );

			// Interpolate or use single texture read
			if ( m_subUVInterpolate )
			{
				// Second UV
				CodeChunk call2 = CodeChunk::Printf( false, "CalcSubUV( (%s), (%s) + 1.0f, %1.0f, %1.0f)", uv.xy().AsChar(), frame.AsChar(), (Float)m_subUVWidth, (Float)m_subUVHeight );
				CodeChunk frame2UV = PS_VAR_FLOAT2( call2 );

				// Sample texture and interpolate
				CodeChunk tex2 = PS_VAR_FLOAT4( SampleTexture( compiler, samplerName, frame2UV, arrayIndex, true ) );
				return CompileNormalmapSampleLerp( compiler, tex1, tex2, Frac( frame ), Vector::ONES, false );
			}
			else
			{
				// Use single texture
				return CompileNormalmapSample( compiler, tex1, Vector::ONES, false );
			}
		}
		else
		{
			CodeChunk tex = SampleTexture( compiler, samplerName, uv, arrayIndex, true );
			return CompileNormalmapSample( compiler, tex, Vector::ONES, false );
		}
	}
	else
	{
		// No texture, use world space normal
		CodeChunk pos = PS_DATA( "WorldNormal" );
		return PS_VAR_FLOAT4( Float4( Normalize( pos.xyz() ), 0.0f ) );
	}
}

CodeChunk CMaterialBlockSamplerNormalArray::SampleTexture( CMaterialBlockCompiler& compiler, const CodeChunk &sampler, const CodeChunk &uv, const CodeChunk& index, bool allowBias ) const
{
	return PS_VAR_FLOAT4( Tex2DArray( sampler, uv, index.x() ) );
}

#endif