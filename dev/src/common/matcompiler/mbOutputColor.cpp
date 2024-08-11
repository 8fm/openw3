/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbOutputColor.h"
#include "../matcompiler//materialShaderConstants.h"
#include "../../../../bin/shaders/include/globalConstants.fx"
#include "..\..\common\engine\mbParamTexture.h"
#include "..\engine\renderFragment.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutputColor );

RED_DEFINE_STATIC_NAME( ReflectionOffset )
RED_DEFINE_STATIC_NAME( ReflectionAmount )
RED_DEFINE_STATIC_NAME( GlobalReflection )

using namespace CodeChunkOperators;

CodeChunk CompileGlobalFogData( CMaterialBlockCompiler& compiler, Bool isSky, Bool isClouds, Bool vertexBased, Bool calcFullData );

CMaterialBlockOutputColor::CMaterialBlockOutputColor()
	: m_isTwoSided( false )
	, m_noDepthWrite( false )
	, m_inputColorLinear( true )
	, m_maskThreshold( 0.33f )
	, m_blendMode( RBM_None )
	, m_checkRefractionDepth( false )
	, m_implicitTransparencyColor( true )
	, m_implicitTransparencyAlpha( true )
	, m_implicitGlobalFogVertexBased( true )
	, m_implicitGlobalFog( true )
{
}

Bool CMaterialBlockOutputColor::IsTwoSided() const
{
	return m_isTwoSided;
}

Bool CMaterialBlockOutputColor::IsTwoSideLighted() const
{
	return false;
}

ERenderingBlendMode CMaterialBlockOutputColor::GetBlendMode() const
{
	return m_blendMode;
}

Bool CMaterialBlockOutputColor::IsEmissive() const
{	
	return false;
}

Bool CMaterialBlockOutputColor::HasBlending() const
{
	return m_blendMode != RBM_None;
}

Bool CMaterialBlockOutputColor::IsReflectiveMasked() const
{
	return HasInput( CNAME( ReflectionAmount ) );
}

Bool CMaterialBlockOutputColor::IsAccumulativelyRefracted() const
{
	Bool allowedByBlendMode = (RBM_None != m_blendMode && RBM_Refractive != m_blendMode);
	return allowedByBlendMode && HasInput(CNAME(RefractionDelta));
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockOutputColor::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

void CMaterialBlockOutputColor::OnRebuildSockets()
{
	// Add sockets
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Color ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Opacity ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( RefractionDelta ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( ReflectionOffset ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( ReflectionAmount ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( GlobalReflection ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Mask ) ) );
}

#endif

//get by parameter name- for example "DiffuseMap" or "Diffues(NEAR)"
CMaterialBlock* CMaterialBlockOutputColor::FindDebugMipMapsSampler( const CName& parameterName ) const
{
	return NULL;
}

void CMaterialBlockOutputColor::Compile( CMaterialBlockCompiler& compiler ) const
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
		CodeChunk diffuseGamma		= m_inputColorLinear ? compiler.GetPS().ApplyGammaToLinearExponent( MDT_Float3, diffuseInput, true, true ) : diffuseInput;
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

	// RefractionDelta
	if ( pass == RP_RefractionDelta )
	{
		CodeChunk multiplier = 1.f;
		Float s = 1.f / ACCUMULATIVE_REFRACTION_MAX_OFFSET;
		CodeChunk delta = CompileInput( compiler, CNAME( RefractionDelta ), shaderTarget, MATERIAL_DEFAULT_REFRACTIONDELTA );
		CodeChunk alpha = PS_DATA( "TranspFadeOutAlpha" );

		if ( m_checkRefractionDepth )
		{
			if ( compiler.GetPS().IsFeedbackDataFetchSupported( RFDT_Depth ) )
			{
				// Compile scene depth
				CodeChunk refDepth   = PS_DATA( "ViewZ" ).x();
				CodeChunk sceneDepth = 0.f;

				CodeChunk ratio = compiler.GetPS().ConstReg( MDT_Float4, "PSC_ViewportSubSize" ).zw();
				CodeChunk offset = PS_VAR_FLOAT2( ratio * (delta * alpha).xy() );

				compiler.GetPS().CompileFeedbackDataFetch( RFDT_Depth, sceneDepth, &offset );

				// Calculate sharpness
				multiplier = CodeChunkOperators::Saturate( (sceneDepth.x() - refDepth) * 100.0f );
			}
		}

		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", multiplier * (delta * alpha).xyxy() * Float4 (s, s, -s, -s) );
		return;
	}

	// Reflection mask
	if ( pass == RP_ReflectionMask )
	{
		CodeChunk reflectionMask = MATERIAL_DEFAULT_REFLECTIONMASK;

		if ( IsReflectiveMasked() )
		{
			const CodeChunk offset = PS_VAR_FLOAT2( CompileInput( compiler, CNAME( ReflectionOffset ), shaderTarget, Vector ( 0, 0, 0, 0 ) ).xy() );
			const CodeChunk amount = PS_VAR_FLOAT( CompileInput( compiler, CNAME( ReflectionAmount ), shaderTarget, Vector::ZEROS ).x() );
			CodeChunk maskResult = compiler.GetPS().AutomaticName();

			CodeChunk call;
			if ( HasInput( CNAME( GlobalReflection ) ) ) //< explicit check so that we wouldn't mess with materials patching
			{
				const CodeChunk globalReflection = PS_VAR_FLOAT( CompileInput( compiler, CNAME( GlobalReflection ), shaderTarget, Vector::ZEROS ).x() );
				call = CodeChunk::Printf( false, "float4 %s = EncodeRealtimeReflectionsMask( %s, %s, (%s > 0.5) );", maskResult.AsChar(), offset.AsChar(), amount.AsChar(), globalReflection.AsChar() );
			}
			else
			{
				call = CodeChunk::Printf( false, "float4 %s = EncodeRealtimeReflectionsMask( %s, %s );", maskResult.AsChar(), offset.AsChar(), amount.AsChar() );
			}

			compiler.GetPS().Statement( call );

			reflectionMask = maskResult;
		}
		
		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", reflectionMask );
		return;
	}

	// Emissive
	if ( pass == RP_Emissive )
	{
		compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", Float4 (0.f, 0.f, 0.f, 1.f) );
		return;
	}

	// Calculate color and alpha
	CodeChunk color = CompileInput( compiler, CNAME( Color ), shaderTarget, MATERIAL_DEFAULT_OUTPUT_COLOR );	
	CodeChunk alpha = CompileInput( compiler, CNAME( Opacity ), shaderTarget, 1.0f );
	if ( !m_inputColorLinear && HasInput( CNAME( Color ) ) )
	{
		color = compiler.GetPS().ApplyGammaToLinearExponent( MDT_Float4, color, false, true );
	}

	// Selection
	if ( compiler.GetMaterialCompiler()->GetContext().m_selected )
	{
		ApplyMaterialSelectionColor4v( color, pass );
	}

	// Mix color and alpha
	CodeChunk mixedColor = PS_VAR_FLOAT4( Float4( color.xyz(), alpha.w() ) );
	switch ( m_blendMode )
	{
	case RBM_None:		
		if ( m_implicitGlobalFog )
		{
			CodeChunk fogResult = compiler.GetPS().AutomaticName();
			CodeChunk fogData = CompileGlobalFogData( compiler, false, false, m_implicitGlobalFogVertexBased, true );
			CodeChunk call = CodeChunk::Printf( false, "float4 %s = ApplyFog_Opaque( %s, %s );", fogResult.AsChar(), mixedColor.xyz().AsChar(), fogData.AsChar() );
			compiler.GetPS().Statement( call );

			mixedColor = PS_VAR_FLOAT4( Float4( fogResult.xyz(), mixedColor.w() ) );
		}
		break;

	case RBM_Addtive:
		{
			CodeChunk colorScale = m_implicitTransparencyColor ? PS_DATA( "TranspColorScale" ) : Float4(1.f, 1.f, 1.f, 1.f);
			CodeChunk fadeOutAlpha = m_implicitTransparencyAlpha ? PS_DATA( "TranspFadeOutAlpha" ) : CodeChunk(1.f);
			if ( m_implicitGlobalFog )
			{
				mixedColor = PS_VAR_FLOAT4( Float4( color.xyz() * colorScale.xyz(), 0.f ) );

				CodeChunk fogResult = compiler.GetPS().AutomaticName();
				CodeChunk fogData = CompileGlobalFogData( compiler, false, false, m_implicitGlobalFogVertexBased, true );
				CodeChunk call = CodeChunk::Printf( false, "float4 %s = ApplyFog_Additive( %s, %s, %s );", fogResult.AsChar(), mixedColor.xyz().AsChar(), fadeOutAlpha.AsChar(), fogData.AsChar());
				compiler.GetPS().Statement( call );

				mixedColor = Float4( fogResult.xyz() * fogResult.w(), 0.f );
			}
			else
			{
				mixedColor = PS_VAR_FLOAT4( Float4( color.xyz() * colorScale.xyz() * fadeOutAlpha, 0.f ) ); 
			}
		}
		break;

	case RBM_AlphaBlend:
		{
			CodeChunk colorScale = m_implicitTransparencyColor ? PS_DATA( "TranspColorScale" ) : Float4(1.f, 1.f, 1.f, 1.f);
			CodeChunk finalAlpha = alpha.w() * (m_implicitTransparencyAlpha ? PS_DATA( "TranspFadeOutAlpha" ) : CodeChunk(1.f));
			if ( m_implicitGlobalFog )
			{
				mixedColor = PS_VAR_FLOAT4( Float4( color.xyz() * colorScale.xyz(), 0.f ) );

				CodeChunk fogResult = compiler.GetPS().AutomaticName();
				CodeChunk fogData = CompileGlobalFogData( compiler, false, false, m_implicitGlobalFogVertexBased, true );
				CodeChunk call = CodeChunk::Printf( false, "float4 %s = ApplyFog_AlphaBlend( %s, %s, %s );", fogResult.AsChar(), mixedColor.xyz().AsChar(), finalAlpha.AsChar(), fogData.AsChar() );
				compiler.GetPS().Statement( call );

				mixedColor = Float4( fogResult.xyz() * fogResult.w(), fogResult.w() );
			}
			else
			{
				mixedColor = PS_VAR_FLOAT4( Float4( color.xyz() * colorScale.xyz() * finalAlpha, finalAlpha ) ); 
			}
		}
		break;

	case RBM_Refractive:
		{
			// Implicit fog not supported for Refractive materials, since
			// it's color already contains refraction with fog applied.
			// So we don't want to apply any additional fog.
			mixedColor = PS_VAR_FLOAT4( Float4( color.xyz(), 1.f ) );
		}
		break;

	default:				
		ASSERT( !"invalid blend mode" );
	} 

	compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", mixedColor );
}

void CMaterialBlockOutputColor::GetValidPasses( TDynArray< ERenderingPass >& passes ) const
{
	// Defaults
	passes.PushBackUnique( RP_ShadowDepthSolid );	// we need this because we can have it dissolving
	passes.PushBackUnique( RP_ShadowDepthMasked );	// we need this for the masking
	passes.PushBackUnique( RP_HitProxies );			// hit proxy for everything
	passes.PushBackUnique( RP_HiResShadowMask );	// 

	// Supported without blending
	if( !HasBlending() )
	{
		passes.PushBackUnique( RP_GBuffer );
	}
	
	if ( IsEmissive() )
	{
		passes.PushBack( RP_Emissive );
	}

	if ( IsAccumulativelyRefracted() )
	{
		passes.PushBack( RP_RefractionDelta );
	}

	if ( IsReflectiveMasked() )
	{
		passes.PushBack( RP_ReflectionMask );
	}

	// Supported with blending
	passes.PushBackUnique( RP_NoLighting );
	passes.PushBackUnique( RP_ForwardLightingSolid );
	passes.PushBackUnique( RP_ForwardLightingTransparent );
}

#endif
