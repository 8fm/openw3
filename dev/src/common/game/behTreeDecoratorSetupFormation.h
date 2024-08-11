/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiFormationData.h"
#include "behTreeDecorator.h"

class CBehTreeNodeDecoratorSetupFormationInstance;

////////////////////////////////////////////////////////////////////////
// Decorator that sets named party member as sub-action target, and resets it on deactivation.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDecoratorSetupFormationDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorSetupFormationDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDecoratorSetupFormationInstance, SetupFormation );
protected:
	CBehTreeValFormation			m_formation;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorSetupFormationDefinition() {}
};


BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorSetupFormationDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_formation, TXT("Formation") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDecoratorSetupFormationInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CAIFormationDataPtr				m_runtimeData;
	CFormation*						m_formation;
public:
	typedef CBehTreeNodeDecoratorSetupFormationDefinition Definition;

	CBehTreeNodeDecoratorSetupFormationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	void OnDestruction() override;

	Bool Activate() override;
	void Deactivate() override;

	void Update() override;
};
