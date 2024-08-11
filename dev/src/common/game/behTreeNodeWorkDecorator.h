/*
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "behTreeDecorator.h"
#include "behTreeCounterData.h"
#include "behTreeWorkData.h"
#include "behTreeDynamicNode.h"

// Listening scared events
class CBehTreeNodeWorkDecoratorInstance;

class CBehTreeNodeWorkDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeWorkDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeWorkDecoratorInstance, WorkDecorator );
protected:	

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;	
};

BEGIN_CLASS_RTTI(CBehTreeNodeWorkDecoratorDefinition);
PARENT_CLASS(IBehTreeNodeDecoratorDefinition);	
END_CLASS_RTTI();

class CBehTreeNodeWorkDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef  IBehTreeNodeDecoratorInstance Super;
protected:				
	CBehTreeCounterDataPtr			m_workBeingPerformed;
	CBehTreeWorkDataPtr				m_workData;
	CBehTreeCounterDataPtr			m_softReactionLocker;
	TActionPointID					m_currentApId;
public:
	typedef CBehTreeNodeWorkDecoratorDefinition Definition;

	CBehTreeNodeWorkDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );	
	
	Bool Activate() override;
	void Deactivate() override;

private:
	RED_INLINE TActionPointID GetCurrentAPId()
	{
		CBehTreeWorkData* workData = m_workData.Get();
		return workData->GetSelectedAP();
	}
};

class CBehTreeNodeCustomWorkInstance;

class CBehTreeNodeCustomWorkDefinition : public IBehTreeDynamicNodeBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCustomWorkDefinition, IBehTreeDynamicNodeBaseDefinition, CBehTreeNodeCustomWorkInstance, CustomWork );
public:	
	CName GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override { return CNAME( AI_LoadCustomWorkTree ); }
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeCustomWorkDefinition );
PARENT_CLASS( IBehTreeDynamicNodeBaseDefinition );
BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();

class CBehTreeNodeCustomWorkInstance : public CBehTreeDynamicNodeInstance
{
	typedef CBehTreeDynamicNodeInstance Super;
protected:
	CBehTreeWorkDataPtr				m_workData;

public:
	typedef CBehTreeNodeCustomWorkDefinition Definition;

	CBehTreeNodeCustomWorkInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool IsAvailable() override;

	Bool Activate() override;

	CAIPerformCustomWorkTree* GetCustomBehTree();	
};
