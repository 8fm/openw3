/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

/// Vertex world space position 
class CMaterialBlockVFace : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockVFace, CMaterialBlock, "Input", "Face Side" );

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
		CodeChunk faceSide = SHADER_DATA( "FadeSide", shaderTarget );
		CodeChunk front = SHADER_VAR_FLOAT( 1.0f, shaderTarget );
		CodeChunk back  = SHADER_VAR_FLOAT( -1.0f, shaderTarget );
		CodeChunk vface = CodeChunk::Printf( false, "((%s)>0 ? (%s) : (%s))", faceSide.AsChar(), front.AsChar(), back.AsChar() );
		return SHADER_VAR_FLOAT4( Float4( vface.xxxx() ), shaderTarget );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockVFace , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockVFace );

#endif