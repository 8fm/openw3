/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibNavgraph.h"
#include "pathlibNodeSet.h"

namespace PathLib
{


namespace NavgraphHelper
{
	template < class Functor >
	RED_INLINE void ForAllNodes( CNavGraph& navgraph, Functor& functor, Bool onlyAttachedNodeSets = false )
	{
		auto& nodes = navgraph.GetNodesArray();
		for ( auto it = nodes.Begin(), end = nodes.End(); it != end; ++it )
		{
			CNavNode& node = *it;
			functor( node );
		}

		const auto& nodeSets = navgraph.GetNodeSets();
		for ( auto itSets = nodeSets.Begin(), endSets = nodeSets.End(); itSets != endSets; ++itSets )
		{
			CNavgraphNodeSet* nodeSet = itSets->m_second;
			if ( onlyAttachedNodeSets && !nodeSet->IsAttached() )
			{
				continue;
			}

			auto& nodeList = nodeSet->GetNodesArray();
			for ( auto it = nodeList.Begin(), end = nodeList.End(); it != end; ++it )
			{
				CNavNode& node = *it;
				functor( node );
			}
		}
	}

	template < class Functor >
	RED_INLINE Bool ForAllNodesBreakable( CNavGraph& navgraph, Functor& functor, Bool onlyAttachedNodeSets = false )
	{
		auto& nodes = navgraph.GetNodesArray();
		for ( auto it = nodes.Begin(), end = nodes.End(); it != end; ++it )
		{
			CNavNode& node = *it;
			if ( !functor( node ) )
			{
				return false;
			}
		}

		const auto& nodeSets = navgraph.GetNodeSets();
		for ( auto itSets = nodeSets.Begin(), endSets = nodeSets.End(); itSets != endSets; ++itSets )
		{
			CNavgraphNodeSet* nodeSet = itSets->m_second;
			if ( onlyAttachedNodeSets && !nodeSet->IsAttached() )
			{
				continue;
			}

			auto& nodeList = nodeSet->GetNodesArray();
			for ( auto it = nodeList.Begin(), end = nodeList.End(); it != end; ++it )
			{
				CNavNode& node = *it;
				if ( !functor( node ) )
				{
					return false;
				}
			}
		}
		return true;
	}
};


};		// namespace PathLib

