/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbOutputColorHair.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

#include "../matcompiler//materialShaderConstants.h"
#include "..\..\common\engine\mbParamTexture.h"
#include "..\engine\baseEngine.h"
#include "..\engine\renderFragment.h"
#include "..\engine\materialBlock.h"

RED_DEFINE_STATIC_NAME( SubsurfaceScattering )
RED_DEFINE_STATIC_NAME( Data0 )
RED_DEFINE_STATIC_NAME( Data1 )
RED_DEFINE_STATIC_NAME( Data2 )

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutputColorHair );

using namespace CodeChunkOperators;

CodeChunk CompileGlobalFogData( CMaterialBlockCompiler& compiler, Bool isSky, Bool isClouds, Bool vertexBased, Bool calcFullData );

CodeChunk CompileVertexBasedShadowing( CMaterialBlockCompiler& compiler, Bool includeCascadesShadow, CodeChunk *outVertexInteriorFactor, CodeChunk *outVertexWeight, CodeChunk *outPixelWeight )
{
	compiler.GetMaterialCompiler()->GetVertexShaderCompiler()->Macro( "#define COMPILE_IN_SHADING_HAIR" );

	compiler.GetMaterialCompiler()->GetVertexShaderCompiler()->Include( "include_constants.fx" );

	CodeChunk worldSpacePosition = compiler.GetVS().Var( MDT_Float3, compiler.GetVS().Data( "WorldPosition" ).xyz() );
	
	CodeChunk screenPos = compiler.GetVS().AutomaticName();
	compiler.GetVS().Statement( CodeChunk::Printf( false, "float4 %s = mul( projectionMatrix, mul( worldToView, float4( %s, 1 ) ) );", screenPos.AsChar(), worldSpacePosition.xyz().AsChar() ) );
	
	CodeChunk hpos = compiler.GetVS().Var( MDT_Float3, screenPos.xyz() / Max( CodeChunk(0.0001f), screenPos.w() ) );
	CodeChunk pixelPos = compiler.GetVS().Var( MDT_Float2, (hpos.xy() * Float2( 0.5f, -0.5f ) + 0.5f) * CodeChunk( "screenDimensions.xy" ) );	

	CodeChunk shadowFactorVS = compiler.GetVS().AutomaticName();
	CodeChunk dimmerAndInteriorFactor = compiler.GetVS().AutomaticName();
	CodeChunk weight = compiler.GetVS().AutomaticName();
	
	compiler.GetVS().Statement( CodeChunk::Printf( false, "float %s = CalcGlobalShadow( 0, %s, 0, 0, false, %s );", shadowFactorVS.AsChar(), worldSpacePosition.xyz().AsChar(), (includeCascadesShadow ? "true" : "false") ) );
	compiler.GetVS().Statement( CodeChunk::Printf( false, "float2 %s = CalcDimmersFactorAndInteriorFactorTransparency( %s, %s );", dimmerAndInteriorFactor.AsChar(), worldSpacePosition.xyz().AsChar(), pixelPos.AsChar() ) );
	compiler.GetVS().Statement( CodeChunk::Printf( false, "float %s = (%s) > 0 ? saturate( (1 - max( abs(%s.x), abs(%s.y))) / 0.1 ) : 0;", weight.AsChar(), screenPos.w().AsChar(), hpos.AsChar(), hpos.AsChar() ) );

	CodeChunk vsResult = Float4( shadowFactorVS, dimmerAndInteriorFactor.x(), dimmerAndInteriorFactor.y(), Float(1.f) ) * Float4 ( weight, weight, weight, weight );

	CodeChunk psResult = compiler.GetPS().Var( MDT_Float4, 0.0f );
	compiler.GetMaterialCompiler()->Connect( MDT_Float4, vsResult, psResult, CodeChunk::Printf(true, "Shadowing" ) );

	if ( outVertexInteriorFactor )
	{
		*outVertexInteriorFactor = dimmerAndInteriorFactor.y();
	}

	if ( outVertexWeight )
	{
		*outVertexWeight = weight;
	}

	if ( outPixelWeight )
	{
		*outPixelWeight = psResult.w();
	}

	return psResult;
}

void CompileVertexBasedEnvProbes( CMaterialBlockCompiler& compiler, const CodeChunk &vertexInteriorFactor, const CodeChunk &vertexWeight, const CodeChunk &pixelWeight, CodeChunk &outAmbient, CodeChunk &outReflection )
{
	compiler.GetMaterialCompiler()->GetVertexShaderCompiler()->Include( "include_constants.fx" );

	CodeChunk worldSpacePosition = compiler.GetVS().Var( MDT_Float3, compiler.GetVS().Data( "WorldPosition" ).xyz() );
	CodeChunk worldSpaceNormal = compiler.GetVS().Var( MDT_Float3, compiler.GetVS().Data( "WorldNormal" ).xyz() );
	CodeChunk cameraPosition = compiler.GetVS().ConstReg( MDT_Float4, "VSC_CameraPosition" ).xyz();
	CodeChunk worldSpaceReflected = Reflect( (worldSpacePosition - cameraPosition).Normalize(), worldSpaceNormal );
	
	CodeChunk ambientVS = compiler.GetVS().Var( MDT_Float3, 0.0f );
	CodeChunk reflectionVS = compiler.GetVS().Var( MDT_Float3, 0.0f );

	compiler.GetVS().Statement( CodeChunk::Printf( false, "CalcEnvProbes_MipLod( %s, %s, %s, %s, %s, AMBIENT_VERTEX_BASED_MIP_INDEX, REFLECTION_VERTEX_BASED_MIP_INDEX, 0, true, true, true, %s );", ambientVS.AsChar(), reflectionVS.AsChar(), worldSpacePosition.AsChar(), worldSpaceNormal.AsChar(), worldSpaceReflected.AsChar(), vertexInteriorFactor.AsChar() ) );
	
	ambientVS = compiler.GetVS().Var( MDT_Float3, ambientVS * vertexWeight );
	reflectionVS = compiler.GetVS().Var( MDT_Float3, reflectionVS * vertexWeight );

	CodeChunk tempAmbientPS = compiler.GetPS().Var( MDT_Float3, 0.0f );
	CodeChunk tempReflectionPS = compiler.GetPS().Var( MDT_Float3, 0.0f );
	compiler.GetMaterialCompiler()->Connect( MDT_Float3, ambientVS, tempAmbientPS, CodeChunk::Printf(true, "EnvProbeAmbient" ) );
	compiler.GetMaterialCompiler()->Connect( MDT_Float3, reflectionVS, tempReflectionPS, CodeChunk::Printf(true, "EnvProbeReflection" ) );

	outAmbient = compiler.GetPS().Var( MDT_Float3, tempAmbientPS / Max( CodeChunk(0.001f), pixelWeight ) );
	outReflection = compiler.GetPS().Var( MDT_Float3, tempReflectionPS / Max( CodeChunk(0.001f), pixelWeight ) );
}

CMaterialBlockOutputColorHair::CMaterialBlockOutputColorHair()
	: m_isTwoSided( false )
	, m_rawOutput( false )
	, m_maskThreshold( 0.33f )
	, m_implicitGlobalFogVertexBased( false )
	, m_shadowingSolidVertexBased( false )
	, m_shadowingTransparentVertexBased( false )
	, m_shadowingCascadesVertexBased( false )
	, m_envProbesSolidVertexBased( false )
	, m_envProbesTransparentVertexBased( false )
{
}

Bool CMaterialBlockOutputColorHair::IsTwoSided() const
{
	return m_isTwoSided;
}

ERenderingBlendMode CMaterialBlockOutputColorHair::GetBlendMode() const
{
	return RBM_None; // deferred lighting always without blending.
}

Bool CMaterialBlockOutputColorHair::IsEmissive() const
{
	return false;
}

Bool CMaterialBlockOutputColorHair::IsAccumulativelyRefracted() const
{
	return false;
}

RED_DEFINE_STATIC_NAME( NoiseMap );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockOutputColorHair::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

void CMaterialBlockOutputColorHair::OnRebuildSockets()
{
	// Add sockets
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( AmbientOcclusion ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Anisotropy ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Diffuse ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Normal ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Specularity ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Roughness ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Translucency ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( NoiseMap ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Data0 ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Data1 ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Data2 ) ) );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Mask ) ) );
}

#endif

void CMaterialBlockOutputColorHair::Compile( CMaterialBlockCompiler& compiler ) const
{
	const ERenderingPass pass = compiler.GetMaterialCompiler()->GetContext().m_renderingContext->m_pass;

	EMaterialShaderTarget shaderTarget = MSH_PixelShader;

	// Mask 

	CodeChunk mask = Float4( 1.0f, 1.0f, 1.0f, 1.0f );

	if ( HasInput( CNAME( Mask ) ) ) 
	{
		mask = PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Mask ), shaderTarget, MATERIAL_DEFAULT_MASK ) );
	}

	// Optional clipping
	// We don't include masking in the OptionalFragmentClipping, because it's handled separately always (not a togglable thing)

	CodeChunk forwardLightingBranchCondition = CodeChunk::Printf( false, "Forced compilation error" );
	if ( pass == RP_ForwardLightingTransparent )
	{
		CodeChunk maskOrig = mask;
		mask = PS_VAR_FLOAT4( Saturate( mask / Max( 0.001f, m_maskThreshold ) ) );

		forwardLightingBranchCondition = CodeChunk::Printf( false, "if ( %s > (0.75 / 255.0) && (%s - %s) < 0.f ) {", mask.x().AsChar(), maskOrig.x().AsChar(), CodeChunk(m_maskThreshold).x().AsChar() );
		//forwardLightingBranchCondition = CodeChunk::Printf( false, "if ( %s > 0 ) {", mask.x().AsChar() );
		
		compiler.GetPS().CompileOptionalFragmentClipping( compiler.GetMaterialCompiler()->GetContext(), CodeChunk() );
	}
	else if ( pass == RP_ForwardLightingSolid )
	{
		// no discard because we're rendering hair with Equal z test

		forwardLightingBranchCondition = CodeChunk::Printf( false, "if ( (%s - %s) >= 0.f ) {", mask.x().AsChar(), CodeChunk(m_maskThreshold).x().AsChar() );

		CodeChunk clipValue = mask.x() - m_maskThreshold;

		compiler.GetPS().Discard( clipValue );

		compiler.GetPS().CompileOptionalFragmentClipping( compiler.GetMaterialCompiler()->GetContext(), CodeChunk() );
	}
	else
	{
		CodeChunk clipValue = mask.x() - m_maskThreshold;

		compiler.GetPS().Discard( clipValue );

		compiler.GetPS().CompileOptionalFragmentClipping( compiler.GetMaterialCompiler()->GetContext(), CodeChunk() );
	}

	// Compile material color for given pass
	
	bool		outputDefined		 = false;
	CodeChunk	outputColor			 = MATERIAL_DEFAULT_OUTPUT_COLOR;
	Bool		useSSAO =				true;

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
				HALT( "Simple pass not recognized, or failed to compile" );
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

	//dex++: restored no-lighting mode
	case RP_NoLighting:
	{
		outputColor = Float4( CompileInput( compiler, CNAME( Diffuse ), shaderTarget,		MATERIAL_DEFAULT_DIFFUSE_GBUFFER ).xyz(), 1.0f );
		break;
	}
	//dex--

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

				normal = compiler.GetPS().PrepareForTwoSidedLighting( normal, vertex_normal );
				
				// UV Density
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

					CodeChunk subsurfaceScattering = PS_VAR_FLOAT(  CompileInput( compiler, CNAME( SubsurfaceScattering ), shaderTarget,   MATERIAL_DEFAULT_SS  ).x() );

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
	
	//dex++: hair was rendered in two parts - solid and transparent		
	case RP_ForwardLightingTransparent:		
			// choppin transparent pass - performance reasons
			compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Macro( "#define COMPILE_IN_SHADING_HAIR_TRANSPARENT" );
			useSSAO = false;

			// no break; intentionally 		
	case RP_ForwardLightingSolid:
		{
			CodeChunk vertex_normal		= PS_VAR_FLOAT3( PS_DATA( "WorldNormal" ).xyz() );		
			CodeChunk vertex_pos		= PS_VAR_FLOAT3( PS_DATA( "WorldPosition" ).xyz() );
			CodeChunk diffuseGamma		= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Diffuse ), shaderTarget, MATERIAL_DEFAULT_DIFFUSE_GBUFFER ).xyz() );
			CodeChunk normal			= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Normal ), shaderTarget, Float4( Normalize( vertex_normal.xyz() ), 0.0f ) ));
			CodeChunk screenPos			= PS_VAR_FLOAT2( PS_DATA( "ScreenVPOS" ).xy() );

			normal = compiler.GetPS().PrepareForTwoSidedLighting( normal, vertex_normal );
			
			CodeChunk specularityGamma	= PS_VAR_FLOAT3( CompileInput( compiler, CNAME( Specularity ), shaderTarget, MATERIAL_DEFAULT_SPECULARITY ).xyz() );
			CodeChunk roughness			= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Roughness ), shaderTarget,   MATERIAL_DEFAULT_ROUGHNESS  ).x() );
			//CodeChunk ambientOcclusion	= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( AmbientOcclusion ), shaderTarget,   MATERIAL_DEFAULT_AO   ).x() );
			//CodeChunk subsurfaceScattering = PS_VAR_FLOAT(  CompileInput( compiler, CNAME( SubsurfaceScattering ), shaderTarget,   MATERIAL_DEFAULT_SS  ).x() );
			CodeChunk translucency		= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Translucency ), shaderTarget, MATERIAL_DEFAULT_TRANSLUCENCY ).x() );

			diffuseGamma		= Saturate( diffuseGamma );
			specularityGamma	= Saturate( specularityGamma );
			roughness			= Saturate( roughness );
			translucency		= Saturate( translucency );

			CodeChunk diffuse = CodeChunkOperators::Pow( diffuseGamma, 2.2f );
			CodeChunk specularity = CodeChunkOperators::Pow( specularityGamma, 2.2f );

			// include some utils functions			
			compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Include( "include_constants.fx" );

			CodeChunk envProbeMipIndex = compiler.GetPS().AutomaticName();
			compiler.GetPS().Statement( CodeChunk::Printf( false, "float %s = PrecomputeCalculateLightingEnvProbeMipIndex( %s, %s, %s );", envProbeMipIndex.AsChar(), vertex_pos.AsChar(), normal.AsChar(), screenPos.AsChar() ) );
			
			// build extra lighting params
			compiler.GetPS().Statement( CodeChunk( "#ifdef __PSSL__" ) );
			compiler.GetPS().Statement( CodeChunk( "SCompileInLightingParams extraLightingParams;" ) );
			compiler.GetPS().Statement( CodeChunk( "#else" ) );
			compiler.GetPS().Statement( CodeChunk( "SCompileInLightingParams extraLightingParams = (SCompileInLightingParams)0;" ) );
			compiler.GetPS().Statement( CodeChunk( "#endif" ) );
			//if ( HasInput( CNAME( Anisotropy ) ) )
			{
				CodeChunk anisotropy		= PS_VAR_FLOAT(  CompileInput( compiler, CNAME( Anisotropy ), shaderTarget, Vector::ZEROS ).x() );
				CodeChunk vertex_tangent	= PS_VAR_FLOAT3( PS_DATA( "WorldTangent" ).xyz() );		
				CodeChunk vertex_binormal	= PS_VAR_FLOAT3( PS_DATA( "WorldBinormal" ).xyz() );		

				compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Macro( "#define COMPILE_IN_SHADING_HAIR" );

				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.vertexNormal = %s;", vertex_normal.AsChar() ) );

				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.anisotropy = saturate( %s );", anisotropy.AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.tangent = %s;", vertex_tangent.AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.bitangent = %s;", vertex_binormal.AsChar() ) );

				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.data0 = %s;", PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Data0 ), shaderTarget, Vector::ZEROS ) ).AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.data1 = %s;", PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Data1 ), shaderTarget, Vector::ZEROS ) ).AsChar() ) );
				compiler.GetPS().Statement( CodeChunk::Printf( false, "extraLightingParams.data2 = %s;", PS_VAR_FLOAT4( CompileInput( compiler, CNAME( Data2 ), shaderTarget, Vector::ZEROS ) ).AsChar() ) );
			}

			//
			compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Macro( "#define CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_ENVPROBE_LOD" );

			CodeChunk applyGlobalFog ( RP_ForwardLightingSolid == pass ? "false" : "true" );

			Bool useCustomFogCalculations = false;
			// use vertices for fog calculations
			if( m_implicitGlobalFogVertexBased && RP_ForwardLightingTransparent == pass )
			{
				applyGlobalFog = "false";
				useCustomFogCalculations = true;
			}

			Bool useVertexShadowing = false;
			CodeChunk useShadowBuffer = (Uint32)( pass != RP_ForwardLightingTransparent ? 1 : 0);
			if( (m_shadowingTransparentVertexBased && RP_ForwardLightingTransparent == pass) || ( m_shadowingSolidVertexBased && RP_ForwardLightingSolid == pass ) )
			{
				if ( RP_ForwardLightingTransparent == pass )
				{
					compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Macro( "#define CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_SF_ALLOW_DISCARD" );
				}

				useShadowBuffer = "false";
				useVertexShadowing = true;
			}
			
			// forward lighting calculation
			CodeChunk param = compiler.GetPS().AutomaticName();
			compiler.GetPS().Statement( CodeChunk::Printf( false, "float3 %s = 0;", param.AsChar() ) );
			compiler.GetPS().Statement( "[branch]" );
			compiler.GetPS().Statement( forwardLightingBranchCondition );

			CodeChunk useSSAOStatement = useSSAO ? "true" : "false";

			CodeChunk call;
			if ( useVertexShadowing )
			{
				compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Macro( "#define CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_SHADOW_FACTOR" );
			
				CodeChunk vertexWeight;
				CodeChunk vertexInteriorFactor;
				CodeChunk pixelWeight;
				CodeChunk shadowing = CompileVertexBasedShadowing( compiler, m_shadowingCascadesVertexBased, &vertexInteriorFactor, &vertexWeight, &pixelWeight );

				if ( !m_shadowingCascadesVertexBased )
				{
					CodeChunk cascadesShadow = compiler.GetPS().AutomaticName();
					compiler.GetPS().Statement( CodeChunk::Printf( false, "float %s = CalcShadowFactor( %s, false, 0, false );", cascadesShadow.AsChar(), vertex_pos.xyz().AsChar() ) );					
					shadowing = compiler.GetPS().Var( MDT_Float4, shadowing * Float4( cascadesShadow, 1.f, 1.f, 1.f ) );
				}
								
				if( ( m_envProbesTransparentVertexBased && RP_ForwardLightingTransparent == pass ) || ( m_envProbesSolidVertexBased && RP_ForwardLightingSolid == pass ) )
				{
					compiler.GetMaterialCompiler()->GetPixelShaderCompiler()->Macro( "#define CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_ENVPROBES" );

					CodeChunk envProbeAmbient, envProbeReflection;
					CompileVertexBasedEnvProbes( compiler, vertexInteriorFactor, vertexWeight, pixelWeight, envProbeAmbient, envProbeReflection );

					call = CodeChunk::Printf( false, "%s = CalculateLightingPBRPipeline( %s, %s, %s, %s, %s, %s, %s, %s, extraLightingParams, %s, %s, %s, %s, %s, %s, %s );", 
						param.AsChar(), 
						vertex_pos.AsChar(),
						diffuse.AsChar(),
						normal.AsChar(),
						vertex_normal.AsChar(),
						specularity.AsChar(),
						roughness.AsChar(),
						translucency.AsChar(),
						screenPos.AsChar(),
						useSSAOStatement.AsChar(),
						useShadowBuffer.AsChar(),
						envProbeMipIndex.AsChar(),
						applyGlobalFog.AsChar(),
						shadowing.AsChar(),
						envProbeAmbient.AsChar(),
						envProbeReflection.AsChar() );
				}
				else
				{
					call = CodeChunk::Printf( false, "%s = CalculateLightingPBRPipeline( %s, %s, %s, %s, %s, %s, %s, %s, extraLightingParams, %s, %s, %s, %s, %s );", 
						param.AsChar(), 
						vertex_pos.AsChar(),
						diffuse.AsChar(),
						normal.AsChar(),
						vertex_normal.AsChar(),
						specularity.AsChar(),
						roughness.AsChar(),
						translucency.AsChar(),
						screenPos.AsChar(),
						useSSAOStatement.AsChar(),
						useShadowBuffer.AsChar(),
						envProbeMipIndex.AsChar(),
						applyGlobalFog.AsChar(),
						shadowing.AsChar() );
				}
			}
			else
			{
				call = CodeChunk::Printf( false, "%s = CalculateLightingPBRPipeline( %s, %s, %s, %s, %s, %s, %s, %s, extraLightingParams, %s, %s, %s, %s );", 
					param.AsChar(), 
					vertex_pos.AsChar(),
					diffuse.AsChar(),
					normal.AsChar(),
					vertex_normal.AsChar(),
					specularity.AsChar(),
					roughness.AsChar(),
					translucency.AsChar(),
					screenPos.AsChar(),
					useSSAOStatement.AsChar(),
					useShadowBuffer.AsChar(),
					envProbeMipIndex.AsChar(),
					applyGlobalFog.AsChar() );
			}

			compiler.GetPS().Statement( call );
			compiler.GetPS().Statement( "}" );

			if( useCustomFogCalculations )
			{
				CodeChunk fogResult = compiler.GetPS().AutomaticName();
				CodeChunk fogData = CompileGlobalFogData( compiler, false, false, true, true );
				CodeChunk call = CodeChunk::Printf( false, "float4 %s = ApplyFog_AlphaBlend( %s, %s, %s );", fogResult.AsChar(), param.xyz().AsChar(), mask.x().AsChar(), fogData.AsChar() );
				compiler.GetPS().Statement( call );

				param = fogResult;
				mask = fogResult.wwww();
			}

			// for transparent pass premultiply the color
			if ( pass == RP_ForwardLightingTransparent )
			{
				compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", Float4( param.xyz() * mask.x(), mask.x() ) );	
			}
			else
			{
				compiler.GetPS().Output( MDT_Float4, "SYS_TARGET_OUTPUT0", Float4( param.xyz(), mask.x() ) );	
			}
			outputDefined = true;
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

void CMaterialBlockOutputColorHair::GetValidPasses( TDynArray< ERenderingPass >& passes ) const
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
