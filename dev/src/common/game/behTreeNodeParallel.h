#pragma once

#include "behTreeNodeSpecial.h"


class CBehTreeNodeParallelInstance;

////////////////////////////////////////////////////////////////////////
// DEFINITION
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeParallelDefinition : public IBehTreeNodeSpecialDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeParallelDefinition, IBehTreeNodeSpecialDefinition, CBehTreeNodeParallelInstance, Parallel );
protected:
	IBehTreeNodeDefinition*				m_child1;
	IBehTreeNodeDefinition*				m_child2;
public:
	CBehTreeNodeParallelDefinition()
		: m_child1( NULL )
		, m_child2( NULL )													{}

	IBehTreeNodeInstance*	SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool					OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;

	Bool					IsTerminal() const override;
	Bool					IsValid() const override;

	Bool					CanAddChild() const override;
	void					RemoveChild( IBehTreeNodeDefinition* node ) override;
	Int32						GetNumChildren() const override;
	IBehTreeNodeDefinition*	GetChild( Int32 index ) const override;
	void					AddChild( IBehTreeNodeDefinition* node ) override;

	void					CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const override;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Correct children order to be coherent with positions, returns true if modification occurs
	Bool CorrectChildrenOrder() override;
#endif
};

BEGIN_CLASS_RTTI( CBehTreeNodeParallelDefinition );
	PARENT_CLASS( IBehTreeNodeSpecialDefinition );
	PROPERTY( m_child1 );
	PROPERTY( m_child2 );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeParallelInstance : public IBehTreeNodeInstance
{
protected:
	union
	{
		struct
		{
			IBehTreeNodeInstance*				m_child1;
			IBehTreeNodeInstance*				m_child2;
		};
		IBehTreeNodeInstance*					m_child[2];
	};
	
public:
	typedef CBehTreeNodeParallelDefinition Defintion;
	friend class CBehTreeNodeParallelDefinition;
	typedef IBehTreeNodeInstance Super;
	CBehTreeNodeParallelInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: IBehTreeNodeInstance( def, owner, context, parent )
		, m_child1( nullptr )
		, m_child2( nullptr )											{}

	~CBehTreeNodeParallelInstance();

	void Update() override;
	Bool Activate() override;
	void Deactivate() override;
	void Complete( eTaskOutcome outcome ) override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

	Bool OnEvent( CBehTreeEvent& e ) override;
	
	Bool Interrupt() override;
	void OnDestruction() override;

	////////////////////////////////////////////////////////////////////
	//! Handling children
	Int32 GetNumChildren() const override;
	IBehTreeNodeInstance* GetChild( Int32 index ) const override;

	Uint32 GetActiveChildCount() const override;
	IBehTreeNodeInstance* GetActiveChild( Uint32 activeChild ) const override;

	IBehTreeNodeInstance* GetParallelChild( Uint32 child ) const		{ return m_child[ child ]; }
};