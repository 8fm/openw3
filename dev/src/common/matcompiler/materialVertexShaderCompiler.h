/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Vertex shader compiler
class CHLSLMaterialVertexShaderCompiler : public CHLSLMaterialShaderCompiler
{
public:
	CHLSLMaterialVertexShaderCompiler( CHLSLMaterialCompiler* compiler );

	// Discard pixel fragment when value < 0
	virtual void Discard( const CodeChunk& value );

	// Get shader type
	virtual EMaterialShaderTarget GetShaderTarget() const { return MSH_VertexShader; }

	// Generate data
	virtual Bool GenerateData( const CodeChunk& semantic, CodeChunk& output );	

	// Compiles default code for given pass. Return value indicates if default code have been generated - otherwise explicit implementation is needed.
	virtual Bool CompileRenderPassSimple( ERenderingPass pass );

	// Compiles deferred shading GBuffer.
	virtual Bool CompileDeferredShadingGBuffer( const CodeChunk &diffuse, const CodeChunk &normal, const CodeChunk &vertexNormal, const CodeChunk &specularity, const CodeChunk &glossinessFactor, const CodeChunk &translucencyFactor, const CodeChunk &ambientOcclusion, const CodeChunk &subsurfaceScattering, const CodeChunk &materialFlagsMaskEncoded );

	// Compiles deferred shading GBuffer for forward shaded stuff.
	virtual Bool CompileDeferredShadingForwardGBuffer( const CodeChunk &normal );

	// Applies fragment clipping if needed
	virtual void CompileOptionalFragmentClipping( const MaterialRenderingContext &context, const CodeChunk& maskValue ) override;

	// Outputs MSAA coverage mask
	virtual void CompileMSAACoverageMask( const CodeChunk &alphaValue );

	// Modified given normal vector, so that it will be valid for two sided lighting
	virtual CodeChunk PrepareForTwoSidedLighting( const CodeChunk& normal, const CodeChunk& worldNormal );

	// Returns information wheather compiler provides given feedback data
	virtual Bool IsFeedbackDataFetchSupported( ERenderFeedbackDataType feedbackType ) const;

	// Returns given feedback data (if supported)
	virtual void CompileFeedbackDataFetch( ERenderFeedbackDataType feedbackType, CodeChunk &outData, const CodeChunk *coordOffset );
};

#endif