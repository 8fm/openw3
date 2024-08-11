/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Environment transparency color filter
class CMaterialBlockEnvTransparencyColorFilter : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockEnvTransparencyColorFilter, CMaterialBlock, "System Samplers", "Environment Transparency Color Filter" );

public:
	CMaterialBlockEnvTransparencyColorFilter ()
	{}

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Color ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		using namespace CodeChunkOperators;

		const CodeChunk color  = CompileInput( compiler, CNAME( Color ), shaderTarget, Vector ( 0.5f, 0.5f, 0.5f, 1.f ) );
		const CodeChunk filter = PS_DATA( "EnvTranspColorFilter" );

		CodeChunk result  = color;		
		CodeChunk weights = RGB_LUMINANCE_WEIGHTS_EnvTransparencyColorFilter;
		// contrast
		result = CodeChunkOperators::Max( CodeChunk( 0.f ), Lerp( Dot3(result, weights), result, filter.w() ) );
		// filter
		result = result * filter;
		// restore original alpha
		result = Float4 ( result.xyz(), color.w() );

		return result;
	}
};


DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockEnvTransparencyColorFilter , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockEnvTransparencyColorFilter );

#endif
