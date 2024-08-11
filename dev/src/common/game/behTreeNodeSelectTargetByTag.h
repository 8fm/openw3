#pragma once

#include "behTreeDecorator.h"
#include "behTreeNodeAtomicAction.h"

class CBehTreeNodeSelectTargetByTagDecoratorInstance;
class CBehTreeNodeSelectTargetByTagInstance;

////////////////////////////////////////////////////////////////////////
// Decorator that sets sub-action target, and resets it on deactivation.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectTargetByTagDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSelectTargetByTagDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeSelectTargetByTagDecoratorInstance, SelectTargetByTagDecorator );
	DECLARE_AS_R4_ONLY
protected:
	CBehTreeValCName			m_tag;
	Bool						m_allowActivationWhenNoTarget;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeSelectTargetByTagDecoratorDefinition()
		: m_allowActivationWhenNoTarget( false )						{}

};


BEGIN_CLASS_RTTI( CBehTreeNodeSelectTargetByTagDecoratorDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_tag, TXT("Tag") );
	PROPERTY_EDIT( m_allowActivationWhenNoTarget, TXT("If flag is set to true goal will activate even if no target is found") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectTargetByTagDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CName						m_tag;
	Bool						m_allowActivationWhenNoTarget;
public:
	typedef CBehTreeNodeSelectTargetByTagDecoratorDefinition Definition;

	CBehTreeNodeSelectTargetByTagDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
		, m_tag( def.m_tag.GetVal( context ) )
		, m_allowActivationWhenNoTarget( def.m_allowActivationWhenNoTarget ) {}

	Bool Activate() override;
	void Deactivate() override;

	void OnGenerateDebugFragments( CRenderFrame* frame ) override;
};


////////////////////////////////////////////////////////////////////////
// Atomic node that sets target permanently (until set otherwise).
// Basically solution with decorator is "cleaner" but this might be
// more intuitive for designers.
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectTargetByTagDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSelectTargetByTagDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeSelectTargetByTagInstance, SelectTargetByTag );
protected:
	CName			m_tag;
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeSelectTargetByTagDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition );
	PROPERTY_EDIT( m_tag, TXT("Tag") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectTargetByTagInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	CName			m_tag;
public:
	typedef CBehTreeNodeSelectTargetByTagDefinition Definition;

	CBehTreeNodeSelectTargetByTagInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeAtomicActionInstance( def, owner, context, parent )
		, m_tag( def.m_tag )											{}

	Bool Activate() override;
	void Update() override;
};