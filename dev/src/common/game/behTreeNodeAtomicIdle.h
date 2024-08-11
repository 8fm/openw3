#pragma once

#include "behTreeNodeAtomicAction.h"

class CBehTreeNodeAtomicIdleInstance;
class CBehTreeNodeCompleteImmediatelyInstance;

////////////////////////////////////////////////////////////////////////
class CBehTreeNodeAtomicIdleDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicIdleDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAtomicIdleInstance, DoNothing );
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicIdleDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition );
END_CLASS_RTTI();

class CBehTreeNodeAtomicIdleInstance : public CBehTreeNodeAtomicActionInstance
{
public:
	typedef CBehTreeNodeAtomicIdleDefinition Definition;

	CBehTreeNodeAtomicIdleInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeAtomicActionInstance( def, owner, context, parent ) {}
};

////////////////////////////////////////////////////////////////////////
class CBehTreeNodeCompleteImmediatelyDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCompleteImmediatelyDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeCompleteImmediatelyInstance, CompleteImmediately );
protected:
	Bool				m_reportSuccess;
public:
	CBehTreeNodeCompleteImmediatelyDefinition()
		: m_reportSuccess( true )										{}
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeCompleteImmediatelyDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition );
	PROPERTY_EDIT( m_reportSuccess, TXT("Report success or failure") );
END_CLASS_RTTI();

class CBehTreeNodeCompleteImmediatelyInstance : public CBehTreeNodeAtomicActionInstance
{
protected:
	Bool				m_reportSuccess;
public:
	typedef CBehTreeNodeCompleteImmediatelyDefinition Definition;

	CBehTreeNodeCompleteImmediatelyInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeAtomicActionInstance( def, owner, context, parent )
		, m_reportSuccess( def.m_reportSuccess )						{}

	void Update() override;
};