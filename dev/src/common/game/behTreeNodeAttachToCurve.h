/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeAtomicAction.h"

class CBehTreeNodeAttachToCurveInstance;
class CBehTreeNodeFlyOnCurveInstance;

class CBehTreeNodeAttachToCurveDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAttachToCurveDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAttachToCurveInstance, AttachToCurve )
protected:
	CBehTreeValCName					m_animationName;
	CBehTreeValString					m_componentName;
	CBehTreeValFloat					m_blendInTime;

public:
	CBehTreeNodeAttachToCurveDefinition()
		: m_blendInTime( 0.5f )														{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAttachToCurveDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition )
	PROPERTY_EDIT( m_animationName, TXT("Animation name") )
	PROPERTY_EDIT( m_componentName, TXT("Dummy component name. Leave empty for 'any'.") )
	PROPERTY_EDIT( m_blendInTime, TXT("Time to blend in curve movement.") )
END_CLASS_RTTI()

class CBehTreeNodeAttachToCurveInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	THandle< CPropertyAnimationSet >	m_animationSet;
	THandle< CNode >					m_node;
	String								m_componentName;
	CName								m_animationName;
	Float								m_blendInTime;

	Float								m_blendInTimeLeft;
	Vector3								m_blendingPos;
	Float								m_blendingYaw;

	virtual void PostUpdate();

public:
	typedef CBehTreeNodeAttachToCurveDefinition Definition;

	CBehTreeNodeAttachToCurveInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_animationName( def.m_animationName.GetVal( context ) )
		, m_blendInTime( def.m_blendInTime.GetVal( context ) )						{ def.m_componentName.GetValRef( context, m_componentName ); }

	void Update() override;
	Bool Activate() override;
	void Deactivate() override;

	Bool OnEvent( CBehTreeEvent& e ) override;
};


///////////////////////////////////////////////////////////////////////////////
// Animate on curve is different way of curve movement.
// We set up
class CBehTreeNodeFlyOnCurveDefinition : public CBehTreeNodeAttachToCurveDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeFlyOnCurveDefinition, CBehTreeNodeAttachToCurveDefinition, CBehTreeNodeFlyOnCurveInstance, FlyOnCurve )
protected:
	CBehTreeValCName						m_animValPitch;
	CBehTreeValCName						m_animValYaw;

	CBehTreeValFloat						m_maxPitchInput;
	CBehTreeValFloat						m_maxPitchOutput;
	CBehTreeValFloat						m_maxYawInput;
	CBehTreeValFloat						m_maxYawOutput;

public:
	CBehTreeNodeFlyOnCurveDefinition()
		: m_maxPitchInput( 30.f )
		, m_maxPitchOutput( 1.f )
		, m_maxYawInput( 45.f )
		, m_maxYawOutput( 1.f )													{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeFlyOnCurveDefinition )
	PARENT_CLASS( CBehTreeNodeAttachToCurveDefinition )
	PROPERTY_EDIT( m_animValPitch, TXT("Behavior graph pitch related value") )
	PROPERTY_EDIT( m_animValYaw, TXT("Behavior graph yaw related value") )
	PROPERTY_EDIT( m_maxPitchInput, TXT("Pitch angle that produce maximum behgraph value output") )
	PROPERTY_EDIT( m_maxPitchOutput, TXT("Maximum behavior graph pitch value") )
	PROPERTY_EDIT( m_maxYawInput, TXT("Yaw angle that produce maximum behgraph value output") )
	PROPERTY_EDIT( m_maxYawOutput, TXT("Maximum behavior graph yaw value") )
END_CLASS_RTTI()

class CBehTreeNodeFlyOnCurveInstance : public CBehTreeNodeAttachToCurveInstance
{
	typedef CBehTreeNodeAttachToCurveInstance Super;
protected:
	CName						m_animValPitch;
	CName						m_animValYaw;

	Float						m_maxPitchInput;
	Float						m_maxPitchOutput;
	Float						m_maxYawInput;
	Float						m_maxYawOutput;

	void PostUpdate() override;
public:
	typedef CBehTreeNodeFlyOnCurveDefinition Definition;

	CBehTreeNodeFlyOnCurveInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_maxPitchInput( def.m_maxPitchInput.GetVal( context ) )
		, m_maxPitchOutput( def.m_maxPitchOutput.GetVal( context ) )
		, m_maxYawInput( def.m_maxYawInput.GetVal( context ) )
		, m_maxYawOutput( def.m_maxYawOutput.GetVal( context ) )				{ m_animValPitch = def.m_animValPitch.GetVal( context ); m_animValYaw = def.m_animValYaw.GetVal( context ); }
};
