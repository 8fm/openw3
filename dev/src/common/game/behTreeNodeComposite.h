/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNode.h"

class IBehTreeNodeCompositeInstance;

#define BEHTREE_STANDARD_SPAWNCOMPOSITE_FUNCTION( defClass )			IBehTreeNodeCompositeInstance* defClass::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const { return new Instance( *this, owner, context, parent ); }

////////////////////////////////////////////////////////////////////////
// DEFINITION
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeCompositeDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeCompositeDefinition, IBehTreeNodeDefinition, IBehTreeNodeCompositeInstance, Composite );
protected:
	static const Uint32 MAX_CHILDREN_COUNT = 128;

	mutable TDynArray< IBehTreeNodeDefinition* >	m_children;

	virtual IBehTreeNodeCompositeInstance* SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const = 0;
	virtual void CustomPostInstanceSpawn( IBehTreeNodeCompositeInstance* instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Correct children spacing, returns true if modification occurs
	Bool CorrectChildrenSpacing();
#endif
	//! Remove null children
	Bool RemoveNullChildren();


public:

	IBehTreeNodeCompositeDefinition()
	{
	}

	//! Is terminal node
	Bool IsTerminal() const override;

	eEditorNodeType	GetEditorNodeType() const override;

	//! Can add child
	Bool CanAddChild() const override;

	//! Collect nodes
	void CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const override;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Correct children positions (left to right), returns true if modification occurs
	Bool CorrectChildrenPositions() override;

	//! Correct children order to be coherent with positions, returns true if modification occurs
	Bool CorrectChildrenOrder() override;

	//! Recursively offset nodes' position
	void OffsetNodesPosition( Int32 offsetX, Int32 offsetY ) override;

#endif
	Int32 GetNumChildren() const override;
	IBehTreeNodeDefinition* GetChild( Int32 index ) const override;
	void AddChild( IBehTreeNodeDefinition* node ) override;
	void RemoveChild( IBehTreeNodeDefinition* node ) override;

	void OnPostLoad() override;

	Bool IsValid() const override;

	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI(IBehTreeNodeCompositeDefinition);
	PARENT_CLASS(IBehTreeNodeDefinition);
	PROPERTY_RO( m_children, TXT("Children") );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////
// INSTANCE
////////////////////////////////////////////////////////////////////////

class IBehTreeNodeCompositeInstance : public IBehTreeNodeInstance
{
	typedef IBehTreeNodeInstance Super;
protected:
	TDynArray< IBehTreeNodeInstance* >	m_children;
	Uint32								m_activeChild;
public:
	static const Uint32					INVALID_CHILD = 0xffffffff;
	typedef IBehTreeNodeCompositeDefinition Definition;
	friend Definition;

	IBehTreeNodeCompositeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
	~IBehTreeNodeCompositeInstance();

	const TDynArray< IBehTreeNodeInstance* >& GetChildren() const		{ return m_children; }
	Uint32 GetActiveChildIndex() const									{ return m_activeChild; }

	////////////////////////////////////////////////////////////////////
	//! Execution cycle
	void Update() override;
	void Deactivate() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;

	////////////////////////////////////////////////////////////////////
	//! Event handling
	Bool OnEvent( CBehTreeEvent& e ) override;

	////////////////////////////////////////////////////////////////////
	//! Custom interface
	Bool Interrupt() override;

	Int32 GetNumChildren() const override;									
	IBehTreeNodeInstance* GetChild( Int32 index ) const override;
	IBehTreeNodeInstance* GetActiveChild() const override;
	void OnDestruction() override;
	Bool IsMoreImportantNodeActive( IBehTreeNodeInstance* askingChild ) override;

#ifdef EDITOR_AI_DEBUG
	Bool DebugUpdate(Bool cascade = false);
#endif
};

