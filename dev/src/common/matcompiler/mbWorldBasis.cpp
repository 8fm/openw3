/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "mbWorldBasis.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

using namespace CodeChunkOperators;

IMPLEMENT_ENGINE_CLASS( CMaterialBlockWorldBasis );
IMPLEMENT_RTTI_ENUM(EFrameBasisTypes);

CMaterialBlockWorldBasis::CMaterialBlockWorldBasis()
	: m_frameBasis( Normal )
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockWorldBasis::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
}

#endif

String CMaterialBlockWorldBasis::GetCaption() const
{

	switch ( m_frameBasis )
	{
		case Normal: 
			return TXT("World Normal");
		case Binormal: 		
			return TXT("World Binormal");
		case Tangent:
			return TXT("World Tangent");
		default: 
			return TXT("WTF?!");
	}


}

CodeChunk CMaterialBlockWorldBasis::Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const
{

	CodeChunk vector ;
	switch ( m_frameBasis )
	{
	case Normal: 
		vector = PS_DATA( "WorldNormal" );
		break;
	case Binormal: 		
		vector = PS_DATA( "WorldBinormal" );
		break;
	case Tangent:
		vector = PS_DATA( "WorldTangent" ); 
		break;
	}

	return PS_VAR_FLOAT4( Float4( Normalize( vector.xyz() ), 0.0f ) );
}

#endif