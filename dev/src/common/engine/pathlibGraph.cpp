#include "build.h"
#include "pathlibGraph.h"

namespace PathLib
{

/////////////////////////////////////////////////////////////////////////
// CPathLibGraph
/////////////////////////////////////////////////////////////////////////
CPathLibGraph::CPathLibGraph()
	: m_initialVersion( 0 )
	, m_version( 0 )
{}
CPathLibGraph::~CPathLibGraph()
{

}

void CPathLibGraph::PreNodeArrayOverflow()
{

}

void CPathLibGraph::OnNodeArrayOverflow()
{

}

CPathLibWorld* CPathLibGraph::VGetPathLibWorld() const
{
	return nullptr;
}
CPathNode* CPathLibGraph::VGetExternalPathNode( CPathNode::Index idx, AreaId areaId )
{
	return nullptr;
}
LinkBufferIndex CPathLibGraph::VGetExtraLinksSpace() const
{
	return 0;
}
 AreaId CPathLibGraph::VGetAreaId() const
 {
	 return INVALID_AREA_ID;
 }

void CPathLibGraph::MarkNodeForDeletion( CPathNode& node )
{
	node.Unlink();
	node.AddFlags( NF_MARKED_FOR_DELETION );
}

LinkBufferIndex CPathLibGraph::AddLink( CPathNode& node, const CPathLink& link )
{
	if ( &node.GetGraph() != this )
	{
		return node.GetGraph().AddLink( node, link );
	}

	LinkBufferIndex idx = m_links.Allocate( link );
	m_links.Get( idx ).m_next = node.m_links;
	node.m_links = idx;
	return idx;
}

};		// namespace PathLib