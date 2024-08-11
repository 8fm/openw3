/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeAtomicPlayAnimationEvent.h"

class CBehTreeNodeAtomicDamageReactionInstance;

class CBehTreeNodeAtomicDamageReactionDefinition : public CBehTreeNodeAtomicPlayAnimationEventDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicDamageReactionDefinition, CBehTreeNodeAtomicPlayAnimationEventDefinition, CBehTreeNodeAtomicDamageReactionInstance, DamageReaction );
protected:
	Float m_delay;
public:

	CBehTreeNodeAtomicDamageReactionDefinition() {}

	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicDamageReactionDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicPlayAnimationEventDefinition );
	PROPERTY_EDIT( m_delay, TXT("Damage delay"));
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicDamageReactionInstance : public CBehTreeNodeAtomicPlayAnimationEventInstance
{
	typedef CBehTreeNodeAtomicPlayAnimationEventInstance Super;
protected:
	Float m_delay;
	Float m_activationDelay;
public:
	typedef CBehTreeNodeAtomicDamageReactionDefinition Definition;

	CBehTreeNodeAtomicDamageReactionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool OnListenedEvent( CBehTreeEvent& e ) override;
	Bool IsAvailable() override;
	void Complete( eTaskOutcome outcome ) override;

	void OnDestruction() override;
};