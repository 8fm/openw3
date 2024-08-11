/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecoratorHLCombat.h"


class CBehTreeNodeDecoratorHLWildHorseDangerInstance;

class CBehTreeNodeDecoratorHLWildHorseDangerDefinition : public CBehTreeDecoratorHLDangerTamableDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorHLWildHorseDangerDefinition, CBehTreeDecoratorHLDangerTamableDefinition, CBehTreeNodeDecoratorHLWildHorseDangerInstance, HighLevelDangerWildHorse );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorHLWildHorseDangerDefinition )
	PARENT_CLASS( CBehTreeDecoratorHLDangerTamableDefinition )
END_CLASS_RTTI();


class CBehTreeNodeDecoratorHLWildHorseDangerInstance : public CBehTreeDecoratorHLDangerTamableInstance
{
	typedef CBehTreeDecoratorHLDangerTamableInstance Super;

protected:
	static CName SpecialWildHorseTag();

	Bool ConditionCheck() override;

public:
	typedef CBehTreeDecoratorHLDangerTamableDefinition Definition;

	CBehTreeNodeDecoratorHLWildHorseDangerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )													{}
};