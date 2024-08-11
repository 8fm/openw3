#pragma once
#include "../../common/game/behTreeDecorator.h"

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorChangeBehaviorGraphInstance;
class CBehTreeDecoratorChangeBehaviorGraphDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorChangeBehaviorGraphDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorChangeBehaviorGraphInstance, ChangeBehaviorGraph );
protected:
	CBehTreeValCName	m_behGraphNameActivate;
	CBehTreeValCName	m_behGraphNameDeactivate;
	Bool				m_activateIfBehaviorUnavailable;
public:
	CBehTreeDecoratorChangeBehaviorGraphDefinition() 
		: m_behGraphNameActivate()
		, m_behGraphNameDeactivate()
		, m_activateIfBehaviorUnavailable( false )
	{
	}
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorChangeBehaviorGraphDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_behGraphNameActivate,					TXT("The name of behavior graph you want to set on the npc, set during activate") )
	PROPERTY_EDIT( m_behGraphNameDeactivate,				TXT("The name of behavior graph you want to set on the npc, set during deactivate") )
	PROPERTY_EDIT( m_activateIfBehaviorUnavailable,	TXT("If this flag is set, decorator will get activated even if behavior graph fails to load or set") )
END_CLASS_RTTI()


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeDecoratorChangeBehaviorGraphInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CName	m_behGraphNameActivate;
	CName	m_behGraphNameDeactivate;
	Bool	m_activateIfBehaviorUnavailable;
	Bool	m_invalidActivationGraph;
	Bool	m_invalidDeactivationGraph;
	
	Bool	AsyncLoadGraph();
	Bool	ActivateGraph( CName graph );
public:
	typedef CBehTreeDecoratorChangeBehaviorGraphDefinition Definition;

	CBehTreeDecoratorChangeBehaviorGraphInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: IBehTreeNodeDecoratorInstance( def, owner, context, parent ) 
		, m_behGraphNameActivate( def.m_behGraphNameActivate.GetVal( context ) )
		, m_behGraphNameDeactivate( def.m_behGraphNameDeactivate.GetVal( context ) )
		, m_activateIfBehaviorUnavailable( def.m_activateIfBehaviorUnavailable )
		, m_invalidActivationGraph( false )
		, m_invalidDeactivationGraph( false )
	{}

	Bool	IsAvailable() override;
	Int32	Evaluate() override;

	Bool	Activate() override;
	void	Deactivate() override;
};


