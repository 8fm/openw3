/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibGraphBase.h"
#include "pathlibHLNode.h"

namespace PathLib
{

class CHLGraphBase : public TPathLibGraph< CHLNode >
{
	typedef TPathLibGraph< CHLNode > Super;
protected:
	typedef THashMap< AreaRegionId, CHLNode::Index > Region2IndexMapping;

	Region2IndexMapping				m_mapping;
	AreaRegionId					m_nextRegionId;

	void							PreNodeArrayOverflow() override;
	void							OnNodeArrayOverflow() override;

	Bool							ConvertLinksToIds();
	Bool							ConvertLinksToPointers();

	CPathNode*						VGetPathNode( CPathNode::Id id ) override;
public:
	CHLGraphBase();
	~CHLGraphBase();

	CHLNode&						AddNode( AreaId areaId, AreaRegionId regionId );
	CHLNode*						FindNode( AreaRegionId regionId );

	void							WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool							ReadFromBuffer( CSimpleBufferReader& reader );

	void							OnPostLoad();

	void							DeleteMarked();

	AreaRegionId					GetNextUniqueRegionId()							{ return m_nextRegionId++; }

	Bool							Debug_VerifyNoDanglingMappings();
	Bool							Debug_CheckAllLinksTwoSided();
	Bool							Debug_VerifyMappingConsistency();
};


};		// namespace PathLib
