/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbGammaCorrection.h"
#include "../engine/materialOutputSocket.h"
#include "../engine/materialCompiler.h"
#include "../engine/materialBlockCompiler.h"
#include "../engine/materialInputSocket.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockGammaCorrection );

CMaterialBlockGammaCorrection::CMaterialBlockGammaCorrection()
	: m_linearToGamma( false )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockGammaCorrection::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );

}

#endif

String CMaterialBlockGammaCorrection::GetCaption() const
{
	if ( m_linearToGamma == true )
	{
		return TXT("Linear to Gamma");
	}

	else
	{
		return TXT("Gamma to Linear");
	}
}

CodeChunk CMaterialBlockGammaCorrection::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	CodeChunk in = CompileInput( compiler, CNAME( In ), shaderTarget, 0.0f );

	if ( m_linearToGamma == true )
	{
		return compiler.GetPS().ApplyGammaToLinearExponent( MDT_Float4, in, true, false );
	}

	else
	{
		return compiler.GetPS().ApplyGammaToLinearExponent( MDT_Float4, in, false, false );
	}
}

#endif