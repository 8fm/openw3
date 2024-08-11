#pragma once
#include "../../common/game/behTreeDecorator.h"
#include "getRiderOrMountHelper.h"

///////////////////////////////////////////////////////////////////////////////
// Decorator that sets sub-action target, and resets it on deactivation.
// Target is dynamically updated as follows
// If the tagged entity is mounted the target will be set to the vehicle
// If the tagged entity is dismounted the target will be set to the vehicle
class CBehTreeNodeSelectTargetOrMountByTagDecoratorInstance;
class CBehTreeNodeSelectTargetOrMountByTagDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSelectTargetOrMountByTagDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeSelectTargetOrMountByTagDecoratorInstance, SelectTargetOrMountByTagDecorator );
protected:
	CBehTreeValCName			m_tag;
	Bool						m_allowActivationWhenNoTarget;
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeSelectTargetOrMountByTagDecoratorDefinition()	
		: m_allowActivationWhenNoTarget( false )	{}

};


BEGIN_CLASS_RTTI( CBehTreeNodeSelectTargetOrMountByTagDecoratorDefinition );
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition );
	PROPERTY_EDIT( m_tag, TXT("Tag") );
	PROPERTY_EDIT( m_allowActivationWhenNoTarget, TXT("If flag is set to true goal will activate even if no target is found") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeSelectTargetOrMountByTagDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	
public:
	typedef CBehTreeNodeSelectTargetOrMountByTagDecoratorDefinition Definition;

	CBehTreeNodeSelectTargetOrMountByTagDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
		, m_tag( def.m_tag.GetVal( context ) )
		, m_allowActivationWhenNoTarget( def.m_allowActivationWhenNoTarget ) 
		, m_getRiderOrMountHelper()			{}

	Bool Activate() override;
	void Deactivate() override;
	void Update() override;

	void OnGenerateDebugFragments( CRenderFrame* frame ) override;

private:

	CName											m_tag;
	Bool											m_allowActivationWhenNoTarget;
	CGetRiderOrMountHelper							m_getRiderOrMountHelper;
};