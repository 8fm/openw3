/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbOutputColorDeferred.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

#include "../matcompiler//materialShaderConstants.h"
#include "..\..\common\engine\mbParamTexture.h"
#include "..\engine\baseEngine.h"
#include "..\engine\renderFragment.h"
#include "..\engine\graphConnection.h"

RED_DEFINE_STATIC_NAME( GlossinessSecond )
RED_DEFINE_STATIC_NAME( SubsurfaceScattering )
RED_DEFINE_STATIC_NAME( MaterialFlagsMask )
//RED_DEFINE_STATIC_NAME( Roughness )
//RED_DEFINE_STATIC_NAME( Glossiness )

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutputColorDeferred );

using namespace CodeChunkOperators;

CMaterialBlockOutputColorDeferred::CMaterialBlockOutputColorDeferred()
	: m_isTwoSided( false )
	, m_rawOutput( false )
	, m_maskThreshold( 0.33f )
	, m_terrain( false )
{
}

Bool CMaterialBlockOutputColorDeferred::IsTwoSided() const
{
	return m_isTwoSided;
}

ERenderingBlendMode CMaterialBlockOutputColorDeferred::GetBlendMode() const
{
	return RBM_None; // deferred lighting always without blending.
}

Bool CMaterialBlockOutputColorDeferred::IsEmissive() const
{
	return HasInput( CNAME( Emissive ) );
}

Bool CMaterialBlockOutputColorDeferred::IsAccumulativelyRefracted() const
{
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockOutputColorDeferred::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

void CMaterialBlockOutputColorDeferred::OnRebuildSockets()
{
	// Add sockets
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( AmbientOcclusion ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Diffuse ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Specularity ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Roughness ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Glossiness ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( GlossinessSecond ), false ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Emissive ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Translucency ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( SubsurfaceScattering ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( MaterialFlagsMask ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Mask ) ) );
}

#endif

void CMaterialBlockOutputColorDeferred::Compile( CMaterialBlockCompiler& compiler ) const
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
	case RP_NoLighting:
		{
			outputColor	= Float4 ( CompileInput( compiler, CNAME( Diffuse ), shaderTarget, MATERIAL_DEFAULT_DIFFUSE ).xyz(), 1.f );
			outputColor = compiler.GetPS().ApplyGammaToLinearExponent( MDT_Float4, outputColor, false, true );
		}		
		break;

	case RP_HitProxies:
	case RP_ShadowDepthSolid:
	case RP_ShadowDepthMasked:
	case RP_HiResShadowMask:
		{
			outputDefined = compiler.GetPS().CompileRenderPassSimple( pass );
			if ( !outputDefined )
			{
				HALT( "Simple pass not recognized, or failed to compile" );
			}			
		}	
		break;

	case RP_GBuffer:
		{	
			// include some utils functions			
			compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );
			CodeChunk uvDensityColor = PS_VAR_FLOAT3( Float3( 1.0f,1.0f,0.0f ) );

			// UV Density
			if ( compiler.GetMaterialCompiler()->GetContext().m_materialDebugMode == MDM_UVDensity && !compiler.IsTerrainShader() )
			{
				uvDensityColor = Float3( 0.0f, 0.0f, 0.f );
				CodeChunk colorA = PS_VAR_FLOAT3( Float3( 1.0f,1.0f,0.0f ) );
				CodeChunk colorB = PS_VAR_FLOAT3( Float3( 0.0f,0.0f,1.0f ) );

				CodeChunk uv = PS_VAR_FLOAT2( PS_DATA( "TexCoords0" ).xy() );
				CodeChunk tiledUV = uv * 10.0f;
				CodeChunk param = compiler.GetPS().AutomaticName();
				CodeChunk call = CodeChunk::Printf( false, "float3 %s = Checker( %s, %s, %s );", param.AsChar(), tiledUV.AsChar(), colorA.AsChar(), colorB.AsChar() );
				compiler.GetPS().Statement( call );
				uvDensityColor = param;
			}

			{
				CodeChunk vertex_normal		= PS_VAR_FLOAT3( PS_DATA( "WorldNormal" ).xyz() );		
				CodeChunk diffuseGamma		= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Diffuse ), shaderTarget,		MATERIAL_DEFAULT_DIFFUSE_GBUFFER ).xyz() );
				CodeChunk normal			= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Normal ), shaderTarget,		Float4( Normalize( vertex_normal.xyz() ), 0.0f ) ));

				normal = compiler.GetPS().PrepareForTwoSidedLighting( normal, vertex_normal );

				CodeChunk specularityGamma	= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Specularity ), shaderTarget,	MATERIAL_DEFAULT_SPECULARITY ).xyz() );
				CodeChunk roughness			= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Roughness ), shaderTarget,   MATERIAL_DEFAULT_ROUGHNESS  ).x() );
				CodeChunk ambientOcclusion	= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( AmbientOcclusion ), shaderTarget,   MATERIAL_DEFAULT_AO   ).x() );
				
				CodeChunk subsurfaceScattering = PS_VAR_FLOAT(  CompileInput( compiler, CNAME( SubsurfaceScattering ), shaderTarget,   MATERIAL_DEFAULT_SS  ).x() );

				CodeChunk translucency		= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Translucency ), shaderTarget, MATERIAL_DEFAULT_TRANSLUCENCY ).x() );
				
				// UV Density
				if ( compiler.GetMaterialCompiler()->GetContext().m_materialDebugMode == MDM_UVDensity && !compiler.IsTerrainShader() )
				{
					diffuseGamma		= uvDensityColor;
					normal				= Float3( 0.f, 0.f, 1.f );
					specularityGamma	= PS_VAR_FLOAT3( Float3(0.f,0.f,0.f) );
				}
				else if ( compiler.GetMaterialCompiler()->GetContext().m_selected )
				{
					// Selection
					CodeChunk selectionColor = compiler.GetPS().ConstReg( MDT_Float4, "PSC_ConstColor" );
					CodeChunk selectionEffects = compiler.GetPS().ConstReg( MDT_Float4, "PSC_SelectionEffect" );
					CodeChunk diffuseGammaDesat = PS_VAR_FLOAT3( CodeChunkOperators::Dot3( diffuseGamma.xyz(), RGB_LUMINANCE_WEIGHTS_GAMMA_SelectionDesaturate ) );
					diffuseGamma = PS_VAR_FLOAT3( CodeChunkOperators::Lerp( diffuseGamma.xyz(), diffuseGammaDesat.xyz(), selectionEffects.x() ) );			
					diffuseGamma = PS_VAR_FLOAT3( CodeChunkOperators::Lerp( diffuseGamma.xyz(), selectionColor.xyz(), selectionColor.w() ) );
				}

				CodeChunk materialMaskEncoded8bit = CompileInput( compiler, CNAME( MaterialFlagsMask ), shaderTarget, MATERIAL_DEFAULT_MATERIAL_FLAGS_MASK_ENCODED ).x();
				outputDefined = compiler.GetPS().CompileDeferredShadingGBuffer( diffuseGamma, normal, vertex_normal, specularityGamma, roughness, translucency, ambientOcclusion, subsurfaceScattering, materialMaskEncoded8bit );

				if ( !outputDefined )
				{
					HALT( "Failed to generate deferred shading gbuffer" );
				}
			}
		}
		break;

	case RP_ForwardLightingSolid:
	case RP_ForwardLightingTransparent:
		{	
			const CodeChunk fallbackColor = Float3( 1.f, 0.f, 0.f );
			compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", Float4( fallbackColor, 1.0f ) );
			outputDefined = true;
		}
		break;

	case RP_RefractionDelta:
		{
			compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", MATERIAL_DEFAULT_REFRACTIONDELTA );
			outputDefined = true;
		}
		break;

	case RP_ReflectionMask:
		{
			compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", MATERIAL_DEFAULT_REFLECTIONMASK );
			outputDefined = true;
		}
		break;

	case RP_Emissive:
		{
			outputColor	= Float4 ( CompileInput( compiler, CNAME( Emissive ), shaderTarget, MATERIAL_DEFAULT_EMISSIVE).xyz(), 1.f );
		}
		break;

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

void CMaterialBlockOutputColorDeferred::GetValidPasses( TDynArray< ERenderingPass >& passes ) const
{
	// Defaults
	passes.PushBackUnique( RP_ShadowDepthSolid );	// we need this because we can have it dissolving
	passes.PushBackUnique( RP_ShadowDepthMasked );	// we need this for the masking
	passes.PushBackUnique( RP_HitProxies );			// hit proxy for everything
	passes.PushBackUnique( RP_HiResShadowMask );	// 

	// Supported
	passes.PushBackUnique( RP_GBuffer );
	if ( IsEmissive() )
	{
		passes.PushBack( RP_Emissive );
	}
	//passes.PushBackUnique( RP_NoLighting );		// for some reason we removed this
}

#endif
