/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "materialBlock.h"
#include "materialGraph.h"

/// Converts value in gamma space into linear space
class CMaterialEncapsulatedGraph : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialEncapsulatedGraph, CMaterialBlock, "Deprecated", "EncapsulatedGraph" );

protected:
	CMaterialGraph* m_graph;

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	CMaterialEncapsulatedGraph( )
	{
	}

	virtual void OnPostLoad()
	{
		TBaseClass::OnPostLoad();
	}

	virtual void OnRebuildSockets()
	{
		//GraphConnectionRebuilder rebuilder( this );
		//CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
	}

	virtual void OnPropertyPostChange( IProperty *property )
	{
	}

	CMaterialGraph* GetGraph()
	{
		if(!m_graph)
		{
			m_graph = new CMaterialGraph();
		}
		return m_graph;
	}

#endif
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const
	{
		return CodeChunk::EMPTY;
	}
#endif
};

BEGIN_CLASS_RTTI( CMaterialEncapsulatedGraph )
	PARENT_CLASS( CMaterialBlock )
END_CLASS_RTTI();