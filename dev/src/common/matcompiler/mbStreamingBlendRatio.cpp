#include "build.h"
#include "mbStreamingBlendRatio.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

IMPLEMENT_ENGINE_CLASS( CMaterialBlockStreamingBlendRatio );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockStreamingBlendRatio::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
}

#endif

CodeChunk CMaterialBlockStreamingBlendRatio::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	return PS_VAR_FLOAT4( Vector::ZEROS );
}

#endif