#pragma once

#include "behTreeNodeAtomicIdle.h"
#include "behTreeNodeAtomicTeleport.h"
#include "behTreeNodeAtomicMove.h"
#include "behTreeNodeCondition.h"
#include "behTreeWorkData.h"

class CBehTreeNodeAtomicTeleportToActionPointInstance;
class CBehTreeNodeAtomicMoveToActionPointInstance;
class CBehTreeNodeConditionTeleportToWorkInstance;
class CBehTreeNodeAlreadyAtWorkInstance;

class CBehTreeNodeAtomicTeleportToActionPointDefinition : public CBehTreeNodeAtomicTeleportDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicTeleportToActionPointDefinition, CBehTreeNodeAtomicTeleportDefinition, CBehTreeNodeAtomicTeleportToActionPointInstance, TeleportToWork );
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeAtomicTeleportToActionPointDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicTeleportDefinition);
END_CLASS_RTTI();


class CBehTreeNodeAtomicTeleportToActionPointInstance : public CBehTreeNodeAtomicTeleportInstance
{
	typedef CBehTreeNodeAtomicTeleportInstance Super;
protected:
	CBehTreeWorkDataPtr					m_workData;

	Bool ComputeTargetAndHeading( Vector& outTarget, Float& outHeading );
public:
	typedef CBehTreeNodeAtomicTeleportToActionPointDefinition Definition;

	CBehTreeNodeAtomicTeleportToActionPointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool IsAvailable() override;

};

//////////////////////////////////////////////////////////////////////////
// Move to action point (job system)
//////////////////////////////////////////////////////////////////////////


class CBehTreeNodeAtomicMoveToActionPointDefinition : public CBehTreeNodeAtomicMoveToDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicMoveToActionPointDefinition, CBehTreeNodeAtomicMoveToDefinition, CBehTreeNodeAtomicMoveToActionPointInstance, AtomicMoveToActionPoint );
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicMoveToActionPointDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicMoveToDefinition);
END_CLASS_RTTI();

class CBehTreeNodeAtomicMoveToActionPointInstance : public CBehTreeNodeAtomicMoveToInstance
{
	typedef CBehTreeNodeAtomicMoveToInstance Super;
protected:
	CBehTreeWorkDataPtr					m_workData;

	Bool ComputeTargetAndHeading() override;
public:
	CBehTreeNodeAtomicMoveToActionPointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool OnEvent( CBehTreeEvent& e ) override;

	void OnGenerateDebugFragments( CRenderFrame* frame ) override;
};

//////////////////////////////////////////////////////////////////////////
// Conditional node that tests if one should teleport to work
//////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionTeleportToWorkDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionTeleportToWorkDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionTeleportToWorkInstance, ConditionShouldTeleportToWork );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeConditionTeleportToWorkDefinition()
		: TBaseClass( false, false, false, false )						{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionTeleportToWorkDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
END_CLASS_RTTI();

class CBehTreeNodeConditionTeleportToWorkInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;

protected:
	CBehTreeWorkDataPtr				m_workData;

	Bool ConditionCheck() override;

public:
	typedef CBehTreeNodeConditionTeleportToWorkDefinition Definition;

	CBehTreeNodeConditionTeleportToWorkInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
};

//////////////////////////////////////////////////////////////////////////
// Node that skips all the teleportation / movement stuff if we
// are skipping the initial animation
//////////////////////////////////////////////////////////////////////////
class CBehTreeNodeAlreadyAtWorkDefinition : public CBehTreeNodeCompleteImmediatelyDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAlreadyAtWorkDefinition, CBehTreeNodeCompleteImmediatelyDefinition, CBehTreeNodeAlreadyAtWorkInstance, AlreadyAtWork );
protected:
	CBehTreeValFloat					m_acceptDistance;
	CBehTreeValFloat					m_acceptRotationRequired;
public:
	CBehTreeNodeAlreadyAtWorkDefinition();

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAlreadyAtWorkDefinition )
	PARENT_CLASS( CBehTreeNodeCompleteImmediatelyDefinition )
	PROPERTY_EDIT( m_acceptDistance, TXT("Distance considered as 'already at work'") )
	PROPERTY_EDIT( m_acceptRotationRequired, TXT("Max rotation off facing 'work-point' considered as 'already at work'") )
END_CLASS_RTTI()

class CBehTreeNodeAlreadyAtWorkInstance : public CBehTreeNodeCompleteImmediatelyInstance
{
	typedef CBehTreeNodeCompleteImmediatelyInstance Super;
protected:
	CBehTreeWorkDataPtr				m_workData;
	Float							m_acceptDistanceSq;
	Float							m_acceptRotationRequired;
public:
	typedef CBehTreeNodeAlreadyAtWorkDefinition Definition;

	CBehTreeNodeAlreadyAtWorkInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool IsAvailable() override;
};