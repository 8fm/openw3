/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbSamplerNormal.h"
#include "../engine/materialInputTextureSocket.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSamplerNormal );

CMaterialBlockSamplerNormal::CMaterialBlockSamplerNormal()
	: m_scale( 1,1,1,1 )
	, m_sampleTangentSpace( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockSamplerNormal::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( Texture ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UV ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( LodBias ) ) ); 
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Normal ) ) );
}

#endif

CodeChunk DecodeNormalmapSample( CMaterialBlockCompiler& compiler, const CodeChunk &normalmapSample )
{
	CodeChunk normal = PS_VAR_FLOAT4( 2.0f * normalmapSample - 1.0f);

	// swizzle channels from DXT compression
	CodeChunk X = PS_VAR_FLOAT(normal.x()*normal.w());
	CodeChunk Y = PS_VAR_FLOAT(normal.y());
	CodeChunk Z = PS_VAR_FLOAT(Pow(CodeChunkOperators::Max(0.f, 1.0f - X*X - Y*Y), 0.5f));

	CodeChunk swizzled = Float3(X,Y,Z);

	return PS_VAR_FLOAT4( Float4( swizzled, 0.0f ) );
}

CodeChunk ProcessDecodedNormalmapSample( CMaterialBlockCompiler& compiler, const CodeChunk &decodedSample, const Vector &scale, bool outputInTangentSpace )
{
	Vector scaleDecodedSample(scale.X, scale.Y, scale.Z);
	CodeChunk local = PS_VAR_FLOAT3( decodedSample.xyz() * scaleDecodedSample );

	// Renormalize if needed
	if ( (scaleDecodedSample.X != 1.0f && scaleDecodedSample.X != -1.0f) || 
		 (scaleDecodedSample.Y != 1.0f && scaleDecodedSample.Y != -1.0f) || 
		 (scaleDecodedSample.Z != 1.0f && scaleDecodedSample.Z != -1.0f) )
	{
		local = PS_VAR_FLOAT3( Normalize( local ) );
	}

	if ( !outputInTangentSpace )
	{
		// Get tangent space basis in world space
		CodeChunk basisX = PS_DATA( "WorldTangent" );
		CodeChunk basisY = PS_DATA( "WorldBinormal" );
		CodeChunk basisZ = PS_DATA( "WorldNormal" );

		// Convert to world space
		CodeChunk worldNormalX = PS_VAR_FLOAT3( basisX * local.x() );
		CodeChunk worldNormalY = PS_VAR_FLOAT3( basisY * local.y() );
		CodeChunk worldNormalZ = PS_VAR_FLOAT3( basisZ * local.z() );
		CodeChunk worldNormal = worldNormalX + worldNormalY + worldNormalZ;
		return PS_VAR_FLOAT4( Float4( worldNormal, 0.0f ) );
	}
	else
	{
		return PS_VAR_FLOAT4( Float4( local, 0.0f ) );
	}
}

CodeChunk CompileNormalmapSample( CMaterialBlockCompiler& compiler, const CodeChunk &normalmapSample, const Vector &scale, bool outputInTangentSpace )
{	
	CodeChunk decodedNormal = DecodeNormalmapSample( compiler, normalmapSample );
	return ProcessDecodedNormalmapSample( compiler, decodedNormal, scale, outputInTangentSpace );
}

CodeChunk CompileNormalmapSampleLerp( CMaterialBlockCompiler& compiler, const CodeChunk &normalmapSample0, const CodeChunk &normalmapSample1, const CodeChunk &lerpFactor, const Vector &scale, bool outputInTangentSpace )
{
	CodeChunk n0 = DecodeNormalmapSample( compiler, normalmapSample0 );
	CodeChunk n1 = DecodeNormalmapSample( compiler, normalmapSample1 );
	CodeChunk n = Lerp( n0.xyz(), n1.xyz(), lerpFactor );
	return CompileNormalmapSample( compiler, n, scale, outputInTangentSpace );
}

CodeChunk CMaterialBlockSamplerNormal::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
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
			// Get uv set 0 coordinates
			uv = PS_DATA( "TexCoords0" );
		}

		// Sample normal in tangent space
		CodeChunk textureName = textureSocket->Compile( compiler, shaderTarget );

		CodeChunk samplerName = BindSamplerState( compiler, shaderTarget );

        CodeChunk unbiased;
		if ( HasInput( CNAME( LodBias )) )
		{
			CodeChunk lodBias = PS_VAR_FLOAT( CompileInput( compiler, CNAME( LodBias ), shaderTarget, 0.0f ).x() );
			unbiased = PS_VAR_FLOAT4( Tex2Dbias( textureName, samplerName, uv.xy(), lodBias ) );
		}
		else
		{
			unbiased = PS_VAR_FLOAT4( Tex2D( textureName, samplerName, uv ) );
		}

		// Process normal
		return CompileNormalmapSample( compiler, unbiased, m_scale, m_sampleTangentSpace );
	}
	else
	{
		if ( !m_sampleTangentSpace )
		{
			// No texture, use world space normal
			CodeChunk pos = PS_DATA( "WorldNormal" );
			return PS_VAR_FLOAT4( Float4( Normalize( pos.xyz() ), 0.0f ) );
		}
		else
		{
			return PS_VAR_FLOAT4( Float4( 0.0f, 0.0f, 1.0f, 0.0f ) );
		}
	}
}

#endif