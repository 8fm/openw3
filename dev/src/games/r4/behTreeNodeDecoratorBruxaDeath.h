/*
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/gameSession.h"

#include "../../common/game/behTreeDecorator.h"
#include "../../common/game/behTreeGuardAreaData.h"


class CBehTreeNodeDecoratorBruxaDeathInstance;

class CBehTreeNodeDecoratorBruxaDeathDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorBruxaDeathDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDecoratorBruxaDeathInstance, CustomBruxaDeathDecorator );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const;
};


BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorBruxaDeathDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI();

class CBehTreeNodeDecoratorBruxaDeathInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeGuardAreaDataPtr			m_guardAreaDataPtr;
	CGameSaveLock						m_saveLock;
public:
	typedef CBehTreeNodeDecoratorBruxaDeathDefinition Definition;

	CBehTreeNodeDecoratorBruxaDeathInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool					Activate() override;
	void					Deactivate() override;

	Bool					OnEvent( CBehTreeEvent& e ) override;
};

