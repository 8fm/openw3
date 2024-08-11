/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeIdleRoot.h"


class CBehTreeNodeFlightIdleDynamicRootInstance;

//////////////////////////////////////////////////////////////////////
// CBehTreeNodeIdleDynamicRootDefinition
class CBehTreeNodeFlightIdleDynamicRootDefinition : public CBehTreeNodeBaseIdleDynamicRootDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeFlightIdleDynamicRootDefinition, CBehTreeNodeBaseIdleDynamicRootDefinition, CBehTreeNodeFlightIdleDynamicRootInstance, FlightIdleDynamic );
public:
	CBehTreeNodeFlightIdleDynamicRootDefinition()									{}

	CName GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
	IAITree* GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;

	static CName StaticEventName();
	static CName StaticAIParamName();

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeFlightIdleDynamicRootDefinition );
	PARENT_CLASS( CBehTreeNodeBaseIdleDynamicRootDefinition );
END_CLASS_RTTI();

/////////////////////////////////////////////////////////////////////
// CBehTreeNodeIdleDynamicRootInstance
class CBehTreeNodeFlightIdleDynamicRootInstance : public CBehTreeNodeBaseIdleDynamicRootInstance
{
	typedef CBehTreeNodeBaseIdleDynamicRootInstance Super;
public:
	typedef CBehTreeNodeFlightIdleDynamicRootDefinition Definition;

	CBehTreeNodeFlightIdleDynamicRootInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeBaseIdleDynamicRootInstance( def, owner, context, parent )	{}
};

