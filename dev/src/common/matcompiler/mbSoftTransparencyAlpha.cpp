/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Soft transparency alpha
class CMaterialBlockSoftTransparencyAlpha : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockSoftTransparencyAlpha, CMaterialBlock, "Image Processing", "Soft Transparency Alpha" );

	Bool	m_invertAlpha;

public:
	CMaterialBlockSoftTransparencyAlpha ()
		: m_invertAlpha( false )
	{}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialInputSocketSpawnInfo( CNAME( Sharpness ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		CodeChunk result = 1.f;

		if ( compiler.GetPS().IsFeedbackDataFetchSupported( RFDT_Depth ) )
		{
			// Compile scene depth
			CodeChunk sharpness  = CompileInput( compiler, CNAME( Sharpness ), shaderTarget, 1.f ).x();
			CodeChunk refDepth   = PS_DATA( "ViewZ" ).x();
			CodeChunk sceneDepth = 0.f;
			compiler.GetPS().CompileFeedbackDataFetch( RFDT_Depth, sceneDepth, NULL );

			// Calculate sharpness
			result = CodeChunkOperators::Saturate( (sceneDepth.x() - refDepth) * sharpness );
			if ( m_invertAlpha )
			{
				result = CodeChunk (1.f) - result;
			}
		}

		return CodeChunk ( Vector::ONES ) * result;
	}
};

BEGIN_CLASS_RTTI( CMaterialBlockSoftTransparencyAlpha )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_invertAlpha, TXT("Invert alpha") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CMaterialBlockSoftTransparencyAlpha );

#endif