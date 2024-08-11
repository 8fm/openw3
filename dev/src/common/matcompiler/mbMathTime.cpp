/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

#include "../matcompiler//materialShaderConstants.h"
#include "../engine/graphConnectionRebuilder.h"

/// Output game time
class CMaterialBlockMathTime : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockMathTime, CMaterialBlock, "Deprecated", "Time" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		if (shaderTarget == MSH_PixelShader)
		{
			return SHADER_CONST_FLOAT4( PSC_TimeVector, shaderTarget );
		}
		else
		{
			return SHADER_CONST_FLOAT4( VSC_TimeVector, shaderTarget );
		}
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockMathTime , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathTime );

#endif