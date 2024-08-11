/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecorator.h"
#include "../engine/pathlibComponent.h"

class IBehTreeNodeExplorationQueueDecoratorInstance;
class CBehTreeNodeExplorationQueueRegisterInstance;
class CBehTreeNodeExplorationQueueUseInstance;

////////////////////////////////////////////////////////////////////////////
// Base abstract class
////////////////////////////////////////////////////////////////////////////
class IBehTreeNodeExplorationQueueDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeExplorationQueueDecoratorDefinition, IBehTreeNodeDecoratorDefinition, IBehTreeNodeExplorationQueueDecoratorInstance, ExplorationQueueDecorator )

public:
	Bool									GetInteractionPoint( CBehTreeSpawnContext& context, Vector3& outInteractionPoint ) const;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeExplorationQueueDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI()

class IBehTreeNodeExplorationQueueDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	PathLib::IComponent::SafePtr			m_metalink;

	IAIQueueMetalinkInterface*				GetQueue();
public:
	typedef IBehTreeNodeExplorationQueueDecoratorDefinition Definition;

	IBehTreeNodeExplorationQueueDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};


////////////////////////////////////////////////////////////////////////////
// Queue registering class
////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeExplorationQueueRegisterDefinition : public IBehTreeNodeExplorationQueueDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeExplorationQueueRegisterDefinition, IBehTreeNodeExplorationQueueDecoratorDefinition, CBehTreeNodeExplorationQueueRegisterInstance, ExplorationQueueRegister )
protected:
	Float									m_timePriority;
	Float									m_distancePriority;
	Float									m_maxTime;
	Float									m_maxDistance;

	IBehTreeNodeDecoratorInstance*			SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeExplorationQueueRegisterDefinition )
	PARENT_CLASS( IBehTreeNodeExplorationQueueDecoratorDefinition )
	PROPERTY_EDIT( m_timePriority, TXT( "Priority that can be achieved just by waiting" ) )
	PROPERTY_EDIT( m_distancePriority, TXT("Priority achieved by distance") )
	PROPERTY_EDIT( m_maxTime, TXT( "Time at which time priority is highest and stops to grow" ) )
	PROPERTY_EDIT( m_maxDistance, TXT("Distance at which distance priority is 0") )
END_CLASS_RTTI()

class CBehTreeNodeExplorationQueueRegisterInstance : public IBehTreeNodeExplorationQueueDecoratorInstance
{
	typedef IBehTreeNodeExplorationQueueDecoratorInstance Super;
protected:
	Float									m_timePriority;
	Float									m_distancePriority;
	Float									m_maxTime;
	Float									m_maxDistance;

	Vector3									m_interactionPoint;

	Float									m_registeredAtTime;

	// possibly virtual
	Float									CalculateWaitingPriority( IAIQueueMetalinkInterface* queue );
public:
	typedef CBehTreeNodeExplorationQueueRegisterDefinition Definition;

	CBehTreeNodeExplorationQueueRegisterInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool									Activate() override;
	void									Deactivate() override;

	Bool									OnEvent(  CBehTreeEvent& e ) override;
};


////////////////////////////////////////////////////////////////////////////
// Queue waiting class
////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeExplorationQueueUseDefinition : public IBehTreeNodeExplorationQueueDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeExplorationQueueUseDefinition, IBehTreeNodeExplorationQueueDecoratorDefinition, CBehTreeNodeExplorationQueueUseInstance, ExplorationQueueUse )
protected:
	IBehTreeNodeDecoratorInstance*			SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeExplorationQueueUseDefinition )
	PARENT_CLASS( IBehTreeNodeExplorationQueueDecoratorDefinition )
END_CLASS_RTTI()

class CBehTreeNodeExplorationQueueUseInstance : public IBehTreeNodeExplorationQueueDecoratorInstance
{
	typedef IBehTreeNodeExplorationQueueDecoratorInstance Super;
protected:
	Bool					m_locked;
public:
	typedef CBehTreeNodeExplorationQueueUseDefinition Definition;

	CBehTreeNodeExplorationQueueUseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_locked( false )													{}

	Bool									Activate() override;
	void									Deactivate() override;

	Bool									OnEvent( CBehTreeEvent& e ) override;
};

