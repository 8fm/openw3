/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/renderFragment.h"
#include "../engine/materialInputTextureSocket.h"
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Sample terrain material
class CMaterialTerrainMaterialSampler : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialTerrainMaterialSampler, CMaterialBlock, "Deprecated", "Terrain Material Sampler" );

protected:
	Vector	m_normalScale;

public:
	CMaterialTerrainMaterialSampler()
		: m_normalScale ( 1.f, 1.f, 1.f, 1.f )
	{}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );

		// Tile map
		CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( MaskTexture ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( MaskTexelSize ) ) );

		// Atlases
		CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( DiffuseAtlasTexture ) ) );
		CreateSocket( MaterialInputTextureSocketSpawnInfo( CNAME( NormalAtlasTexture ) ) );

		// Detail scale
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( DetailScale ) ) );

		// Outputs
		CreateSocket( MaterialOutputSocketSpawnInfo( String( TXT("[0]") ), CNAME( Diffuse ), Color::WHITE ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( String( TXT("[1]") ), CNAME( Normal ), Color::WHITE ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( String( TXT("[2]") ), CNAME( Specularity ), Color::WHITE ) );
	}
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		// Compile the terrain material only for the deferred pass
		const RenderingContext& renderingContext = *compiler.GetMaterialCompiler()->GetContext().m_renderingContext;
		if ( renderingContext.m_pass == RP_GBuffer || renderingContext.m_pass == RP_NoLighting )
		{
			// Make sure all needed parameters are connected
			Bool isValid = HasInput( CNAME( MaskTexture ) );
			isValid &= HasInput( CNAME( MaskTexelSize ) );
			isValid &= HasInput( CNAME( DiffuseAtlasTexture ) );
			isValid &= HasInput( CNAME( NormalAtlasTexture ) );
			isValid &= HasInput( CNAME( DetailScale ) );
			if ( isValid )
			{
				// Get the input semantics
				CodeChunk uv = PS_DATA( "TexCoords0" );
				CodeChunk worldUVW = PS_DATA( "WorldPosition" );
				CodeChunk normal = PS_DATA( "WorldNormal" );
				
				// Get the mask texture and parameters
				CodeChunk maskSampler = CompileInput( compiler, CNAME( MaskTexture ), shaderTarget );
				CodeChunk maskTexelSize = CompileInput( compiler, CNAME( MaskTexelSize ), shaderTarget );

				// For Xenon diffuse atlas and normal atlas are the same texture :)
				CName normalAtlasInput = CNAME( NormalAtlasTexture );

				// Get the atlases
				CodeChunk diffuseAtlasSampler = CompileInput( compiler, CNAME( DiffuseAtlasTexture ), shaderTarget );
				CodeChunk normalAtlasSampler = CompileInput( compiler, normalAtlasInput, shaderTarget );

				// Get the detail scale
				CodeChunk detailScale  = CompileInput( compiler, CNAME( DetailScale ), shaderTarget );
				CodeChunk detailScaleW = 0.5f * (detailScale.x() + detailScale.y());
				CodeChunk atlasUVW = compiler.GetPS().Var( MDT_Float3, Float3(detailScale.xy(), detailScaleW) * worldUVW.xyz() * Float3 ( 1.f, -1.f, 1.f ) );
				
				// Get normal scale
				CodeChunk normalScale = m_normalScale;

				// Calculate terrain material parameters
				CodeChunk param = compiler.GetPS().AutomaticName();
				CodeChunk call = CodeChunk::Printf( false, "TerrainMaterialData %s = CalcTerrainMaterial( (%s), (%s), 666.0, 666.0, (%s), (%s), (%s), %s, %s, %s );", param.AsChar(), uv.AsChar(), atlasUVW.AsChar(), normal.AsChar(), maskSampler.AsChar(), maskTexelSize.AsChar(), diffuseAtlasSampler.AsChar(), normalAtlasSampler.AsChar(), normalScale.xyz().AsChar() );
				compiler.GetPS().Statement( call );

				// Allocate the output an call the material function
				CodeChunk result = CodeChunk::Printf( false, "{ %s.Diffuse, %s.Normal, %s.Specularity }", param.AsChar(), param.AsChar(), param.AsChar() );
				return compiler.GetPS().Var( MDT_Float4, 3, result );
			}
		}

		// Output default color
		CodeChunk defaultParams = CodeChunk( "{ float4(1,1,1,1), float4(0,0,1,0), float4(0.2,0.2,0.2,1) }", true );
		CodeChunk defaultOutput = compiler.GetPS().Var( MDT_Float4, 3, defaultParams );
		return defaultOutput;
	}
};

BEGIN_CLASS_RTTI(CMaterialTerrainMaterialSampler)
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_normalScale, TXT("Normalmap scale") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialTerrainMaterialSampler );

#endif