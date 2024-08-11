/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Desaturation
class CMaterialBlockMathDesaturate : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathDesaturate, CMaterialBlock, "Image Processing", "Desaturate" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Amount ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk val = CompileInput( compiler, CNAME( In ), shaderTarget, 0.0f );
		CodeChunk y = SHADER_VAR_FLOAT4( RGB_LUMINANCE_WEIGHTS_MatBlockDesaturate, shaderTarget );
		CodeChunk lum = Dot3( val, y );

		CodeChunk ret;
		if ( HasInput( CNAME( Amount ) ) )
		{
			CodeChunk amount = CompileInput( compiler, CNAME( Amount ), shaderTarget, 0.5f );
			CodeChunk desaturated = Float4( lum.xxx(), val.w() );
			return SHADER_VAR_FLOAT4( Lerp( val, desaturated, amount.x() ), shaderTarget );
		}
		else
		{
			// Fully desaturated
			ret = SHADER_VAR_FLOAT4( Float4( lum.xxx(), val.w() ), shaderTarget );
		}

		return ret;
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathDesaturate , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathDesaturate );

#endif
