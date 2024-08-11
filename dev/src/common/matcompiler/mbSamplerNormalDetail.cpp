/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbSamplerNormalDetail.h"
#include "../engine/materialInputTextureSocket.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSamplerNormalDetail );

CodeChunk DecodeNormalmapSample( CMaterialBlockCompiler& compiler, const CodeChunk &normalmapSample );

CMaterialBlockSamplerNormalDetail::CMaterialBlockSamplerNormalDetail()
	: m_scale( 1,1,1,1 )
	, m_sampleTangentSpace( false )
	, m_BlendStartDistance( 0.0f )
	, m_BlendEndDistance( 3.6f )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockSamplerNormalDetail::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( Texture ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( UV ) ) );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( DetailTexture ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailUV ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailUVRotation ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailPower ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailDensity ) ) );

	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( EndDist ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( StartDist ) ) );

	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Normal ) ) );
}

#endif

CodeChunk CMaterialBlockSamplerNormalDetail::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	// Get texture sockets
	CMaterialInputTextureSocket* nmSocket = Cast< CMaterialInputTextureSocket >( FindSocket( CNAME( Texture ) ) );
	CMaterialInputTextureSocket* dnmSocket = Cast< CMaterialInputTextureSocket >( FindSocket( CNAME( DetailTexture ) ) );

	if ( nmSocket && nmSocket->HasConnections() ) 
	{
		CodeChunk uv;
		CodeChunk detPow, detDens;
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

		// Sample normal in tangent space
		CodeChunk textureName = nmSocket->Compile( compiler, shaderTarget );

		CodeChunk samplerName = BindSamplerState( compiler, shaderTarget );
		
		// decode base normalmap
		CodeChunk swizzled = DecodeNormalmapSample( compiler, Tex2D( textureName, samplerName, uv ) ).xyz();

		CodeChunk detX,detY,detZ;
		CodeChunk normal;

		// if detail normal map is connected - combine it with the first one
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
				distanceBlend = SHADER_VAR_FLOAT( 1.0f - frac, shaderTarget );
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

			// get detail sampelr name
			CodeChunk detailSamplerName = dnmSocket->Compile( compiler, shaderTarget );

			// sample detail normal
			CodeChunk detailSwizzled;
			if ( HasInput( CNAME( DetailUVRotation ) ) )
			{
				// detail map COORD rotation
				CodeChunk rotAxisX;
				CodeChunk rotAxisY;
				{
					// in the close distance from 0, we can approximate sin(x) function with x
					CodeChunk Ux = CompileInput( compiler, CNAME( DetailUVRotation ), shaderTarget ).x();
					CodeChunk Uy = SHADER_VAR_FLOAT( 1.0f - CodeChunkOperators::Abs(Ux), shaderTarget );

					// apply simple rotation matrix with two dot operations
					// [  cos(x) sin(x) ] = [  Uy  Ux ]
					// [ -sin(x) cos(x) ]   [ -Ux  Uy ]
					rotAxisX = SHADER_VAR_FLOAT2( Float2( Uy, Ux ), shaderTarget );
					rotAxisY = SHADER_VAR_FLOAT2( Float2( -Ux, Uy ), shaderTarget );
					detailUV = SHADER_VAR_FLOAT2( Float2( Dot2(rotAxisX, detailUV.xy()), Dot2(rotAxisY, detailUV.xy() ) ), shaderTarget );
				}

				// sample detail texture
				detailSwizzled = SHADER_VAR_FLOAT3( DecodeNormalmapSample( compiler, Tex2D( detailSamplerName, samplerName, detailUV ) ).xyz(), shaderTarget );

				// detail map NORMAL reverse rotation (compensates COORD rotation)
				{
					// mul = 1 / ( Ux*Ux + Uy*Uy );
					CodeChunk mul = SHADER_VAR_FLOAT( 1.f / (rotAxisX.x() * rotAxisY.y() - rotAxisX.y() * rotAxisY.x()), shaderTarget );
					// apply rotation matrix with negative angle with two dot operations [ cos(-x) = cos(x), -sin(x) = sin(x) ]
					// [  cos(-x) sin(-x) ] = [ cos(x) -sin(x) ] = [ Uy -Ux ]
					// [ -sin(-x) cos(-x) ]   [ sin(x)  cos(x) ] = [ Ux  Uy ]
					CodeChunk rotAxisX2 = mul * Float2( rotAxisY.y(), -rotAxisX.y() );
					CodeChunk rotAxisY2 = mul * Float2( -rotAxisY.x(), rotAxisX.x() );
					CodeChunk detailRotatedXY = SHADER_VAR_FLOAT2( Float2( Dot2(rotAxisX2, detailSwizzled.xy()), Dot2(rotAxisY2, detailSwizzled.xy() ) ), shaderTarget );
					detailSwizzled = SHADER_VAR_FLOAT3( Float3 ( detailRotatedXY, detailSwizzled.z() ), shaderTarget );
				}

				// normalize
				detailSwizzled = SHADER_VAR_FLOAT3( Normalize( detailSwizzled ), shaderTarget );
			}
			else
			{
				// sample detail texture
				detailSwizzled = SHADER_VAR_FLOAT3( DecodeNormalmapSample( compiler, Tex2D( detailSamplerName, samplerName, detailUV ) ).xyz(), shaderTarget );

				// normalize
				detailSwizzled = SHADER_VAR_FLOAT3( Normalize( detailSwizzled ), shaderTarget );
			}

			normal = SHADER_VAR_FLOAT3( Float3( swizzled.xy() + distanceBlend.xx() * detPow.xx() * detailSwizzled.xy(), swizzled.z() ), shaderTarget );
			normal = SHADER_VAR_FLOAT3( Normalize( normal ), shaderTarget );
		}
		else normal = swizzled;
			
		Vector scaleDecodedSample(m_scale.X, m_scale.Y, m_scale.Z);
		normal = SHADER_VAR_FLOAT3( normal.xyz() * scaleDecodedSample, shaderTarget );

		// Renormalize if needed
		if ( (scaleDecodedSample.X != 1.0f && scaleDecodedSample.X != -1.0f) || 
			 (scaleDecodedSample.Y != 1.0f && scaleDecodedSample.Y != -1.0f) || 
			 (scaleDecodedSample.Z != 1.0f && scaleDecodedSample.Z != -1.0f) )
		{
			normal = SHADER_VAR_FLOAT3( Normalize( normal ), shaderTarget );
		}

		if ( !m_sampleTangentSpace )
		{
			// Get tangent space basis in world space
			CodeChunk basisX = SHADER_DATA( "WorldTangent", shaderTarget );
			CodeChunk basisY = SHADER_DATA( "WorldBinormal", shaderTarget );
			CodeChunk basisZ = SHADER_DATA( "WorldNormal", shaderTarget );

			// Convert to world space
			CodeChunk worldNormalX = SHADER_VAR_FLOAT3( basisX * normal.x(), shaderTarget );
			CodeChunk worldNormalY = SHADER_VAR_FLOAT3( basisY * normal.y(), shaderTarget );
			CodeChunk worldNormalZ = SHADER_VAR_FLOAT3( basisZ * normal.z(), shaderTarget );
			CodeChunk worldNormal = worldNormalX + worldNormalY + worldNormalZ;
			return SHADER_VAR_FLOAT4( Float4( worldNormal, 0.0f ), shaderTarget );
		}
		else
		{
			return SHADER_VAR_FLOAT4( Float4( normal, 0.0f ), shaderTarget );
		}
	}
	else
	{
		if ( !m_sampleTangentSpace )
		{
			// No texture, use world space normal
			CodeChunk pos = SHADER_DATA( "WorldNormal", shaderTarget );
			return SHADER_VAR_FLOAT4( Float4( Normalize( pos.xyz() ), 0.0f ), shaderTarget );
		}
		else
		{
			return SHADER_VAR_FLOAT4( Float4( 0.0f, 0.0f, 1.0f, 0.0f ), shaderTarget );
		}
	}
}

#endif
