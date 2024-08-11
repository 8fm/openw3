/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/renderFragment.h"
#include "../engine/materialBlock.h"
#include "../engine/graphBlock.h"
#include "../engine/materialInputTextureSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "mbTerrainMaterialBlending.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

RED_DEFINE_STATIC_NAME( DiffuseTextureArray );
RED_DEFINE_STATIC_NAME( NormalTextureArray );
RED_DEFINE_STATIC_NAME( VerticalUVTightness );
RED_DEFINE_STATIC_NAME( HorizontalUVTightness );
RED_DEFINE_STATIC_NAME( SpecularContrast );

CMaterialTerrainMaterialBlending::CMaterialTerrainMaterialBlending()
{
	Float defaultScales[7] = { 0.333f, 0.166f, 0.05f, 0.025f, 0.0125f, 0.0075f, 0.00375f };
	for( Uint32 i = 0; i < 7; ++i )
	{
		m_uvScales[i] = defaultScales[i];
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CMaterialTerrainMaterialBlending::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	// Texture inputs
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( DiffuseTextureArray ) ) );
	CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( NormalTextureArray ) ) );

	// Outputs
	CreateSocket( MaterialOutputSocketSpawnInfo( String( TXT("Diffuse") ), CNAME( Diffuse ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( String( TXT("Normal") ), CNAME( Normal ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( String( TXT("Roughness") ), CNAME( Roughness ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( String( TXT("Params.xxxx") ), CNAME( Specularity ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( String( TXT("Params.yyyy") ), CNAME( RSpec_Base ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( String( TXT("Params.zzzz") ), CNAME( Fallof ), Color::WHITE ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( String( TXT("Params.wwww") ), CNAME( RSpec_Scale ), Color::WHITE ) );
}
#endif // NO_EDITOR_GRAPH_SUPPORT

CodeChunk CMaterialTerrainMaterialBlending::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	const CodeChunk blendResultName( "materialBlendResult" );

	// Compile the terrain material only for the deferred pass
	const RenderingContext& renderingContext = *compiler.GetMaterialCompiler()->GetContext().m_renderingContext;
	if ( renderingContext.m_pass == RP_GBuffer || renderingContext.m_pass == RP_NoLighting )
	{
		// Make sure all needed parameters are connected
		Bool isValid = HasInput( CNAME( DiffuseTextureArray ) );
		isValid &= HasInput( CNAME( NormalTextureArray ) );
		if ( isValid )
		{
			if ( !compiler.GetCompiledBlockOutputs( this ) )
			{
				// Grab diffuse texture array socket
				CMaterialInputTextureSocket* diffuseTextureArraySocket = Cast< CMaterialInputTextureSocket >( FindSocket( CNAME( DiffuseTextureArray ) ) );
				CodeChunk diffuseTAName = diffuseTextureArraySocket->Compile( compiler, shaderTarget );

				// Grab normal texture array socket
				CMaterialInputTextureSocket* normalTextureArraySocket = Cast< CMaterialInputTextureSocket >( FindSocket( CNAME( NormalTextureArray ) ) );
				CodeChunk normalTAName = normalTextureArraySocket->Compile( compiler, shaderTarget );

				// Get the crucial input semantics...
				CodeChunk regionUV = SHADER_DATA( "TexCoords0", shaderTarget );
				CodeChunk regionUVColorMap = SHADER_DATA( "TexCoords1", shaderTarget );
				CodeChunk vertexPos = SHADER_DATA( "WorldPosition", shaderTarget );
				CodeChunk clipmapLevel = SHADER_DATA( "TexIndex", shaderTarget );

				// ...and an optional one. It may contain a clipmap normal, a stamp normal, or nothing.
				CodeChunk vertexNormal = SHADER_VAR_FLOAT3( 0.0f, shaderTarget );
				if ( !compiler.GetMaterialCompiler()->GetContext().m_pregeneratedMaps || compiler.GetMaterialCompiler()->GetContext().m_renderingContext->m_terrainToolStampVisible )
				{
					vertexNormal = SHADER_DATA( "VertexNormal", shaderTarget );
				}

				// Calculate terrain material parameters
				CodeChunk call;
				// When doing tesselated terrain, with stamp data, we need to pass a couple extra parameters.
				if ( compiler.GetMaterialCompiler()->GetContext().m_vertexFactory == MVF_TesselatedTerrain && compiler.GetMaterialCompiler()->GetContext().m_renderingContext->m_terrainToolStampVisible )
				{
					CodeChunk stampUV = SHADER_DATA( "VertexColor", shaderTarget );
					call = CodeChunk::Printf( false, "TerrainMaterialBlended %s = CalcTerrainMaterial( (%s), (%s), (%s), (%s), (%s), (%s), (%s), (%s), (%s), (%s) );", 
						blendResultName.AsChar(), vertexPos.xyz().AsChar(), vertexNormal.AsChar(), regionUV.AsChar(), clipmapLevel.AsChar(), regionUVColorMap.AsChar(), diffuseTAName.AsChar(), normalTAName.AsChar(), stampUV.xy().AsChar(), stampUV.z().AsChar(), stampUV.w().AsChar() );
				}
				else
				{
					call = CodeChunk::Printf( false, "TerrainMaterialBlended %s = CalcTerrainMaterial( (%s), (%s), (%s), (%s), (%s), (%s), (%s) );", 
						blendResultName.AsChar(), vertexPos.xyz().AsChar(), vertexNormal.AsChar(), regionUV.AsChar(), clipmapLevel.AsChar(), regionUVColorMap.AsChar(), diffuseTAName.AsChar(), normalTAName.AsChar() );
				}
				compiler.GetPS().Statement( call );
				
				// HACK: Terrain scales definitinon
				compiler.GetPS().Macro( CodeChunk::Printf( true, "static const float scales[7] = { %.3f, %.3f, %.2f, %.3f, %.4f, %.4f, %.5f };",
					m_uvScales[0], m_uvScales[1], m_uvScales[2], m_uvScales[3], m_uvScales[4], m_uvScales[5], m_uvScales[6] ) );
			}

			// Return value based on the output socket
			return blendResultName;
		}
	}

	// Output default
	CodeChunk defaultOutput = CodeChunk::Printf( false, "(TerrainMaterialBlended)0" );
	return defaultOutput;
}

IMPLEMENT_ENGINE_CLASS( CMaterialTerrainMaterialBlending );

#endif
