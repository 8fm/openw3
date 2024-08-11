/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbSamplerTextureArray.h"
#include "../engine/materialInputTextureSocket.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSamplerTextureArray );

CMaterialBlockSamplerTextureArray::CMaterialBlockSamplerTextureArray()
	: m_subUVWidth ( 0 )
	, m_subUVHeight ( 0 )
	, m_subUVInterpolate ( false )
	, m_projected( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockSamplerTextureArray::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( Texture ) ) );

	// Inputs
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UV ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( LodBias ), false ) ); 
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( ArrayIndex ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UVRotation ) ) );

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

CodeChunk CMaterialBlockSamplerTextureArray::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
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
			// Use stream that's appropriate for sampling method (4 component stream for tex2Dproj sampler)
			if ( m_projected )
			{
				// Get uv set 2 coordinates
				uv = PS_DATA( "TexCoords2" ); // float4
			}
			else
			{
				// Get uv set 0 coordinates
				uv = PS_DATA( "TexCoords0" ); //float2
			}
		}

		// uv map rotation
		if ( HasInput( CNAME( UVRotation ) ) )
		{
			CodeChunk Uy = CompileInput( compiler, CNAME( UVRotation ), shaderTarget ).x();
			CodeChunk Ux = SHADER_VAR_FLOAT( 1.0f - CodeChunkOperators::Abs(Uy), shaderTarget );
			CodeChunk Uvec = SHADER_VAR_FLOAT2( Float2( Ux,Uy ), shaderTarget );
			CodeChunk Vvec = SHADER_VAR_FLOAT2( Float2( -Uvec.y(), Uvec.x() ), shaderTarget );
			uv = SHADER_VAR_FLOAT2( Float2( Dot2(Uvec, uv.xy()), Dot2(Vvec, uv.xy() ) ), shaderTarget );
		}

		// Get sample name
		CodeChunk textureName = textureSocket->Compile( compiler, shaderTarget );
		
		CodeChunk samplerName = BindSamplerState( compiler, shaderTarget );

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
			CodeChunk tex1 = PS_VAR_FLOAT4( SampleTexture( compiler, textureName, samplerName, frame1UV, arrayIndex, true ) );

			// Interpolate or use single texture read
			if ( m_subUVInterpolate )
			{
				// Second UV
				CodeChunk call2 = CodeChunk::Printf( false, "CalcSubUV( (%s), (%s) + 1.0f, %1.0f, %1.0f)", uv.xy().AsChar(), frame.AsChar(), (Float)m_subUVWidth, (Float)m_subUVHeight );
				CodeChunk frame2UV = PS_VAR_FLOAT2( call2 );

				// Sample texture and interpolate
				CodeChunk tex2 = PS_VAR_FLOAT4( SampleTexture( compiler, textureName, samplerName, frame2UV, arrayIndex, true ) );
				return Lerp( tex1, tex2, Frac( frame ) );
			}
			else
			{
				// Use single texture
				return tex1;
			}
		}
		else if ( m_projected )
		{
			return PS_VAR_FLOAT4( Tex2Dproj( samplerName, uv ) );
		}
		else
		{
			return SampleTexture( compiler, textureName, samplerName, uv, arrayIndex, true );			
		}
	}
	else
	{
		// No texture
		return Vector( 0.5f, 0.5f, 0.5f, 1.0f );
	}
}

CodeChunk CMaterialBlockSamplerTextureArray::SampleTexture( CMaterialBlockCompiler& compiler, const CodeChunk &texture, const CodeChunk &sampler, const CodeChunk &uv, const CodeChunk& index, bool allowBias ) const
{
	return PS_VAR_FLOAT4( Tex2DArray( texture, sampler, uv, index.x() ) );
}

#endif