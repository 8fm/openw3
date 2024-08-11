/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeCustomMoveData.h"
#include "behTreeDecorator.h"

class IBehTreeNodeSetupCustomMoveDataInstance;
class CBehTreeNodeSetupCustomMoveTargetToPositionInstance;
class CBehTreeNodeSetCustomMoveTargetToInteractionPointInstance;
class CBehTreeNodeSetCustomMoveTargetToDestinationPointInstance;
class CBehTreeNodeNotifyDoorInstance;

////////////////////////////////////////////////////////////////////////////
// Abstract base of custom move target setup class
////////////////////////////////////////////////////////////////////////////
class IBehTreeNodeSetupCustomMoveDataDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeSetupCustomMoveDataDefinition, IBehTreeNodeDecoratorDefinition, IBehTreeNodeSetupCustomMoveDataInstance, SetCustomMoveTarget )
protected:
	Bool									m_setTargetForEvaluation;
public:
	IBehTreeNodeSetupCustomMoveDataDefinition()
		: m_setTargetForEvaluation( false )									{}
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeSetupCustomMoveDataDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_setTargetForEvaluation, TXT( "Set target temporary for use of evaluation process" ) )
END_CLASS_RTTI()


class IBehTreeNodeSetupCustomMoveDataInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeCustomMoveDataPtr				m_targetPtr;
	Bool									m_setTargetForEvaluation;

	virtual Bool		ComputeTargetAndHeading( Vector& outTarget, Float& outHeading ) = 0;
public:
	typedef IBehTreeNodeSetupCustomMoveDataDefinition Definition;

	IBehTreeNodeSetupCustomMoveDataInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool				Activate() override;

	Bool				IsAvailable() override;
	Int32				Evaluate() override;
};

////////////////////////////////////////////////////////////////////////////
// General - parametrized target setup node
////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSetupCustomMoveTargetToPositionDefinition : public IBehTreeNodeSetupCustomMoveDataDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSetupCustomMoveTargetToPositionDefinition, IBehTreeNodeSetupCustomMoveDataDefinition, CBehTreeNodeSetupCustomMoveTargetToPositionInstance, SetCustomMoveTargetToPosition )

protected:
	CBehTreeValVector						m_target;
	CBehTreeValFloat						m_heading;

	IBehTreeNodeDecoratorInstance*			SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeSetupCustomMoveTargetToPositionDefinition )
	PARENT_CLASS( IBehTreeNodeSetupCustomMoveDataDefinition )
	PROPERTY_EDIT( m_target, TXT("Target position") )
	PROPERTY_EDIT( m_heading, TXT("Target heading") )
END_CLASS_RTTI()

class CBehTreeNodeSetupCustomMoveTargetToPositionInstance : public IBehTreeNodeSetupCustomMoveDataInstance
{
	typedef IBehTreeNodeSetupCustomMoveDataInstance Super;
protected:
	Vector3				m_target;
	Float				m_heading;

	Bool				ComputeTargetAndHeading( Vector& outTarget, Float& outHeading ) override;
public:
	typedef CBehTreeNodeSetupCustomMoveTargetToPositionDefinition Definition;

	CBehTreeNodeSetupCustomMoveTargetToPositionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_heading( def.m_heading.GetVal( context ) )						{ Vector val; def.m_target.GetValRef( context, val ); m_target = val.AsVector3(); }
};

////////////////////////////////////////////////////////////////////////////
// Exploration - mark interaction point as target
////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSetCustomMoveTargetToInteractionPointDefintion : public IBehTreeNodeSetupCustomMoveDataDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSetCustomMoveTargetToInteractionPointDefintion, IBehTreeNodeSetupCustomMoveDataDefinition, CBehTreeNodeSetCustomMoveTargetToInteractionPointInstance, SetCustomMoveTargetToExpInteractionPoint )
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeSetCustomMoveTargetToInteractionPointDefintion )
	PARENT_CLASS( IBehTreeNodeSetupCustomMoveDataDefinition )
END_CLASS_RTTI()

class CBehTreeNodeSetCustomMoveTargetToInteractionPointInstance : public IBehTreeNodeSetupCustomMoveDataInstance
{
	typedef IBehTreeNodeSetupCustomMoveDataInstance Super;
protected:
	Vector3				m_target;
	Float				m_heading;

	Bool				ComputeTargetAndHeading( Vector& outTarget, Float& outHeading ) override;
public:
	typedef CBehTreeNodeSetCustomMoveTargetToInteractionPointDefintion Definition;

	CBehTreeNodeSetCustomMoveTargetToInteractionPointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};

////////////////////////////////////////////////////////////////////////////
// Exploration - mark destination point as target
////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSetCustomMoveTargetToDestinationPointDefintion : public IBehTreeNodeSetupCustomMoveDataDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSetCustomMoveTargetToDestinationPointDefintion, IBehTreeNodeSetupCustomMoveDataDefinition, CBehTreeNodeSetCustomMoveTargetToDestinationPointInstance, SetCustomMoveTargetToExpDestinationPoint )
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeSetCustomMoveTargetToDestinationPointDefintion )
	PARENT_CLASS( IBehTreeNodeSetupCustomMoveDataDefinition )
END_CLASS_RTTI()

class CBehTreeNodeSetCustomMoveTargetToDestinationPointInstance : public IBehTreeNodeSetupCustomMoveDataInstance
{
	typedef IBehTreeNodeSetupCustomMoveDataInstance Super;
protected:
	Vector3				m_target;
	Float				m_heading;

	Bool				ComputeTargetAndHeading( Vector& outTarget, Float& outHeading ) override;
public:
	typedef CBehTreeNodeSetCustomMoveTargetToDestinationPointDefintion Definition;

	CBehTreeNodeSetCustomMoveTargetToDestinationPointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};

////////////////////////////////////////////////////////////////////////////
// Exploration - inform door of passing
////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeNotifyDoorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeNotifyDoorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeNotifyDoorInstance, NotifyDoor )
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeNotifyDoorDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI()

class CBehTreeNodeNotifyDoorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;

private:
	THandle< CComponent >	m_metalinkComponent;

public:
	typedef CBehTreeNodeNotifyDoorDefinition Definition;

	CBehTreeNodeNotifyDoorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate() override;
};