/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbOutputColorSkin.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

#include "../matcompiler//materialShaderConstants.h"
#include "..\..\common\engine\mbParamTexture.h"
#include "..\engine\baseEngine.h"
#include "..\engine\renderFragment.h"
#include "..\engine\materialBlock.h"
#include "..\engine\graphConnectionRebuilder.h"

RED_DEFINE_STATIC_NAME( GlossinessSecond )
RED_DEFINE_STATIC_NAME( SpecularitySecond )
RED_DEFINE_STATIC_NAME( SubsurfaceScattering )
RED_DEFINE_STATIC_NAME( Data0 )
RED_DEFINE_STATIC_NAME( Data1 )
RED_DEFINE_STATIC_NAME( Data2 )
RED_DEFINE_STATIC_NAME( Data3 )
RED_DEFINE_STATIC_NAME( Data4 )
RED_DEFINE_STATIC_NAME( Data5 )
RED_DEFINE_STATIC_NAME( Data6 )

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutputColorSkin );

using namespace CodeChunkOperators;

CMaterialBlockOutputColorSkin::CMaterialBlockOutputColorSkin()
	: m_maskThreshold( 0.33f )
{
}

Bool CMaterialBlockOutputColorSkin::IsTwoSided() const
{
	return false;
}

Bool CMaterialBlockOutputColorSkin::IsTwoSideLighted() const
{
	return false;
}

ERenderingBlendMode CMaterialBlockOutputColorSkin::GetBlendMode() const
{
	return RBM_None; // deferred lighting always without blending.
}

Bool CMaterialBlockOutputColorSkin::IsEmissive() const
{
	return HasInput( CNAME( Emissive ) );
}


Bool CMaterialBlockOutputColorSkin::IsAccumulativelyRefracted() const
{
	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockOutputColorSkin::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

void CMaterialBlockOutputColorSkin::OnRebuildSockets()
{
	// Add sockets
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( AmbientOcclusion ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Diffuse ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Specularity ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Roughness ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Translucency ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Data0 ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Data1 ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Data2 ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Data3 ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Data4 ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Data5 ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Data6 ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Mask ) ) );
}

#endif

void CMaterialBlockOutputColorSkin::Compile( CMaterialBlockCompiler& compiler ) const
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
			CodeChunk uvDensityColor = PS_VAR_FLOAT3( Float3( 1.0f,1.0f,1.0f ) );
			// UV Density
			if ( compiler.GetMaterialCompiler()->GetContext().m_materialDebugMode == MDM_UVDensity && !compiler.IsTerrainShader() )
			{
				CodeChunk selectionEffects = compiler.GetPS().ConstReg( MDT_Float4, "PSC_SelectionEffect" );
				CodeChunk uv = PS_VAR_FLOAT2(selectionEffects.xy() * Float2(0.03125f,0.03125f) * PS_DATA( "TexCoords0" ).xy());

				CodeChunk param = compiler.GetPS().AutomaticName();
				CodeChunk call = CodeChunk::Printf( false, "float3 %s = CalcTextureDensityGrid( %s );", param.AsChar(), uv.AsChar() );
				compiler.GetPS().Statement( call );
				uvDensityColor = param;
			}

			{
				CodeChunk vertex_normal		= PS_VAR_FLOAT3( PS_DATA( "WorldNormal" ).xyz() );		
				CodeChunk normal			= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Normal ), shaderTarget,		Float4( Normalize( vertex_normal.xyz() ), 0.0f ) ));

				if ( compiler.GetMaterialCompiler()->GetContext().m_materialDebugMode == MDM_UVDensity && !compiler.IsTerrainShader() )
				{
					normal				= Float3( 0.f, 0.f, 1.f );
				}
				
				if ( MDM_FullGBuffer == compiler.GetMaterialCompiler()->GetContext().m_materialDebugMode )
				{
					CodeChunk diffuseGamma		= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Diffuse ), shaderTarget,		MATERIAL_DEFAULT_DIFFUSE_GBUFFER ).xyz() );
					CodeChunk specularityGamma	= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Specularity ), shaderTarget,	MATERIAL_DEFAULT_SPECULARITY ).xyz() );
					CodeChunk roughness			= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Roughness ), shaderTarget,   MATERIAL_DEFAULT_ROUGHNESS  ).x() );
					CodeChunk ambientOcclusion	= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( AmbientOcclusion ), shaderTarget,   MATERIAL_DEFAULT_AO   ).x() );

					CodeChunk subsurfaceScattering = PS_VAR_FLOAT( MATERIAL_DEFAULT_SS );

					CodeChunk translucency		= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Translucency ), shaderTarget, MATERIAL_DEFAULT_TRANSLUCENCY ).x() );

					outputDefined = compiler.GetPS().CompileDeferredShadingGBuffer( diffuseGamma, normal, vertex_normal, specularityGamma, roughness, translucency, ambientOcclusion, subsurfaceScattering );
				}
				else
				{
					outputDefined = compiler.GetPS().CompileDeferredShadingForwardGBuffer( normal );				
				}
			}

			if ( !outputDefined )
			{
				HALT( "Failed to generate deferred shading gbuffer" );
			}
		}
		break;

	//dex++: merged pass for skin should go here
	case RP_ForwardLightingSolid:
	case RP_ForwardLightingTransparent:
		{
			CodeChunk vertex_normal		= PS_VAR_FLOAT3( PS_DATA( "WorldNormal" ).xyz() );		
			CodeChunk vertex_pos		= PS_VAR_FLOAT3( PS_DATA( "WorldPosition" ).xyz() );
			CodeChunk diffuseGamma		= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Diffuse ), shaderTarget, MATERIAL_DEFAULT_DIFFUSE_GBUFFER ).xyz() );
			CodeChunk normal			= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Normal ), shaderTarget, Float4( Normalize( vertex_normal.xyz() ), 0.0f ) ));
			CodeChunk screenPos			= PS_VAR_FLOAT2( PS_DATA( "ScreenVPOS" ).xy() );
			CodeChunk specularityGamma	= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Specularity ), shaderTarget, MATERIAL_DEFAULT_SPECULARITY ).xyz() );
			CodeChunk roughness			= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Roughness ), shaderTarget,   MATERIAL_DEFAULT_ROUGHNESS  ).x() );
			CodeChunk ambientOcclusion	= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( AmbientOcclusion ), shaderTarget,   MATERIAL_DEFAULT_AO   ).x() );
			//CodeChunk subsurfaceScattering = PS_VAR_FLOAT(  CompileInput( compiler, CNAME( SubsurfaceScattering ), shaderTarget,   MATERIAL_DEFAULT_SS  ).x() );
			CodeChunk translucency		= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Translucency ), shaderTarget, MATERIAL_DEFAULT_TRANSLUCENCY ).x() );

			diffuseGamma		= Saturate( diffuseGamma );
			specularityGamma	= Saturate( specularityGamma );
			roughness			= Saturate( roughness );
			ambientOcclusion	= Saturate( ambientOcclusion );
			translucency		= Saturate( translucency );

			CodeChunk diffuse = CodeChunkOperators::Pow( diffuseGamma, 2.2f );
			CodeChunk specularity = CodeChunkOperators::Pow( specularityGamma, 2.2f );

			// include some utils functions			
			compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );

			// build extra lighting params
			compiler.GetPS().Statement( CodeChunk( "#ifdef __PSSL__" ) );
			compiler.GetPS().Statement( CodeChunk( "SCompileInLightingParams extraLightingParams;" ) );
			compiler.GetPS().Statement( CodeChunk( "#else" ) );
			compiler.GetPS().Statement( CodeChunk( "SCompileInLightingParams extraLightingParams = (SCompileInLightingParams)0;" ) );
			compiler.GetPS().Statement( CodeChunk( "#endif" ) );
			{
				compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Macro( "#define COMPILE_IN_SHADING_SKIN" );

				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.vertexNormal = %s;", vertex_normal.AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.data0 = %s;", PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Data0 ), shaderTarget, Vector::ZEROS ) ).AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.data1 = %s;", PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Data1 ), shaderTarget, Vector::ZEROS ) ).AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.data2 = %s;", PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Data2 ), shaderTarget, Vector::ZEROS ) ).AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.data3 = %s;", PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Data3 ), shaderTarget, Vector::ZEROS ) ).AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.data4 = %s;", PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Data4 ), shaderTarget, Vector::ZEROS ) ).AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.data5 = %s;", PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Data5 ), shaderTarget, Vector::ZEROS ) ).AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.data6 = %s;", PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Data6 ), shaderTarget, Vector::ZEROS ) ).AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.ao = %s;", ambientOcclusion.AsChar() ) );
			}

			// forward lighting calculation
			CodeChunk param = compiler.GetPS().AutomaticName();
			CodeChunk call = CodeChunk::Printf( false, "float3 %s = CalculateLightingPBRPipeline( %s, %s, %s, %s, %s, %s, %s, %s, extraLightingParams, true, true, false );", 
				param.AsChar(), 
				vertex_pos.AsChar(),
				diffuse.AsChar(),
				normal.AsChar(),
				vertex_normal.AsChar(),
				specularity.AsChar(),
				roughness.AsChar(),
				translucency.AsChar(),
				screenPos.AsChar() );
			compiler.GetPS().Statement( call );

			// 
			compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", Float4( param.xyz(), 1.f ) );	
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

void CMaterialBlockOutputColorSkin::GetValidPasses( TDynArray< ERenderingPass >& passes ) const
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
	passes.PushBackUnique( RP_NoLighting );
	passes.PushBackUnique( RP_ForwardLightingSolid );
	passes.PushBackUnique( RP_ForwardLightingTransparent );
}

#endif
