/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Converts value in linear space into gamma space
class CMaterialBlockConvertLinearToGamma : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockConvertLinearToGamma, CMaterialBlock, "Deprecated", "LinearToGamma" );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( X ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk x = CompileInput( compiler, CNAME( X ), shaderTarget, 0.0f );		
		return compiler.GetPS().ApplyGammaToLinearExponent( MDT_Float4, x, true, false );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockConvertLinearToGamma , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockConvertLinearToGamma );

#endif