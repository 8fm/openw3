/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiFormationData.h"
#include "behTreeNodeSelectPartyMember.h"

class CBehTreeNodeSelectFormationLeaderInstance;

////////////////////////////////////////////////////////////////////////
// DEFINITION
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectFormationLeaderDefinition : public CBehTreeNodeSelectPartyMemberDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSelectFormationLeaderDefinition, CBehTreeNodeSelectPartyMemberDefinition, CBehTreeNodeSelectFormationLeaderInstance, TrySelectingLeaderAndSetupFormation );
protected:
	CBehTreeValFormation			m_formation;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeSelectFormationLeaderDefinition )
	PARENT_CLASS( CBehTreeNodeSelectPartyMemberDefinition )
	PROPERTY_EDIT( m_formation, TXT("Formation resource") )
END_CLASS_RTTI()
////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectFormationLeaderInstance : public CBehTreeNodeSelectPartyMemberInstance
{
	typedef CBehTreeNodeSelectPartyMemberInstance Super;
protected:
	CAIFormationDataPtr				m_runtimeData;
	CFormation*						m_formation;

	Bool TestCondition();
public:
	typedef CBehTreeNodeSelectFormationLeaderDefinition Definition;

	CBehTreeNodeSelectFormationLeaderInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void OnDestruction() override;

	Bool Activate() override;
	Bool IsAvailable() override;
	Int32 Evaluate() override;
};
