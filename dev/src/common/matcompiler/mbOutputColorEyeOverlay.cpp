/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbOutputColorEyeOverlay.h"
#include "../matcompiler//materialShaderConstants.h"
#include "..\..\common\engine\mbParamTexture.h"
#include "..\engine\renderFragment.h"
#include "..\engine\graphBlock.h"
#include "..\engine\graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutputColorEyeOverlay );

using namespace CodeChunkOperators;

CMaterialBlockOutputColorEyeOverlay::CMaterialBlockOutputColorEyeOverlay()
	: m_isTwoSided( false )
	, m_maskThreshold( 0.33f )
{
}

Bool CMaterialBlockOutputColorEyeOverlay::IsTwoSided() const
{
	return m_isTwoSided;
}

Bool CMaterialBlockOutputColorEyeOverlay::IsTwoSideLighted() const
{
	return false;
}

ERenderingBlendMode CMaterialBlockOutputColorEyeOverlay::GetBlendMode() const
{
	return RBM_None;
}

Bool CMaterialBlockOutputColorEyeOverlay::IsEmissive() const
{	
	return false;
}

Bool CMaterialBlockOutputColorEyeOverlay::IsAccumulativelyRefracted() const
{
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockOutputColorEyeOverlay::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

void CMaterialBlockOutputColorEyeOverlay::OnRebuildSockets()
{
	// Add sockets
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Color ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Mask ) ) );
}

#endif

void CMaterialBlockOutputColorEyeOverlay::Compile( CMaterialBlockCompiler& compiler ) const
{
	const ERenderingPass pass = compiler.GetMaterialCompiler()->GetContext().m_renderingContext->m_pass;

	EMaterialShaderTarget shaderTarget = MSH_PixelShader;

	// Mask 
	CodeChunk clipValue;
	if ( HasInput( CNAME( Mask ) ) ) 
	{
		CodeChunk mask = CompileInput( compiler, CNAME( Mask ), shaderTarget, 1.0f );
		clipValue = mask.w() - m_maskThreshold;
	}

	// Optional clipping
	compiler.GetPS().CompileOptionalFragmentClipping( compiler.GetMaterialCompiler()->GetContext(), clipValue );

	// Simple pass
	if ( compiler.GetPS().CompileRenderPassSimple( pass ) )
	{
		// Root chunks does not return value
		return;
	}

	// GBuffer
	if ( pass == RP_GBuffer )
	{
		CodeChunk diffuseInput		= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Color ), shaderTarget, MATERIAL_DEFAULT_DIFFUSE_GBUFFER ).xyz() );
		CodeChunk diffuseGamma		= compiler.GetPS().ApplyGammaToLinearExponent( MDT_Float3, diffuseInput, true, true );
		CodeChunk normal			= PS_VAR_FLOAT3( PS_DATA( "WorldNormal" ).xyz() );
		CodeChunk specularity		= MATERIAL_DEFAULT_SPECULARITY;
		CodeChunk glossiness		= MATERIAL_DEFAULT_GLOSSINESS;
		CodeChunk translucency		= MATERIAL_DEFAULT_TRANSLUCENCY;
		CodeChunk ambientOcclusion	= MATERIAL_DEFAULT_AO;
		CodeChunk subsurfaceScattering= MATERIAL_DEFAULT_SS;

		// Selection
		if ( compiler.GetMaterialCompiler()->GetContext().m_selected )
		{
			CodeChunk selectionColor = compiler.GetPS().ConstReg( MDT_Float4, "PSC_ConstColor" );
			CodeChunk selectionEffects = compiler.GetPS().ConstReg( MDT_Float4, "PSC_SelectionEffect" );
			CodeChunk diffuseGammaDesat = PS_VAR_FLOAT3( CodeChunkOperators::Dot3( diffuseGamma.xyz(), RGB_LUMINANCE_WEIGHTS_GAMMA_SelectionDesaturate ) );
			diffuseGamma = PS_VAR_FLOAT3( CodeChunkOperators::Lerp( diffuseGamma.xyz(), diffuseGammaDesat.xyz(), selectionEffects.x() ) );				
			diffuseGamma = PS_VAR_FLOAT3( CodeChunkOperators::Lerp( diffuseGamma.xyz(), selectionColor.xyz(), selectionColor.w() ) );
		}

		if ( compiler.GetPS().CompileDeferredShadingGBuffer( diffuseGamma, normal, normal, specularity, glossiness, translucency, ambientOcclusion, subsurfaceScattering ) )
		{
			return;
		}
		else
		{
			HALT( "Failed to generate deferred shading gbuffer" );
		}
	}	

	// Emissive
	if ( pass == RP_Emissive )
	{
		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", Float4 (0.f, 0.f, 0.f, 1.f) );
		return;
	}

	// Calculate color and alpha
	CodeChunk color = CompileInput( compiler, CNAME( Color ), shaderTarget, MATERIAL_DEFAULT_OUTPUT_COLOR );	
	CodeChunk alpha = PS_VAR_FLOAT4( 1.f );
	
	// Selection
	if ( compiler.GetMaterialCompiler()->GetContext().m_selected )
	{
		ApplyMaterialSelectionColor4v( color, pass );
	}

	// Mix color and alpha
	CodeChunk mixedColor = PS_VAR_FLOAT4( Float4( color.xyz(), alpha.w() ) );

	compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", mixedColor );
}

void CMaterialBlockOutputColorEyeOverlay::GetValidPasses( TDynArray< ERenderingPass >& passes ) const
{
	// Defaults
	passes.PushBackUnique( RP_ShadowDepthSolid );	// we need this because we can have it dissolving
	passes.PushBackUnique( RP_ShadowDepthMasked );	// we need this for the masking
	passes.PushBackUnique( RP_HitProxies );			// hit proxy for everything
	passes.PushBackUnique( RP_HiResShadowMask );	// 

	// Supported
	passes.PushBackUnique( RP_GBuffer );
	passes.PushBackUnique( RP_NoLighting );
	passes.PushBackUnique( RP_ForwardLightingSolid );
	passes.PushBackUnique( RP_ForwardLightingTransparent );
}
#endif
