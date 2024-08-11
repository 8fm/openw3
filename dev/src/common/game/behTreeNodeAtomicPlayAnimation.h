/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeAtomicAction.h"

class CBehTreeNodeAtomicPlayAnimationInstance;

class CBehTreeNodeAtomicPlayAnimationDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicPlayAnimationDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAtomicPlayAnimationInstance, PlayAnimationOnSlot );
protected:
	CBehTreeValCName m_animationName;
	CBehTreeValCName m_slotName;
	CBehTreeValFloat m_blendInTime;
	CBehTreeValFloat m_blendOutTime;
public:
	CBehTreeNodeAtomicPlayAnimationDefinition()
		: m_slotName( CNAME( NPC_ANIM_SLOT ) )
		, m_blendInTime( 0.2f )
		, m_blendOutTime( 0.2f )											{};

	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicPlayAnimationDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition);
	PROPERTY_EDIT( m_animationName, TXT("Animation name"));
	PROPERTY_EDIT( m_slotName, TXT("Animation slot name"));
	PROPERTY_EDIT( m_blendInTime, TXT("Animation slot name"));
	PROPERTY_EDIT( m_blendOutTime, TXT("Animation slot name"));
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicPlayAnimationInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;

protected:
	CName		m_animationName;
	CName		m_slotName;
	Float		m_blendIn;
	Float		m_blendOut;

public:
	typedef CBehTreeNodeAtomicPlayAnimationDefinition Definition;

	CBehTreeNodeAtomicPlayAnimationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;
};