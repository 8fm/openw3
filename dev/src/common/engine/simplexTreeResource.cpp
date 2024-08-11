/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "simplexTreeResource.h"

IMPLEMENT_ENGINE_CLASS( CResourceSimplexTree );

CResourceSimplexTree::CResourceSimplexTree()
{

}

CResourceSimplexTree::~CResourceSimplexTree()
{

}

CSimplexTreeNode CResourceSimplexTree::Get()
{
	CSimplexTreeNode out;
	out.GetNodes().Clear();
	const Int32 num = m_nodes.Size();
	Int32 i;
	out.GetNodes().Reserve( num );
	for( i=0;i<num;++i )
	{
		out.GetNodes().PushBack( m_nodes[i] );
	}
	return out;
}