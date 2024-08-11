/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../core/simplexTree.h"
#include "../core/resource.h"

class CResourceSimplexTree : public CResource
{	
	DECLARE_ENGINE_RESOURCE_CLASS( CResourceSimplexTree, CResource, "w3simplex", "CResourceSimplexTree" );

public:
	CResourceSimplexTree();
	virtual ~CResourceSimplexTree();
	TDynArray<SSimplexTreeStruct> & GetNodes() { return m_nodes; }
	CSimplexTreeNode Get();
private:
	TDynArray<SSimplexTreeStruct> m_nodes;
};

BEGIN_CLASS_RTTI( CResourceSimplexTree );
PARENT_CLASS( CResource );
PROPERTY_RO( m_nodes,			TXT("Nodes") );
END_CLASS_RTTI();