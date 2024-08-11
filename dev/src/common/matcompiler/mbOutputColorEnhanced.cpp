/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbOutputColorEnhanced.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

#include "../matcompiler//materialShaderConstants.h"
#include "..\..\common\engine\mbParamTexture.h"
#include "..\engine\baseEngine.h"
#include "..\engine\renderFragment.h"
#include "..\engine\graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutputColorEnhanced );

using namespace CodeChunkOperators;

CMaterialBlockOutputColorEnhanced::CMaterialBlockOutputColorEnhanced()
	: m_maskThreshold( 0.33f )
	, m_alphaToCoverageScale( 0 )
{
}

Bool CMaterialBlockOutputColorEnhanced::IsTwoSided() const
{
	return false;
}

Bool CMaterialBlockOutputColorEnhanced::IsTwoSideLighted() const
{
	return false;
}

ERenderingBlendMode CMaterialBlockOutputColorEnhanced::GetBlendMode() const
{
	return RBM_None;
}

Bool CMaterialBlockOutputColorEnhanced::IsEmissive() const
{
	return false;
}

Bool CMaterialBlockOutputColorEnhanced::IsAccumulativelyRefracted() const
{
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockOutputColorEnhanced::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

void CMaterialBlockOutputColorEnhanced::OnRebuildSockets()
{
	// Add sockets
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( AmbientOcclusion ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Diffuse ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Specularity ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Hardness ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Glossiness ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Emissive ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( EnvironmentMap ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Transmission ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( HeadDimming ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Mask ) ) );
}

#endif

void CMaterialBlockOutputColorEnhanced::Compile( CMaterialBlockCompiler& compiler ) const
{
	const ERenderingPass pass = compiler.GetMaterialCompiler()->GetContext().m_renderingContext->m_pass;

	EMaterialShaderTarget shaderTarget = MSH_PixelShader;

	// Mask 

	CodeChunk clipValue;
	if ( HasInput( CNAME( Mask ) ) ) 
	{
		CodeChunk mask = CompileInput( compiler, CNAME( Mask ), shaderTarget, MATERIAL_DEFAULT_MASK );
		clipValue = mask.w() - m_maskThreshold;
	}

	// Optional clipping

	compiler.GetPS().CompileOptionalFragmentClipping( compiler.GetMaterialCompiler()->GetContext(), clipValue );

	// Compile material color for given pass
	
	bool		outputDefined		 = false;
	CodeChunk	outputColor			 = MATERIAL_DEFAULT_OUTPUT_COLOR;
	
	switch ( pass )
	{
	case RP_HitProxies:
	case RP_ShadowDepthSolid:
	case RP_ShadowDepthMasked:
	case RP_HiResShadowMask:
		{
			outputDefined = compiler.GetPS().CompileRenderPassSimple( pass );
			if ( !outputDefined )
			{
				HALT( "Simple pass not recognized, or failed to compile");
			}			
		}	
		break;

	case RP_Emissive:
	case RP_RefractionDelta:
	{
		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", Float4( 0.0f, 0.0f, 0.0f, 0.0f ) );
		outputDefined = true;
		break;
	}

	case RP_ReflectionMask:
	{
		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", MATERIAL_DEFAULT_REFLECTIONMASK );
		outputDefined = true;
		break;
	}

	case RP_ForwardLightingTransparent:
	{
		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", Float4( 0.0f, 0.0f, 0.0f, 0.0f ) );
		outputDefined = true;
		break;
	}

	case RP_NoLighting:
	{
		outputColor = Float4 ( PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Diffuse ), shaderTarget,		MATERIAL_DEFAULT_DIFFUSE_GBUFFER ).xyz() ), 1.f );
		break;
	}
	
	case RP_GBuffer:
		{	
			{
				CodeChunk vertex_normal		= PS_VAR_FLOAT3( PS_DATA( "WorldNormal" ).xyz() );
				CodeChunk ambientOcclusion	= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( AmbientOcclusion ), shaderTarget,   MATERIAL_DEFAULT_AO   ).x() );
				CodeChunk diffuseGamma		= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Diffuse ), shaderTarget,		MATERIAL_DEFAULT_DIFFUSE_GBUFFER ).xyz() );
				CodeChunk normal			= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Normal ), shaderTarget,		Float3( vertex_normal ) ).xyz() );
				CodeChunk specularity		= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Specularity ), shaderTarget,	MATERIAL_DEFAULT_SPECULARITY ).xyz() );
				CodeChunk glossiness		= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Glossiness ), shaderTarget,   MATERIAL_DEFAULT_GLOSSINESS   ).x() );
				CodeChunk envColor			= PS_VAR_FLOAT3(  CompileInput( compiler, CNAME( EnvironmentMap ), shaderTarget,   Vector ( 0.0f, 0.0f, 0.0f, 1.0f )   ).xyz() );
				CodeChunk transmission		= 0.f;
				CodeChunk translucency		= transmission; //< whatever / dead code anyway
				CodeChunk subsurfaceScattering = PS_VAR_FLOAT( 0.0f );

				normal = compiler.GetPS().PrepareForTwoSidedLighting( normal, vertex_normal );

				if ( compiler.GetMaterialCompiler()->GetContext().m_selected )
				{
					// Selection
					CodeChunk selectionColor = compiler.GetPS().ConstReg( MDT_Float4, "PSC_ConstColor" );
					CodeChunk selectionEffects = compiler.GetPS().ConstReg( MDT_Float4, "PSC_SelectionEffect" );
					CodeChunk diffuseGammaDesat = PS_VAR_FLOAT3( CodeChunkOperators::Dot3( diffuseGamma.xyz(), RGB_LUMINANCE_WEIGHTS_GAMMA_SelectionDesaturate ) );
					diffuseGamma = PS_VAR_FLOAT3( CodeChunkOperators::Lerp( diffuseGamma.xyz(), diffuseGammaDesat.xyz(), selectionEffects.x() ) );			
					diffuseGamma = PS_VAR_FLOAT3( CodeChunkOperators::Lerp( diffuseGamma.xyz(), selectionColor.xyz(), selectionColor.w() ) );
				}

				outputDefined = compiler.GetPS().CompileDeferredShadingGBuffer( diffuseGamma, normal, vertex_normal, specularity, glossiness, translucency, ambientOcclusion, subsurfaceScattering );

				if ( m_alphaToCoverageScale > 0 && HasInput( CNAME( Mask ) ) )
				{
					CodeChunk compareVal = PS_VAR_FLOAT( Saturate( clipValue / ((1.0f - m_maskThreshold) * m_alphaToCoverageScale ) ) );
					compiler.GetPS().CompileMSAACoverageMask( compareVal );
				}

				if ( !outputDefined )
				{
					HALT( "Failed to generate deferred shading gbuffer" );
				}
			}
		}
		break;

	case RP_ForwardLightingSolid:
	{
		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", Float4( 1.f, 1.f, 1.f, 1.f ) );	
		outputDefined = true;
	}

	default:
		ASSERT( !"Invalid rendering context pass" );
	};

	// Postprocess calculated color

	if ( !outputDefined )
	{		
		// Handle selection

		if ( compiler.GetMaterialCompiler()->GetContext().m_selected )
		{
			ApplyMaterialSelectionColor4v( outputColor, pass );
		}

		// Finalize

		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", outputColor );	
	}
}

void CMaterialBlockOutputColorEnhanced::GetValidPasses( TDynArray< ERenderingPass >& passes ) const
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
}

#endif
