/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/renderFragment.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

CHLSLMaterialPixelShaderCompiler::CHLSLMaterialPixelShaderCompiler( CHLSLMaterialCompiler* compiler, CHLSLMaterialVertexShaderCompiler* vertexShader )
	: CHLSLMaterialShaderCompiler( compiler, 0, 0, 0, 0)
	, m_vertexShader( vertexShader )	
{
}

Bool CHLSLMaterialPixelShaderCompiler::GenerateData( const CodeChunk& semantic, CodeChunk& output )
{
	const MaterialRenderingContext& context = m_compiler->GetContext();

	// Base texture coordinates
	if ( semantic == "TexCoords0" )
	{
		CodeChunk vsData = m_vertexShader->Data( semantic );
		CodeChunk psData = Var( MDT_Float2, 0.0f );
		m_compiler->Connect( MDT_Float2, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Second texture coordinates
	if ( semantic == "TexCoords1" )
	{
		CodeChunk vsData = m_vertexShader->Data( semantic );
		CodeChunk psData = Var( MDT_Float2, 0.0f );
		m_compiler->Connect( MDT_Float2, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Projected texture coordinates
	if ( semantic == "TexCoords2" )
	{
		CodeChunk vsData = m_vertexShader->Data( semantic );
		CodeChunk psData = Var( MDT_Float4, 0.0f );
		m_compiler->Connect( MDT_Float4, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Vertex position
	if ( semantic == "VertexPosition" )
	{
		CodeChunk vsData = m_vertexShader->Data( semantic );
		CodeChunk psData = Var( MDT_Float3, 0.0f );
		m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Vertex normal
	if ( semantic == "VertexNormal" )
	{
		CodeChunk vsData = m_vertexShader->Data( semantic );
		CodeChunk psData = Var( MDT_Float3, 0.0f );
		m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Vertex color
	if ( semantic == "VertexColor" )
	{
		CodeChunk vsData = m_vertexShader->Data( semantic );
		CodeChunk psData = Var( MDT_Float4, 0.0f );
		m_compiler->Connect( MDT_Float4, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Vertex world space position
	if ( semantic == "WorldPosition" )
	{
		CodeChunk vsData = m_vertexShader->Data( semantic );
		CodeChunk psData = Var( MDT_Float3, 0.0f );
		m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Vertex view space position
	if ( semantic == "ViewPosition" )
	{
		CodeChunk vsData = m_vertexShader->Data( semantic );
		CodeChunk psData = Var( MDT_Float3, 0.0f );
		m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Vertex world space normal
	if ( semantic == "WorldNormal" )
	{
		if ( ( context.m_vertexFactory == MVF_TesselatedTerrain || context.m_vertexFactory == MVF_TerrainSkirt )
			&& context.m_pregeneratedMaps && !context.m_renderingContext->m_terrainToolStampVisible )
		{
			// Terrain with pregenerated maps doesn't use vertex normals.
			output = Float3( 0.0f, 0.0f, 0.0f );
		}
		else if ( context.m_vertexFactory == MVF_Terrain && !context.m_renderingContext->m_terrainToolStampVisible )
		{
			CodeChunk uv = Data( "TexCoords0" );
			CodeChunk psData = Var( MDT_Float3, CodeChunk::Printf( false, "CalcTerrainNormal( %s )", uv.AsChar() ) );
			output = psData;
		}
		else
		{
			CodeChunk vsData = m_vertexShader->Data( semantic );
			CodeChunk psData = Var( MDT_Float3, 0.0f );
			m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
			output = psData;
		}

		return true;
	}

	if ( semantic == "FrontFacing" )
	{
		CodeChunk vFace = Input( MDT_Float, "vFace" );	
		output = vFace;
		return true;
	}

	if ( semantic == "WorldNormalFacingCamera" )
	{	
		//almost the same as above, with one exception- if WorldNormal goes away from camera, it will be flipped
		CodeChunk worldNormal;
		GenerateData( "WorldNormal", worldNormal );
		
		CodeChunk vFace;
		GenerateData("FrontFacing", vFace );
		
		output = Float3( worldNormal.x() * vFace.x(), worldNormal.y() * vFace.x(), worldNormal.z() * vFace.x() );
		return true;
	}

	// Vertex world space tangent
	if ( semantic == "WorldTangent" )
	{
		if ( context.m_vertexFactory == MVF_Terrain && !context.m_renderingContext->m_terrainToolStampVisible )
		{
			CodeChunk normal = Data( "WorldNormal" );
			CodeChunk psData = Var( MDT_Float3, CodeChunk::Printf( false, "CalcTerrainTangent( %s )", normal.AsChar() ) );
			output = psData;
		}
		else
		{
			CodeChunk vsData = m_vertexShader->Data( "WorldTangent" );
			CodeChunk psData = Var( MDT_Float3, 0.0f );
			m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
			output = psData;
		}

		return true;
	}

	// Vertex world space binormal
	if ( semantic == "WorldBinormal" )
	{
		if ( context.m_vertexFactory == MVF_Terrain && !context.m_renderingContext->m_terrainToolStampVisible )
		{
			CodeChunk normal = Data( "WorldNormal" );
			CodeChunk psData = Var( MDT_Float3, CodeChunk::Printf( false, "CalcTerrainBinormal( %s )", normal.AsChar() ) );
			output = psData;
		}
		else
		{
			CodeChunk vsData = m_vertexShader->Data( "WorldBinormal" );
			CodeChunk psData = Var( MDT_Float3, 0.0f );
			m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
			output = psData;
		}

		return true;
	}

	// Vertex world space binormal
	if ( semantic == "TBNMatrix" )
	{
		CodeChunk worldNormal;
		GenerateData( "WorldNormal", worldNormal );

		CodeChunk worldTangent;
		GenerateData( "WorldTangent", worldTangent );

		CodeChunk worldBinormal;
		GenerateData( "WorldBinormal", worldBinormal );

		CodeChunk psData = Var( MDT_Float3x3, CodeChunk::Printf( false, "float3x3( %s, %s, %s )", worldTangent.AsChar(), worldBinormal.AsChar(), worldNormal.AsChar() ) );
		output = psData;

		return true;
	}

	// Light space position
	if ( semantic == "LightPosition" )
	{
		CodeChunk vsData = m_vertexShader->Data( "LightPosition" );
		CodeChunk psData = Var( MDT_Float3, 0.0f );
		m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Screen-space W of a pixel
	if ( semantic == "ScreenW" )
	{
		CodeChunk vsData = m_vertexShader->Data( "ScreenW" );
		CodeChunk psData = Var( MDT_Float, 0.0f );
		m_compiler->Connect( MDT_Float, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Screen-space Z of a pixel
	if ( semantic == "ScreenZ" )
	{
		CodeChunk vsData = m_vertexShader->Data( "ScreenZ" );
		CodeChunk psData = Var( MDT_Float, 0.0f );
		m_compiler->Connect( MDT_Float, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// View-space W of a pixel
	if ( semantic == "ViewW" )
	{
		CodeChunk vsData = m_vertexShader->Data( "ViewW" );
		CodeChunk psData = Var( MDT_Float, 0.0f );
		m_compiler->Connect( MDT_Float, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// View-space Z of a pixel
	if ( semantic == "ViewZ" )
	{
		CodeChunk vsData = m_vertexShader->Data( "ViewZ" );
		CodeChunk psData = Var( MDT_Float, 0.0f );
		m_compiler->Connect( MDT_Float, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Screen space UV
	if ( semantic == "ScreenUV" )
	{
		CodeChunk viewportSize = ConstReg( MDT_Float4, "PSC_ViewportSize" );
		CodeChunk vpos = Input( MDT_Float4, "SYS_POSITION" );
#ifndef USE_HALF_PIXEL_OFFSET
		output = vpos.xy() * viewportSize.zw();
#else
		output = vpos.xy() * viewportSize.zw() + 0.5f * viewportSize.zw();
#endif
		return true;
	}

	// EnvTranspColorFilter
	if ( semantic == "EnvTranspColorFilter" )
	{
		CodeChunk colorDistMin	= ConstReg( MDT_Float4, "PSC_EnvTranspFilterDistMinColor" );
		CodeChunk colorDistMax	= ConstReg( MDT_Float4, "PSC_EnvTranspFilterDistMaxColor" );
		CodeChunk params		= ConstReg( MDT_Float4, "PSC_EnvTranspFilterParams" );
		CodeChunk dist			= Data( "ViewZ" ).x();
		
		CodeChunk t	= Var( MDT_Float, Saturate( dist * params.x() ) );
		t			= t * t;
		output		= Var( MDT_Float4, Lerp( colorDistMin, colorDistMax, t ) );
		return true;
	}

	// Transparency fadeout alpha
	if ( semantic == "TranspColorScale" )
	{
		CodeChunk params = ConstReg( MDT_Float4, "PSC_TransparencyParams" );
		output = Float4 ( params.xyz(), 1.f );
		return true;
	}

	// Transparency fadeout alpha
	if ( semantic == "TranspFadeOutAlpha" )
	{
		output = ConstReg( MDT_Float4, "PSC_TransparencyParams" ).w();
		return true;
	}

	// ScreenVPOS
	if ( semantic == "ScreenVPOS" )
	{
		CodeChunk vpos = Input( MDT_Float4, "SYS_POSITION" );
		output = vpos.xy();
		return true;
	}

	// FaceSide
	if ( semantic == "FadeSide" )
	{
		CodeChunk vface = Input( MDT_Uint, "SYS_FRONT_FACE" );
		output = vface;
		return true;
	}

	// Distance to camera
	if ( semantic == "CameraDistance" )
	{
		CodeChunk cameraPos = ConstReg( MDT_Float3, "PSC_CameraPosition" );
		CodeChunk vertexPos = Data( "WorldPosition" );
		output = Var( MDT_Float, Length( cameraPos.xyz() - vertexPos.xyz() ) );
		return true;
	}

	// Animation frame
	if ( semantic == "AnimationFrame" )
	{
		CodeChunk vsData = m_vertexShader->Data( "AnimationFrame" );
		CodeChunk psData = Var( MDT_Float, 0.0f );
		m_compiler->Connect( MDT_Float, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Motion blend ratio
	if ( semantic == "MotionBlend" )
	{
		CodeChunk vsData = m_vertexShader->Data( "MotionBlend" );
		CodeChunk psData = Var( MDT_Float, 0.0f );
		m_compiler->Connect( MDT_Float, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Dissolve blend ratio
	if ( semantic == "DissolveParams" )
	{	
		CodeChunk vsData = m_vertexShader->Data( "DissolveParams" );
		CodeChunk psData = Var( MDT_Float4, 0.0f );
		m_compiler->Connect( MDT_Float4, vsData, psData, semantic );
		output = psData;
		return true;
	}

	// Foliage color
	if ( semantic == "FoliageColor" )
	{	
		CodeChunk vsData = m_vertexShader->Data( "FoliageColor" );
		CodeChunk psData = Var( MDT_Float3, 0.0f );
		m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
		output = psData;
		return true;
	}
	
	// Texture index
	if ( semantic == "TexIndex" )
	{
		CodeChunk vsData = m_vertexShader->Data( "TexIndex" );
		CodeChunk psData = Var( MDT_Int4, 0.0f );
		m_compiler->Connect( MDT_Int4, vsData, psData, semantic );
		output = psData;
		return true;
	}

	if ( semantic == "TexCoords_Dissolve" )
	{
		CodeChunk vsData = m_vertexShader->Data( "TexCoords_Dissolve" );
		CodeChunk psData = Var( MDT_Float2, 0.0f );
		m_compiler->Connect( MDT_Float2, vsData, psData, semantic );
		output = psData;
		return true;
	}

	if ( semantic == "ClippingEllipsePosition" )
	{
		CodeChunk vsData = m_vertexShader->Data( "ClippingEllipsePosition" );
		CodeChunk psData = Var( MDT_Float3, 0.0f );
		m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
		output = psData;
		return true;
	}
	
	if ( semantic == "SkeletalExtraData" )
	{
		CodeChunk vsData = m_vertexShader->Data( "SkeletalExtraData" );
		CodeChunk psData = Var( MDT_Float3, 0.0f );
		m_compiler->Connect( MDT_Float3, vsData, psData, semantic );
		output = Float4( psData, 0.0f );
		return true;
	}	

	// Not defined
	return false;
}

void CHLSLMaterialPixelShaderCompiler::Discard( const CodeChunk& value )
{
	m_code.Print( "clip( (%s).x );", value.AsChar() );
}

Bool CHLSLMaterialPixelShaderCompiler::CompileRenderPassSimple( ERenderingPass pass )
{
	switch ( pass )
	{
		case RP_ShadowDepthSolid:
		case RP_ShadowDepthMasked:
		{
			// No output here (we're just laying down the depth)
			return true;
		}
	
		case RP_HitProxies:
		{
			// Output hit proxy color 
			CodeChunk hitProxyColor = ConstReg( MDT_Float4, "PSC_HitProxyColor" );
			Output( MDT_Float4, "SYS_TARGET_OUTPUT0", hitProxyColor );
			return true;
		}

		case RP_HiResShadowMask:
		{	
			Include( "hiResShadows.fx" );

			CodeChunk pos = Data( "WorldPosition" );
			
			CodeChunk param = AutomaticName();
			CodeChunk call = CodeChunk::Printf( false, "float %s = CalcHiResShadows( %s );", param.AsChar(), pos.xyz().AsChar() );
			Statement( call );			
			
			Output( MDT_Float4, "SYS_TARGET_OUTPUT0", param );
			return true;
		}
	}

	// Not every pass type is supported by this function, so simply return false.
	return false;	
}

Bool CHLSLMaterialPixelShaderCompiler::CompileDeferredShadingGBuffer( const CodeChunk &diffuse, const CodeChunk &normal, const CodeChunk &vertexNormal, const CodeChunk &specularity, const CodeChunk &glossinessFactor, const CodeChunk &translucencyFactor, const CodeChunk &ambientOcclusion, const CodeChunk &subsurfaceScattering, const CodeChunk &materialFlagsMaskEncoded )
{
	Include( "inc_bestfitnormals.fx" );
	
	CodeChunk packed_normal;
	{		
		CodeChunk param = AutomaticName();
		CodeChunk call = CodeChunk::Printf( false, "float3 %s = CompressNormalsToUnsignedGBuffer( %s );", param.AsChar(), normal.AsChar() );
		Statement( call );

		packed_normal = param;
	}

	CodeChunk attrib0 = Float4 ( diffuse.xyz(), translucencyFactor );
	CodeChunk attrib1 = Float4 ( packed_normal.xyz(), glossinessFactor );
	CodeChunk attrib2 = Float4 ( specularity.xyz(), materialFlagsMaskEncoded );

	Output( MDT_Float4, "SYS_TARGET_OUTPUT0", attrib0 );
	Output( MDT_Float4, "SYS_TARGET_OUTPUT1", attrib1 );
	Output( MDT_Float4, "SYS_TARGET_OUTPUT2", attrib2 );

	return true;
}

Bool CHLSLMaterialPixelShaderCompiler::CompileDeferredShadingForwardGBuffer( const CodeChunk &normal )
{
	Include( "inc_bestfitnormals.fx" );

	CodeChunk packed_normal;
	{		
		CodeChunk param = AutomaticName();
		CodeChunk call = CodeChunk::Printf( false, "float3 %s = CompressNormalsToUnsignedGBuffer( %s );", param.AsChar(), normal.AsChar() );
		Statement( call );

		packed_normal = param;
	}
	
	CodeChunk attrib  = Float4 ( packed_normal.xyz(), Float(0.5f) );
	CodeChunk attrib2 = Float4 ( 0.f, 0.f, 0.f, 0.f/*GBUFF_MATERIAL_MASK_ENCODED_DEFAULT*/ );
	
	Output( MDT_Float4, "SYS_TARGET_OUTPUT0", attrib );
	Output( MDT_Float4, "SYS_TARGET_OUTPUT1", attrib2 );
	
	return true;
}

void CHLSLMaterialPixelShaderCompiler::CompileOptionalFragmentClipping( const MaterialRenderingContext &context, const CodeChunk& maskValue )
{
	if ( context.m_discardingPass )
	{
		if ( !maskValue.IsEmpty() )
		{
			CodeChunk call = CodeChunk::Printf( false, "[branch] if ( (PSC_DiscardFlags.x) != 0 ) { clip( (%s).x ); }", maskValue.AsChar() );
			Statement( call );
		}

		if ( DoesMaterialVertexFactorySupportDissolve( context.m_vertexFactory ) )
		{
			Uint32 resolution			= DISSOLVE_TEXTURE_SIZE;
			CodeChunk screenPos			= Data( "ScreenVPOS" );
			CodeChunk dissolveTexel		= screenPos.xy() % resolution;
			CodeChunk dissolveSampler	= ConstSampler( MST_Texture, "PSSMP_Dissolve", PSSMP_Dissolve );
			CodeChunk dissolveData		= Tex2Dload( dissolveSampler, dissolveTexel, 0u );
			CodeChunk dissolveParams	= Data( "DissolveParams" );

			CodeChunk discardValue		= dissolveData * dissolveParams.xxxx() + dissolveParams.yyyy();

			CodeChunk call = CodeChunk::Printf( false, "[branch] if ( (PSC_DiscardFlags.y) != 0 ) { clip( (%s).x ); }", discardValue.AsChar() );
			Statement( call );
		}

		if ( DoesMaterialVertexFactorySupportUVDissolve( context.m_vertexFactory ) )
		{
			CodeChunk texPos			= Data( "TexCoords_Dissolve" );
			CodeChunk dissolveSampler	= ConstSampler( MST_Texture, "PSSMP_UVDissolve", PSSMP_UVDissolve );
			CodeChunk dissolveData		= Tex2D( dissolveSampler, texPos.xy() );
			CodeChunk dissolveParams	= Data( "DissolveParams" );

			CodeChunk discardValue		= dissolveData * dissolveParams.zzzz() + dissolveParams.wwww();

			CodeChunk call = CodeChunk::Printf( false, "[branch] if ( (PSC_DiscardFlags.z) != 0 ) { clip( (%s).x ); }", discardValue.AsChar() );
			Statement( call );
		}

		if ( DoesMaterialVertexFactorySupportClippingEllipse( context.m_vertexFactory ) )
		{
			// Discard anything that is inside the clipping ellipse. Since ClippingEllipsePosition is the position transformed
			// into the ellipse's own local space, we just have to check if the position is outside the unit sphere.
			CodeChunk ellipsePos		= Data( "ClippingEllipsePosition" );
			CodeChunk discardValue		= Dot3( ellipsePos, ellipsePos ) - 1.0f;

			CodeChunk call = CodeChunk::Printf( false, "[branch] if ( (PSC_DiscardFlags.w) != 0 ) { clip( (%s).x ); }", discardValue.AsChar() );
			Statement( call );
		}
	}
}

void CHLSLMaterialPixelShaderCompiler::CompileMSAACoverageMask( const CodeChunk &alphaValue )
{
	CodeChunk coverage = Var( MDT_Uint, CodeChunk::Printf( false, "BuildMSAACoverageMask_AlphaToCoverage( %s )", alphaValue.AsChar() ) );
	Output( MDT_Uint, "SV_Coverage", coverage );
}

CodeChunk CHLSLMaterialPixelShaderCompiler::PrepareForTwoSidedLighting( const CodeChunk &normal, const CodeChunk& worldNormal )
{
	CodeChunk normalBack		= Var( MDT_Float3, normal.xyz() - worldNormal.xyz() * Dot3( worldNormal.xyz(), normal.xyz() ) * 2.f );
	CodeChunk faceSide			= Data( "FadeSide" );
	CodeChunk modifiedNormal	= Var( MDT_Float3, normal.xyz() );
	CodeChunk call = CodeChunk::Printf( false, "[branch] if ( (%s) <= 0 ) { %s = %s; }", faceSide.AsChar(), modifiedNormal.xyz().AsChar(), normalBack.xyz().AsChar() );
	Statement( call );

	return modifiedNormal;
}

Bool CHLSLMaterialPixelShaderCompiler::IsFeedbackDataFetchSupported( ERenderFeedbackDataType feedbackType ) const
{
	return true;
}

void CHLSLMaterialPixelShaderCompiler::CompileFeedbackDataFetch( ERenderFeedbackDataType feedbackType, CodeChunk &outData, const CodeChunk *coordOffset )
{
	// Calculate texcoord
	CodeChunk coord	= Data( "ScreenUV" ).xy();
	if ( NULL != coordOffset )
	{
		CodeChunk size		= ConstReg( MDT_Float4, "PSC_ViewportSize" );
		CodeChunk subSize	= ConstReg( MDT_Float4, "PSC_ViewportSubSize" );
		CodeChunk maxCoord	= subSize.zw() - size.zw() * 0.5f;
		coord = Min( coord + coordOffset->xy(), maxCoord );
	}

	// Fetch data
	switch ( feedbackType )
	{
	case RFDT_Depth:
		{
			CodeChunk sampler = ConstSampler( MST_Texture, "PSSMP_SceneDepth", PSSMP_SceneDepth );
			CodeChunk linearDepth = Tex2Dlod( sampler, Float4 (coord, 0.f, 0.f) ).x();
			CodeChunk deprojectedDepthParam = AutomaticName();
			CodeChunk call = CodeChunk::Printf( false, "float %s = DeprojectDepthRevProjAware( %s );", deprojectedDepthParam.AsChar(), linearDepth.AsChar());
			Statement( call );
			outData = Float4( deprojectedDepthParam, deprojectedDepthParam, deprojectedDepthParam, deprojectedDepthParam );
		}
		break;

	case RFDT_Color:
		{
			CodeChunk sampler = ConstSampler( MST_Texture, "PSSMP_SceneColor", PSSMP_SceneColor );
			outData = Tex2Dlod( sampler, Float4 (coord, 0.f, 0.f) );
		}
		break;

	default:
		ASSERT( !"invalid feedback type" );
	}
}

#endif
