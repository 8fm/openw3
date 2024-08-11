/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

CHLSLMaterialVertexShaderCompiler::CHLSLMaterialVertexShaderCompiler( CHLSLMaterialCompiler* compiler )
	: CHLSLMaterialShaderCompiler( compiler, 0, 0, 0, 0 )
{
}

Bool CHLSLMaterialVertexShaderCompiler::GenerateData( const CodeChunk& semantic, CodeChunk& output )
{
	// World position
	if ( semantic == "WorldPosition" )
	{
		output = CodeChunk( "FatVertex.WorldPosition", false );
		return true;
	}

	// World normal
	if ( semantic == "WorldNormal" )
	{
		output = CodeChunk( "FatVertex.WorldNormal", false );
		return true;
	}

	// World binormal
	if ( semantic == "WorldBinormal" )
	{
		output = CodeChunk( "FatVertex.WorldBinormal", false );
		return true;
	}

	// World tangent
	if ( semantic == "WorldTangent" )
	{
		output = CodeChunk( "FatVertex.WorldTangent", false );
		return true;
	}

	// Vertex position
	if ( semantic == "VertexPosition" )
	{
		output = CodeChunk( "FatVertex.VertexPosition", false );
		return true;
	}

	// Vertex normal
	if ( semantic == "VertexNormal" )
	{
		output = CodeChunk( "FatVertex.VertexNormal", false );
		return true;
	}

	// Vertex color
	if ( semantic == "VertexColor" )
	{
		output = CodeChunk( "FatVertex.VertexColor", false );
		return true;
	}

	// Texcoord0
	if ( semantic == "TexCoords0" )
	{
		output = CodeChunk( "FatVertex.UV", false );
		return true;
	}

	// Texcoord1
	if ( semantic == "TexCoords1" )
	{
		output = CodeChunk( "FatVertex.UV2", false );
		return true;
	}

	// Texcoord2
	if ( semantic == "TexCoords2" )
	{
		output = CodeChunk( "FatVertex.ProjUV", false );
		return true;
	}

	// Screen-space position
	if ( semantic == "ScreenPosition" )
	{
		output = CodeChunk( "FatVertex.ScreenPosition", false );
		return true;
	}

	// View-space position
	if ( semantic == "ViewPosition" )
	{
		output = CodeChunk( "FatVertex.ViewPosition", false );
		return true;
	}

	// Position in light space
	if ( semantic == "LightPosition" )
	{
		output = CodeChunk( "FatVertex.LightPosition", false );
		return true;
	}

	// Pixel screen-space W
	if ( semantic == "ScreenW" )
	{
		output = CodeChunk( "FatVertex.ScreenPosition.w", false );
		return true;
	}

	// Pixel screen-space z
	if ( semantic == "ScreenZ" )
	{
		output = CodeChunk( "FatVertex.ScreenPosition.z", false );
		return true;
	}

	// Pixel view-space W
	if ( semantic == "ViewW" )
	{
		output = CodeChunk( "FatVertex.ViewPosition.w", false );
		return true;
	}

	// Pixel view-space z
	if ( semantic == "ViewZ" )
	{
		output = CodeChunk( "FatVertex.ViewPosition.z", false );
		return true;
	}

	// Animation frame
	if ( semantic == "AnimationFrame" )
	{
		output = CodeChunk( "FatVertex.AnimationFrame", false );
		return true;
	}

	// Motion blend ratio 
	if ( semantic == "MotionBlend" )
	{
		output = CodeChunk( "FatVertex.MotionBlend", false );
		return true;
	}

	// Dissolve factor
	if ( semantic == "DissolveParams" )
	{
		output = CodeChunk( "FatVertex.DissolveParams", false );
		return true;
	}	

	// Foliage color
	if ( semantic == "FoliageColor" )
	{	
		output = CodeChunk( "FatVertex.FoliageColor", false );
		return true;
	}
		
	if ( semantic == "TexIndex" )
	{
		output = CodeChunk( "FatVertex.TexIndex", false );
		return true;
	}

	if ( semantic == "TexCoords_Dissolve" )
	{
		output = CodeChunk( "FatVertex.DissolveUV", false );
		return true;
	}

	if ( semantic == "ClippingEllipsePosition" )
	{
		output = CodeChunk( "FatVertex.ClippingEllipsePos", false );
		return true;
	}

	// Particle emitter center to vertex direction
	if ( semantic == "SkeletalExtraData" )
	{
		output = CodeChunk( "FatVertex.SkeletalExtraData", false );
		return true;
	}

	// Not defined
	return false;
}

void CHLSLMaterialVertexShaderCompiler::Discard( const CodeChunk& value )
{	
}

Bool CHLSLMaterialVertexShaderCompiler::CompileRenderPassSimple( ERenderingPass pass )
{
	return false;
}

Bool CHLSLMaterialVertexShaderCompiler::CompileDeferredShadingForwardGBuffer( const CodeChunk &normal )
{
	return false;
}

Bool CHLSLMaterialVertexShaderCompiler::CompileDeferredShadingGBuffer( const CodeChunk &diffuse, const CodeChunk &normal, const CodeChunk &vertexNormal, const CodeChunk &specularity, const CodeChunk &glossinessFactor, const CodeChunk &translucencyFactor, const CodeChunk &ambientOcclusion, const CodeChunk &subsurfaceScattering, const CodeChunk &materialFlagsMaskEncoded ) 
{
	return false;
}

void CHLSLMaterialVertexShaderCompiler::CompileOptionalFragmentClipping( const MaterialRenderingContext &context, const CodeChunk& maskValue )
{
	// empty
}

void CHLSLMaterialVertexShaderCompiler::CompileMSAACoverageMask( const CodeChunk &alphaValue )
{
	// empty
}

CodeChunk CHLSLMaterialVertexShaderCompiler::PrepareForTwoSidedLighting( const CodeChunk &normal, const CodeChunk& worldNormal )
{
	return CodeChunk::EMPTY;
	// empty
}

Bool CHLSLMaterialVertexShaderCompiler::IsFeedbackDataFetchSupported( ERenderFeedbackDataType feedbackType ) const
{
	return false;
}

void CHLSLMaterialVertexShaderCompiler::CompileFeedbackDataFetch( ERenderFeedbackDataType feedbackType, CodeChunk &outData, const CodeChunk *coordOffset )
{
	ASSERT( !"invalid call" );
}

#endif
