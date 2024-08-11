#pragma once

/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/


#include "behTreeNode.h"

#define BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( _defClass )				IBehTreeNodeDecoratorInstance* _defClass::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const { return new Instance( *this, owner, context, parent ); }

class IBehTreeNodeDecoratorInstance;
////////////////////////////////////////////////////////////////////////
// DEFINITION
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeDecoratorDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeDecoratorDefinition, IBehTreeNodeDefinition, IBehTreeNodeDecoratorInstance, Decorator );
protected:
	IBehTreeNodeDefinition*			m_child;

	virtual IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const = 0;
public:

	IBehTreeNodeDecoratorDefinition()
		: m_child( NULL )												{}

	// Editor support
	Bool							IsTerminal() const override;
	Bool							CanAddChild() const override;
	Int32							GetNumChildren() const override;
	IBehTreeNodeDefinition*			GetChild( Int32 index ) const override;
	void							AddChild( IBehTreeNodeDefinition* node ) override;
	void							RemoveChild( IBehTreeNodeDefinition* node ) override;
	Bool							IsValid() const override;
	eEditorNodeType					GetEditorNodeType() const override;
	void							CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const override;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void							OffsetNodesPosition( Int32 offsetX, Int32 offsetY ) override;
#endif

	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeDecoratorDefinition );
	PARENT_CLASS( IBehTreeNodeDefinition );
	PROPERTY_RO( m_child, TXT("Child") );
END_CLASS_RTTI();
////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeDecoratorInstance : public IBehTreeNodeInstance
{
	typedef IBehTreeNodeInstance Super;
protected:
	IBehTreeNodeInstance*			m_child;
public:
	typedef IBehTreeNodeDecoratorDefinition Definition;

	IBehTreeNodeDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
	~IBehTreeNodeDecoratorInstance();

	void SetDecoratorChild( IBehTreeNodeInstance* child )				{ ASSERT( !m_child ); m_child = child; }
	IBehTreeNodeInstance* GetDecoratorChild() const						{ return m_child; }

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;

	////////////////////////////////////////////////////////////////////
	//! Event handling
	Bool OnEvent( CBehTreeEvent& e ) override;

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	Bool IsAvailable() override;
	Int32 Evaluate() override;

	////////////////////////////////////////////////////////////////////
	//! Custom interface
	Bool Interrupt() override;

	Int32 GetNumChildren() const override;			
	IBehTreeNodeInstance* GetChild( Int32 index ) const override;
	IBehTreeNodeInstance* GetActiveChild() const override;
	void OnDestruction() override;
#ifdef EDITOR_AI_DEBUG
	Bool DebugUpdate(Bool cascade = false);
#endif
};