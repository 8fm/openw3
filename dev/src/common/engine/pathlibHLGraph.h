/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibConst.h"

class CPathLibWorld;

namespace PathLib
{

class CHLNode;

namespace HLGraph
{

	class CAccessibleNodesMarking
	{
	private:
		Uint16			m_marker;

	public:
		CAccessibleNodesMarking()
			: m_marker( 0xbaad )															{}

		Bool	IsHLNodeAccessible( CHLNode& hlNode );
	};

	CHLNode* FindNode( CPathLibWorld& pathlib, Uint32 category, CoherentRegion regionId );
	Bool MarkAccessibleNodes( CPathLibWorld& pathlib, Uint32 category, CoherentRegion regionId, CAccessibleNodesMarking& outAccessibleNodes );

	Bool WalkableSlotQuery( );



};		// namespace HLGraph

};