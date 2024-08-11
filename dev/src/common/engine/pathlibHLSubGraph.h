/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibHLGraphBase.h"

namespace PathLib
{

class CHLSubGraph : private CHLGraphBase
{
	typedef CHLGraphBase Super;
protected:
	CNavGraph&						m_navgraph;

	CNavGraph&						GetNavgraph() const								{ return m_navgraph; }

	CPathNode*						VGetExternalPathNode( CPathNode::Index idx, AreaId areaId ) override;
	CPathLibWorld*					VGetPathLibWorld() const;
	AreaId							VGetAreaId() const;
public:
	CHLSubGraph( CNavGraph& navgraph )
		: m_navgraph( navgraph )													{}

	CHLNode&						AddHLNode( AreaRegionId areaRegionId );
	CHLNode*						FindHLNode( AreaRegionId areaRegionId );
	CHLNode&						RequestHLNode( AreaRegionId areaRegionId );
	void							ConnectRegions( CHLNode& n0, CHLNode& n1, NodeFlags nodeFlags );
	void							ConnectRegions( AreaRegionId region, AreaRegionId destRegion, NodeFlags nodeFlags );
	void							ConnectRegions( AreaRegionId region, AreaId destAreaId, AreaRegionId destRegion, NodeFlags nodeFlags );

	void							UnlinkFromHLGraph();
	void							CleanLegacyNodes();
	void							UpdateLinkage();

	// wrappers for base interface
	CHLNode*						GetHLNode( CHLNode::Index index )				{ return GetNode( index ); }
	Uint32							GetHLNodesCount() const							{ return GetNodesArray().Size(); }
	AreaRegionId					GetNextUniqueRegionId()							{ return Super::GetNextUniqueRegionId(); }
	NodesArray&						GetHLNodesArray()								{ return Super::GetNodesArray(); }
	const NodesArray&				GetHLNodesArray() const							{ return Super::GetNodesArray(); }

	void							WriteHLToBuffer( CSimpleBufferWriter& writer ) const	{ Super::WriteToBuffer( writer ); }
	Bool							ReadHLFromBuffer( CSimpleBufferReader& reader )			{ return Super::ReadFromBuffer( reader ); }
	void							OnHLPostLoad()											{ Super::OnPostLoad(); }

	Bool							Debug_HLCheckAllLinksTwoSided();
};

};