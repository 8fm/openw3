/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbSamplerTexture.h"
#include "../engine/graphBlock.h"
#include "../engine/materialInputSocket.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialInputTextureSocket.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSamplerTexture );

CMaterialBlockSamplerTexture::CMaterialBlockSamplerTexture()
	: m_projected( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockSamplerTexture::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( Texture ) ) );

	// Inputs
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UV ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( LodBias ) ) ); 
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UVRotation ) ) );

	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( SubUVWidth ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( SubUVHeight ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Frame ) ) ); 

	// Outputs
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Color ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( Red ), Color::RED ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("yyyy"), CNAME( Green ), Color::GREEN ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("zzzz"), CNAME( Blue ), Color::BLUE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( Alpha ), Color( 127, 127, 127 ) ) );
}

#endif

CodeChunk CMaterialBlockSamplerTexture::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
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
				uv = SHADER_DATA( "TexCoords2", shaderTarget ); // float4
			}
			else
			{
				// Get uv set 0 coordinates
				uv = SHADER_DATA( "TexCoords0", shaderTarget ); //float2
			}
		}
		
		// fake detail map rotation
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

		Bool hasSubUVWidthInput  = HasInput( CNAME( SubUVWidth ) );
		Bool hasSubUVHeightInput = HasInput( CNAME( SubUVHeight ) );

		CodeChunk subUVWidth = hasSubUVWidthInput
			? SHADER_VAR_FLOAT( CompileInput( compiler, CNAME( SubUVWidth ), shaderTarget ).x(), shaderTarget )
			: CodeChunk( m_subUVWidth );

		CodeChunk subUVHeight = hasSubUVHeightInput
			? SHADER_VAR_FLOAT( CompileInput( compiler, CNAME( SubUVHeight ), shaderTarget ).x(), shaderTarget )
			: CodeChunk( m_subUVHeight );

		// Sub UV ?
		const Bool isSubUV = 
			( m_subUVWidth >= 1 && m_subUVHeight >= 1 && ( m_subUVHeight * m_subUVWidth > 1 ) )
			||
			( hasSubUVWidthInput || hasSubUVHeightInput );

		if ( isSubUV )
		{
			// Get the base frame
			CodeChunk frame = SHADER_VAR_FLOAT( CompileInput( compiler, CNAME( Frame ), shaderTarget, 0.0f ).x(), shaderTarget );

			// Calculate the UV
			CodeChunk call1 = CodeChunk::Printf( false, "CalcSubUV( (%s), (%s), (%s), (%s))", uv.xy().AsChar(), frame.AsChar(), subUVWidth.AsChar(), subUVHeight.AsChar() );
			CodeChunk frame1UV = SHADER_VAR_FLOAT2( call1, shaderTarget );

			// Sample first texture
			CodeChunk tex1 = SHADER_VAR_FLOAT4( SampleTexture( compiler, textureName, samplerName, frame1UV, true, shaderTarget ), shaderTarget );

			// Interpolate or use single texture read
			if ( m_subUVInterpolate )
			{
				// Second UV
				CodeChunk call2 = CodeChunk::Printf( false, "CalcSubUV( (%s), (%s) + 1.0f, (%s), (%s))", uv.xy().AsChar(), frame.AsChar(), subUVWidth.AsChar(), subUVHeight.AsChar() );
				CodeChunk frame2UV = SHADER_VAR_FLOAT2( call2, shaderTarget );

				// Sample texture and interpolate
				CodeChunk tex2 = SHADER_VAR_FLOAT4( SampleTexture( compiler, textureName, samplerName, frame2UV, true, shaderTarget ), shaderTarget );
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
			return SHADER_VAR_FLOAT4( Tex2Dproj( textureName, samplerName, uv ), shaderTarget );
		}
		else
		{
			return SampleTexture( compiler, textureName, samplerName, uv, true, shaderTarget );			
		}
	}
	else
	{
		// No texture
		return Vector( 0.5f, 0.5f, 0.5f, 1.0f );
	}
}

CodeChunk CMaterialBlockSamplerTexture::SampleTexture( CMaterialBlockCompiler& compiler, const CodeChunk &texture, const CodeChunk &sampler, const CodeChunk &uv, bool allowBias, EMaterialShaderTarget shaderTarget ) const
{
	if ( HasInput( CNAME( LodBias ) ) )
	{
		CodeChunk lodBias = SHADER_VAR_FLOAT( CompileInput( compiler, CNAME( LodBias ), shaderTarget, 0.0f ).x(), shaderTarget );
		return SHADER_VAR_FLOAT4( Tex2Dbias( texture, sampler, uv.xy(), lodBias ), shaderTarget );
	}
	else
	{
		return SHADER_VAR_FLOAT4( Tex2D( texture, sampler, uv ), shaderTarget );
	}
}

#endif
