/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecorator.h"


class CBehTreeNodeSelectPartyMemberInstance;

////////////////////////////////////////////////////////////////////////
// Decorator that sets named party member as sub-action target, and resets it on deactivation.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectPartyMemberDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSelectPartyMemberDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeSelectPartyMemberInstance, SelectPartyMemberAsActionTarget );
protected:
	CBehTreeValCName			m_partyMemberName;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeSelectPartyMemberDefinition() {}
};


BEGIN_CLASS_RTTI( CBehTreeNodeSelectPartyMemberDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_partyMemberName, TXT("Named party member") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectPartyMemberInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	THandle< CEncounter >		m_encounter;
	CName						m_partyMemberName;
public:
	typedef CBehTreeNodeSelectPartyMemberDefinition Definition;

	CBehTreeNodeSelectPartyMemberInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate() override;
	void Deactivate() override;
};
