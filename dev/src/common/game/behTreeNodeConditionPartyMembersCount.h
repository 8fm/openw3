/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeCondition.h"


class CBehTreeNodeConditionPartyMembersCountInstance;

class CBehTreeNodeConditionPartyMembersCountDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionPartyMembersCountDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionPartyMembersCountInstance, ConditionPartyMembersCount );

protected:
	CBehTreeValCName							m_partyMemberName;
	CBehTreeValInt								m_count;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	String GetNodeCaption() const;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionPartyMembersCountDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_partyMemberName, TXT("Forward availability computation") );
	PROPERTY_EDIT( m_count, TXT("Referenced value [inclusive]") );
END_CLASS_RTTI();

class CBehTreeNodeConditionPartyMembersCountInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	CName										m_partyMemberName;
	Uint32										m_count;
	THandle< CEncounter >						m_encounter;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionPartyMembersCountDefinition Definition;

	CBehTreeNodeConditionPartyMembersCountInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};