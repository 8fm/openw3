/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbMathSplitAppendVector.h"
#include "../engine/graphBlock.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockMathSplitAppendVector );

CMaterialBlockMathSplitAppendVector::CMaterialBlockMathSplitAppendVector()
	: m_splitVector( true )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockMathSplitAppendVector::OnPropertyPostChange( IProperty *property )
{
	// Pass to base class
	TBaseClass::OnPropertyPostChange( property );

	// Update layout
	OnRebuildSockets();
}
void CMaterialBlockMathSplitAppendVector::OnRebuildSockets()
{
	if ( m_splitVector == true )
	{
		GraphConnectionRebuilder rebuilder( this );    
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( In )) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xxxx"), CNAME( X ), Color::RED ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("yyyy"), CNAME( Y ), Color::GREEN ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("zzzz"), CNAME( Z ), Color::BLUE ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( W ), Color( 127, 127, 127 ) ) );
	}

	else
	{
		GraphConnectionRebuilder rebuilder( this );    
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( X ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( Y ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( Z ) ) );
		CreateSocket( MaterialInputSocketSpawnInfo ( CNAME( W ) ) );
		CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
	}
}



String CMaterialBlockMathSplitAppendVector::GetCaption() const
{
	if ( m_splitVector == true )
	{
		return TXT("Split Vector");
	}

	else
	{
		return TXT("Append Vector");
	}
}
#endif


CodeChunk CMaterialBlockMathSplitAppendVector::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{
	if ( m_splitVector == true )
	{
		CodeChunk in = CompileInput( compiler, CNAME( In ), shaderTarget, 0.0f );
		return in;
	}

	else
	{
		CodeChunk x = CompileInput( compiler, CNAME( X ), shaderTarget, 0.0f );
		CodeChunk y = CompileInput( compiler, CNAME( Y ), shaderTarget, 0.0f );
		CodeChunk z = CompileInput( compiler, CNAME( Z ), shaderTarget, 0.0f );
		CodeChunk w = CompileInput( compiler, CNAME( W ), shaderTarget, 0.0f );
		return Float4( x.x(), y.x(), z.x(), w.x() );
	}
}

#endif