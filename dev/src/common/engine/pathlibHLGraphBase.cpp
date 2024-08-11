/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibHLGraphBase.h"

#include "pathlibGraphBase.inl"

namespace PathLib
{

void CHLGraphBase::PreNodeArrayOverflow()
{
	ConvertLinksToIds();
}
void CHLGraphBase::OnNodeArrayOverflow()
{
	ConvertLinksToPointers();
}

Bool CHLGraphBase::ConvertLinksToIds()
{
	return Super::ConvertLinksToIds( true );
}
Bool CHLGraphBase::ConvertLinksToPointers()
{
	return Super::ConvertLinksToPointers( true );
}

CPathNode* CHLGraphBase::VGetPathNode( CPathNode::Id id )
{
	return Super::GetNode( id.m_index );
}

CHLGraphBase::CHLGraphBase()
	: m_nextRegionId( 0 )
{
}
CHLGraphBase::~CHLGraphBase()
{

}

CHLNode& CHLGraphBase::AddNode( AreaId areaId, AreaRegionId regionId )
{
	CHLNode& node = Super::AddNode( CHLNode( areaId, *this, regionId ) );

	m_mapping[ regionId ] = node.GetIndex();

	return node;
}

CHLNode* CHLGraphBase::FindNode( AreaRegionId regionId )
{
	auto itFind = m_mapping.Find( regionId );
	if ( itFind == m_mapping.End() )
	{
		return nullptr;
	}
	return Super::GetNode( itFind->m_second );
}

void CHLGraphBase::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Super::WriteToBuffer( writer );

	writer.Put( m_nextRegionId );
}
Bool CHLGraphBase::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !Super::ReadFromBuffer( reader ) )
	{
		return false;
	}

	if ( !reader.Get( m_nextRegionId ) )
	{
		return false;
	}

	return true;
}

void CHLGraphBase::OnPostLoad()
{
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		m_mapping[ it->GetAreaRegionId() ] = it->GetIndex();
	}
	ConvertLinksToPointers();
}

void CHLGraphBase::DeleteMarked()
{
	struct Implementation
	{
		enum { PRE_NODE_TRASHING = true };

		CHLGraphBase*			m_graph;

		Implementation( CHLGraphBase* me )
			: m_graph( me ) {}

		void PreNodeTrashing( CHLNode& node )
		{
			m_graph->m_mapping.Erase( node.GetAreaRegionId() );
		}
		void PreDeletion()
		{
			m_graph->ConvertLinksToIds();
		}
		void PostDeletion()
		{
			m_graph->ConvertLinksToPointers();
		}
		void PreNodeIndexChange( CHLNode& originalNode, CHLNode::Index newIndex )
		{
			m_graph->m_mapping[ originalNode.GetAreaRegionId() ] = newIndex;
		}
	} implemetation( this );
	Super::DeleteMarked( implemetation );
}

Bool CHLGraphBase::Debug_VerifyNoDanglingMappings()
{
	for ( auto it = m_mapping.Begin(), end = m_mapping.End(); it != end; ++it )
	{
		AreaRegionId regionId = it->m_first;
		CHLNode::Index idx = it->m_second;
		if ( idx >= m_nodes.Size() )
		{
			return false;
		}
		if ( m_nodes[ idx ].GetAreaRegionId() != regionId )
		{
			return false;
		}
	}
	return true;
}

Bool CHLGraphBase::Debug_CheckAllLinksTwoSided()
{
	return Super::Debug_CheckAllLinksTwoSided();
}
Bool CHLGraphBase::Debug_VerifyMappingConsistency()
{
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		if ( m_mapping.Find( it->GetAreaRegionId() ) == m_mapping.End() )
		{
			return false;
		}
	}
	return true;
}

};			// namespace PathLib