/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeDecorator.h"

#include "behTreeFlightData.h"


class IBehTreeNodeFlightDecoratorInstance;

class IBehTreeNodeFlightDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeFlightDecoratorDefinition, IBehTreeNodeDecoratorDefinition, IBehTreeNodeFlightDecoratorInstance, FlightDecorator );
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeFlightDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI()

class IBehTreeNodeFlightDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeFlightDataPtr			m_flightData;
public:
	typedef IBehTreeNodeFlightDecoratorDefinition Definition;

	IBehTreeNodeFlightDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_flightData( owner )													{}
};