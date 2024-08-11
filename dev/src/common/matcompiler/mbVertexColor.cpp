/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../engine/materialBlock.h"
#include "../engine/graphConnectionRebuilder.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Vertex color output stream
class CMaterialBlockVertexColor : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockVertexColor, CMaterialBlock, "Input", "Vertex Color" );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets()
	{
		GraphConnectionRebuilder rebuilder( this );
		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("xyzw"), CNAME( Color ), Color::WHITE ) );

		{
			MaterialOutputSocketSpawnInfo a( TXT("xxxx"), CNAME( Red ), Color::RED );
			a.m_isVisible = true;
			a.m_isVisibleByDefault = true;
			CreateSocket( a );
		}

		{
			MaterialOutputSocketSpawnInfo a( TXT("yyyy"), CNAME( Green ), Color::GREEN );
			a.m_isVisible = true;
			a.m_isVisibleByDefault = true;
			CreateSocket( a );
		}


		{
			MaterialOutputSocketSpawnInfo a( TXT("zzzz"), CNAME( Blue ), Color::BLUE );
			a.m_isVisible = true;
			a.m_isVisibleByDefault = true;
			CreateSocket( a );
		}

		CreateSocket( MaterialOutputSocketSpawnInfo( TXT("wwww"), CNAME( Alpha ), Color( 127, 127, 127 ) ) );
	}

#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		return SHADER_DATA( "VertexColor", shaderTarget );
	}
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialBlockVertexColor , CMaterialBlock);

IMPLEMENT_ENGINE_CLASS( CMaterialBlockVertexColor );

#endif